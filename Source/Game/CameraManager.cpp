#include "StdAfx.h"
#include "CameraManager.h"
#include "Config.h"
#include "Controls.h"
#include "terra.h"
#include "Runtime.h"
#include "SafeMath.h"
#include "Universe.h"
#include "GameShell.h"

terCameraType* terCamera = NULL;
int cameraTiltLock = 1;

//Area to consider around the center in pixels
const float CAMERA_MOUSE_DELTA_AREA = 10.00f;
//Threshold for dead area, in pixels
const float CAMERA_MOUSE_DEAD_THRESHOLD = 0.5f;
//Multiplier of delta speed from center to edge of area
const float CAMERA_MOUSE_DELTA_FACTOR = 1.5f;

const float CAMERA_ZOOM_GROUND_MAX = 100.0f;
const float CAMERA_MIN_HEIGHT = 200.0f;
#if defined(GPX) || 0
//Original game values
const float CAMERA_MAX_HEIGHT = 2000.0f;
const float CAMERA_THETA_MIN = static_cast<float>(XM_PI/10.0);
const float CAMERA_THETA_MAX = static_cast<float>(XM_PI/3.0);
#else
const float CAMERA_MAX_HEIGHT = 5000.0f;
const float CAMERA_THETA_MIN = static_cast<float>(XM_PI/5.0);
const float CAMERA_THETA_MAX = static_cast<float>(XM_PI/2.85);
#endif
const float CAMERA_ZOOM_MAX = CAMERA_MAX_HEIGHT / 2.0f;
const float CAMERA_ZOOM_MIN = CAMERA_MIN_HEIGHT + 100.0f;
const float CAMERA_ZOOM_TERRAIN_THRESOLD1 = CAMERA_ZOOM_MIN + CAMERA_ZOOM_GROUND_MAX;
const float CAMERA_ZOOM_TERRAIN_THRESOLD2 = CAMERA_ZOOM_MAX;

void SetCameraPosition(cCamera *UCamera,const MatXf& Matrix)
{
	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf CameraMatrix;
	CameraMatrix=mr*ml*Matrix;

	UCamera->SetPosition(CameraMatrix);
}

//-------------------------------------------------------------------

CameraCoordinate::CameraCoordinate(const Vect2f& position, float psi, float theta, float distance)
:	position_(to3D(position, 0)),
	psi_(psi), 
	theta_(theta),
	distance_(distance)
{
	//check();
}

CameraCoordinate CameraCoordinate::operator*(float t) const
{
	return CameraCoordinate(position()*t, psi()*t, theta()*t, distance()*t);
}

CameraCoordinate CameraCoordinate::operator+(const CameraCoordinate& coord) const
{
	return CameraCoordinate(position() + coord.position(), psi() + coord.psi(), theta() + coord.theta(), distance() + coord.distance());
}

void CameraCoordinate::uncycle(const CameraCoordinate& coord0)
{
	psi_ = ::uncycle(psi_, coord0.psi(), 2*XM_PI);
	theta_ = ::uncycle(theta_, coord0.theta(), 2*XM_PI);
}

void CameraCoordinate::interpolate(const CameraCoordinate& coord0, const CameraCoordinate& coord1, float t)
{
	position_.interpolate(coord0.position(), coord1.position(), t);
	//psi_ = coord0.psi() + (coord1.psi() - coord0.psi())*t;
	psi_ = cycle(coord0.psi() + getDist(coord1.psi(), coord0.psi(), 2*XM_PI)*t, 2*XM_PI);
	theta_ = cycle(coord0.theta() + getDist(coord1.theta(), coord0.theta(), 2*XM_PI)*t, 2*XM_PI);
	distance_ = coord0.distance() + (coord1.distance() - coord0.distance())*t;
}

void CameraCoordinate::interpolateHermite(const CameraCoordinate coords[4], float u)
{
	float t2 = u*u;
	float t3 = u*t2;
	*this = coords[3]*((-t2+t3)/2.0f) + coords[0]*(-u/2.0+t2-t3/2.0) + coords[2]*(2.0*t2-3.0/2.0*t3+u/2.0) + coords[1]*(1.0-5.0/2.0*t2+3.0/2.0*t3);
}

void CameraCoordinate::check(bool restricted)
{
	static float z = FieldCluster::ZeroGround;//(float)(vMap.GetAlt(vMap.XCYCL(round(position().x)),vMap.YCYCL(xm::round(position().y))) >> VX_FRACTION);
	
	position_.z = z;
	
	if(distance() < CAMERA_ZOOM_TERRAIN_THRESOLD1)
		position_.z = z;
	else if(distance() > CAMERA_ZOOM_TERRAIN_THRESOLD2)
		position_.z = CAMERA_ZOOM_GROUND_MAX;
	else{
		float t = (distance() - CAMERA_ZOOM_TERRAIN_THRESOLD1)/(CAMERA_ZOOM_TERRAIN_THRESOLD2 - CAMERA_ZOOM_TERRAIN_THRESOLD1);
		position_.z = z + t*(CAMERA_ZOOM_GROUND_MAX - z);
	}
	
	float scroll_border = (distance() - CAMERA_ZOOM_MIN)/(CAMERA_ZOOM_MAX - CAMERA_ZOOM_MIN)*CAMERA_WORLD_SCROLL_BORDER;
	position_.x = clamp(position().x, scroll_border, vMap.H_SIZE - scroll_border);
	position_.y = clamp(position().y, scroll_border, vMap.V_SIZE - scroll_border);
	position_.z = FieldCluster::ZeroGround;

    distance_ = clamp(distance(), CAMERA_ZOOM_MIN, CAMERA_ZOOM_MAX);
	if(restricted){
		//максимально допустимый наклон на данной высоте
		//  линейно от CAMERA_THETA_MIN на CAMERA_ZOOM_MIN
		//          до CAMERA_THETA_MAX на CAMERA_ZOOM_MAX
		float t = 1 - (distance() - CAMERA_ZOOM_MIN)/(CAMERA_ZOOM_MAX - CAMERA_ZOOM_MIN);
		float theta_max = CAMERA_THETA_MIN + t*(CAMERA_THETA_MAX - CAMERA_THETA_MIN);

		theta_ = clamp(theta(), 0, theta_max);
    }

	//psi_ = cycle(psi(), 2*XM_PI);
}

int CameraCoordinate::height()
{
	return vMap.IsFullLoad() ? vMap.GetAlt(vMap.XCYCL(xm::round(position().x)), vMap.YCYCL(xm::round(position().y))) >> VX_FRACTION : 0;
}

//-------------------------------------------------------------------

terCameraType::terCameraType(cCamera* camera)
: coordinate_(Vect2f(500, 500), 0, 0, 300)
{
	Camera = camera;
	
	cameraMouseZoom = false;

	setFocus(HardwareCameraFocus);
	
	matrix_ = MatXf::ID;
//	focus_ = 1.0f;

	restricted_ = false;

	tilting_ = 0;

	replayIndex_ = -1;
	replayIndexMax_ = 0;
	interpolationTimer_ = 0;

	unit_follow = 0;
	memset(cameraSavePoints, 0, sizeof(void*)*5);
	
	cameraPsiVelocity = cameraPsiForce = 0;
	cameraThetaVelocity = cameraThetaForce = 0;
	cameraZoomVelocity = cameraZoomForce = 0;
	cameraPositionVelocity = cameraPositionForce = Vect3f::ZERO;

	explodingDuration_ = 0;
	explodingFactor_ = 0;
    
    IniManager("Perimeter.ini", false).getInt("Game","CameraTiltLock",cameraTiltLock);

	update();
}

terCameraType::~terCameraType()
{
	Camera->Release();
}


void terCameraType::setFocus(float focus)
{
	focus_ = focus;
	Camera->SetAttr(ATTRCAMERA_PERSPECTIVE);
	SetFrustumGame();
	update();
}

void terCameraType::update()
{
	Vect3f position;
	auto aspectScale = ((float) terScreenSizeX) / terScreenSizeY / (4.0f / 3);
	position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance() * aspectScale);
	position += coordinate().position();
	if(oscillatingTimer_()){
		float t = (float)(explodingDuration_ - oscillatingTimer_())/1000;
		position.x += explodingFactor_*cameraExplodingPrm.x(t);
		position.y += explodingFactor_*cameraExplodingPrm.y(t);
		position.z += explodingFactor_*cameraExplodingPrm.z(t);
	}
    
    position.z = clamp(position.z, restricted() ? CAMERA_MIN_HEIGHT : coordinate().height(), CAMERA_MAX_HEIGHT);

	matrix_ = MatXf::ID;
	matrix_.rot() = Mat3f(coordinate().theta(), X_AXIS)*Mat3f(XM_PI/2 - coordinate().psi(), Z_AXIS);
	matrix_ *= MatXf(Mat3f::ID, -position);	

	SetCameraPosition(Camera, matrix_);
}

void terCameraType::SetFrustumGame()
{
    Vect2f center(0.5f,0.5f);
    sRectangle4f clip(-0.5f,-0.5f,0.5f,0.5f);
    Vect2f focus(focus_,focus_);
    Vect2f zplane(30.0f,10000.0f);
	Camera->SetFrustum(								// устанавливается пирамида видимости
		&center,									// центр камеры
		&clip,										// видимая область камеры
		&focus,										// фокус камеры
		&zplane										// ближайший и дальний z-плоскости отсечения
		);
}

void terCameraType::SetFrustumMenu()
{
    Vect2f center(0.5f,0.5f);
    sRectangle4f clip(-0.5f,-0.5f,0.5f,0.5f);
    Vect2f focus(focus_,focus_);
    Vect2f zplane(30.0f,10000.0f);
    Camera->SetFrustum(								// устанавливается пирамида видимости
            &center,								// центр камеры
            &clip,									// видимая область камеры
            &focus,									// фокус камеры
            &zplane									// ближайший и дальний z-плоскости отсечения
    );
}

void terCameraType::SetFrustumCutScene()
{
    Vect2f center(0.5f,0.5f);
    sRectangle4f clip(-0.5f,CUT_SCENE_TOP,0.5f,CUT_SCENE_BOTTOM);
    Vect2f focus(focus_,focus_);
    Vect2f zplane(30.0f,10000.0f);
    Camera->SetFrustum(								// устанавливается пирамида видимости
            &center,								// центр камеры
            &clip,									// видимая область камеры
            &focus,									// фокус камеры
            &zplane									// ближайший и дальний z-плоскости отсечения
    );
}

void terCameraType::calcRayIntersection(float x,float y,Vect3f& v0,Vect3f& v1)
{
    Vect2f v(x,y);
	Camera->ConvertorCameraToWorld(&v1,&v);
	if(Camera->GetAttr(ATTRCAMERA_PERSPECTIVE)){
		MatXf matrix;
		Camera->GetPosition(&matrix);
		v0 = matrix.invXformVect(matrix.trans(),v0);
		v0.negate();	
	}else{
		v0.x = v1.x;
		v0.y = v1.y;
		v0.z = v1.z + 10.0f;
	}

	Vect3f dir = v1 - v0;
	float m = 9999.9f/dir.norm();
	dir *= m;
	v1 = v0 + dir;	
}


//---------------------------------------------------
void terCameraType::controlQuant()
{
	if (gameShell->isCutSceneMode()) {
		return;
	}
	if(interpolationTimer_)
		return;
	
//	cameraMouseZoom = isPressed(VK_LBUTTON) && isPressed(VK_RBUTTON);
	
	if(!unit_follow){
		if(g_controls_converter.pressed(CTRL_CAMERA_MOVE_DOWN))
			cameraPositionForce.y = CAMERA_SCROLL_SPEED_DELTA;
		
		if(g_controls_converter.pressed(CTRL_CAMERA_MOVE_UP))
			cameraPositionForce.y = -CAMERA_SCROLL_SPEED_DELTA;
		
		if(g_controls_converter.pressed(CTRL_CAMERA_MOVE_RIGHT))
			cameraPositionForce.x = CAMERA_SCROLL_SPEED_DELTA;
		
		if(g_controls_converter.pressed(CTRL_CAMERA_MOVE_LEFT))
			cameraPositionForce.x = -CAMERA_SCROLL_SPEED_DELTA;
	}
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ROTATE_UP))
		cameraThetaForce = -CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ROTATE_DOWN))
		cameraThetaForce = CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ROTATE_LEFT))
		cameraPsiForce = CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ROTATE_RIGHT))
		cameraPsiForce = -CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ZOOM_INC))
		cameraZoomForce = -CAMERA_ZOOM_SPEED_DELTA;
	
	if(g_controls_converter.pressed(CTRL_CAMERA_ZOOM_DEC))
        cameraZoomForce = CAMERA_ZOOM_SPEED_DELTA;
}

void terCameraType::mouseQuant(const Vect2f& mousePos)
{
	if (gameShell->isCutSceneMode()) {
		return;
	}
	if(interpolationTimer_ || unit_follow)
		return;

	if(xm::abs(mousePos.x + 0.5f) < CAMERA_BORDER_SCROLL_AREA_HORZ){
		if(coordinate().position().x > 0)
			cameraPositionForce.x = -CAMERA_BORDER_SCROLL_SPEED_DELTA;
	}
	else if(xm::abs(mousePos.x - 0.5f) < CAMERA_BORDER_SCROLL_AREA_HORZ){
		if(coordinate().position().x < vMap.H_SIZE) 
			cameraPositionForce.x = CAMERA_BORDER_SCROLL_SPEED_DELTA;
	}
	
	if(xm::abs(mousePos.y + 0.5f) < CAMERA_BORDER_SCROLL_AREA_UP){
		if(coordinate().position().y > 0)
			cameraPositionForce.y = -CAMERA_BORDER_SCROLL_SPEED_DELTA;
	}
	else if(xm::abs(mousePos.y - 0.5f) < CAMERA_BORDER_SCROLL_AREA_DN){
		if(coordinate().position().y < vMap.V_SIZE) 
			cameraPositionForce.y = CAMERA_BORDER_SCROLL_SPEED_DELTA;
	}
}

void limitValue(float& value, float limit) {
    if (0 < value) {
        value = std::max(0.0f, std::min(limit, value));
    } else if (value < 0) {
        value = std::min(0.0f, std::max(-limit, value));
    }
}

void cooldownForce(float& force, float value) {
    if (0 < force) {
        force = std::max(0.0f, force - value);
    } else if (force < 0) {
        force = std::min(0.0f, force + value);
    }
}

void terCameraType::tilt(Vect2f mouseDelta)
{
	if (gameShell->isCutSceneMode()) {
		return;
	}

    //Keep it within bounds, in pixels
    limitValue(mouseDelta.x, CAMERA_MOUSE_DELTA_AREA);
    limitValue(mouseDelta.y, CAMERA_MOUSE_DELTA_AREA);
    
    //printf("X %f %f F %f V %f\n", mouseDelta.x, gameShell->mousePositionRelative().x, cameraPsiForce, cameraPsiVelocity);
    
    //Check if is tilting
    tilting_ = CAMERA_MOUSE_DEAD_THRESHOLD < xm::abs(mouseDelta.y);
    bool rotating = CAMERA_MOUSE_DEAD_THRESHOLD < xm::abs(mouseDelta.x);

    //Do axis locking
    if (cameraTiltLock) {
        if (xm::abs(mouseDelta.y) <= xm::abs(mouseDelta.x)) {
            tilting_ = false;
        } else {
            rotating = false;
        }
    }
 
    //Apply delta speed according to tilt limit range
    mouseDelta *= (CAMERA_KBD_ANGLE_SPEED_DELTA * CAMERA_MOUSE_DELTA_FACTOR) / CAMERA_MOUSE_DELTA_AREA;
    
    //Apply to force
    if (tilting_) {
        cameraThetaForce = -mouseDelta.y;
        //if (!rotating) cameraPsiForce = 0;
    }
    if (rotating) {
        cameraPsiForce = mouseDelta.x;
        //if (!tilting_) cameraThetaForce = 0;
    }
}

bool terCameraType::cursorTrace(const Vect2f& pos2, Vect3f& v)
{
	Vect3f pos,dir;
	GetCamera()->GetWorldRay(pos2, pos, dir);
	return terScene->Trace(pos,pos+dir,&v);
}

void terCameraType::shift(const Vect2f& mouseDelta)
{
	if (gameShell->isCutSceneMode()) {
		return;
	}
	if(interpolationTimer_ || unit_follow)
		return;

	Vect2f delta = mouseDelta;
	Vect3f v1, v2;
	if(cursorTrace(Vect2f::ZERO, v1) && cursorTrace(delta, v2))
		delta = v2 - v1; 
	else
		delta = Vect2f::ZERO;
	
	coordinate().position() -= to3D(delta, 0);
}

void terCameraType::mouseWheel(int delta)
{
	if (gameShell->isCutSceneMode()) {
		return;
	}
	if(delta > 0)
		cameraZoomForce -= CAMERA_ZOOM_SPEED_DELTA;
	else if(delta < 0)
		cameraZoomForce += CAMERA_ZOOM_SPEED_DELTA;
}

int tilting_count = 0;
void terCameraType::quant(float mouseDeltaX, float mouseDeltaY, float delta_time, bool tilting)
{
	if(interpolationTimer_){
		float t = (frame_time() - interpolationTimer_)/(float)interpolationDuration_;
		if(t >= 1){
			if(replayIndex_ != -1){
				if(++replayIndex_ < replayIndexMax_){
					setPath(replayIndex_);
					t = 0;
				}
				else{
					stopReplayPath();
				}
			}
			else{
				interpolationTimer_ = 0;
				t = 1;
			}
		}
		coordinate_.interpolateHermite(interpolationPoints_, t);
		coordinate().check(false);
	}
	else{
		if(unit_follow)
			QuantCameraFollow(delta_time);

		//зум вслед за мышью
		if(cameraMouseZoom)
			cameraZoomForce = mouseDeltaY*CAMERA_ZOOM_MOUSE_MULT;

        //Multiplier for max value of force
        const float zoom_force_limit = 2.0f;
        const float rotate_force_limit = 2.0f;
        //Factor for force -> velocity effect
        const float zoom_factor = 40.0f;
        const float move_factor = 40.0f;
        const float rotation_factor = 60.0f;
        //Limit for rotation velocity, this avoids too high rotation causing erratic behavior
        const float rotation_velocity_limit = XM_PI * 2.0f;
        //Velocity damping factor
        float damp_factor = delta_time * 0.0005f;
        //Force cooldown factor
        float force_cooldown = delta_time * 10.0f;
        float rotation_force_cooldown = delta_time * 6.0f;
		
		//zoom
        limitValue(cameraZoomForce, CAMERA_ZOOM_SPEED_DELTA * zoom_force_limit);
		cameraZoomVelocity += cameraZoomForce*CAMERA_ZOOM_SPEED_MASS * zoom_factor;
		coordinate().distance() += cameraZoomVelocity*delta_time;

#if 0
		if(restricted()){
			//if(!cameraMouseTrack){
			//при зуме камера должна принимать макс. допустимый наклон
//			if(xm::abs(cameraZoomVelocity) > 1.0f)
			if(cameraZoomVelocity < -1.0f)
				cameraThetaForce += CAMERA_KBD_ANGLE_SPEED_DELTA;
			//}
		}
#endif
        
		//move
		cameraPositionVelocity += cameraPositionForce*CAMERA_SCROLL_SPEED_MASS*move_factor;
		
		float d = coordinate().distance()/CAMERA_MOVE_ZOOM_SCALE*delta_time;
		coordinate().position() += Mat3f(-XM_PI/2 + coordinate().psi(), Z_AXIS)*cameraPositionVelocity*d;
		
		//rotate
        limitValue(cameraPsiForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotate_force_limit);
        limitValue(cameraThetaForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotate_force_limit);
        cameraPsiVelocity   += cameraPsiForce*CAMERA_ANGLE_SPEED_MASS*rotation_factor;
        cameraThetaVelocity += cameraThetaForce*CAMERA_ANGLE_SPEED_MASS*rotation_factor;

        //printf("D %f X %f F %f V %f T %d\n", 1.0f/delta_time, gameShell->mousePositionRelative().x, cameraPsiForce, cameraPsiVelocity, tilting);
        limitValue(cameraPsiVelocity, rotation_velocity_limit);
        limitValue(cameraThetaVelocity, rotation_velocity_limit);
		
		coordinate().psi()   += cameraPsiVelocity*delta_time;
		coordinate().theta() += cameraThetaVelocity*delta_time;
		
        cameraZoomVelocity  *= CAMERA_ZOOM_SPEED_DAMP * damp_factor;
        cameraPositionVelocity *= CAMERA_SCROLL_SPEED_DAMP * damp_factor;
        cameraPsiVelocity   *= CAMERA_ANGLE_SPEED_DAMP * damp_factor;
        cameraThetaVelocity *= CAMERA_ANGLE_SPEED_DAMP * damp_factor;

        cooldownForce(cameraZoomForce, CAMERA_ZOOM_SPEED_DELTA * force_cooldown);
        cooldownForce(cameraPositionForce.x, CAMERA_BORDER_SCROLL_SPEED_DELTA * force_cooldown);
        cooldownForce(cameraPositionForce.y, CAMERA_BORDER_SCROLL_SPEED_DELTA * force_cooldown);

        //Hnadle tilting mode cooldown
        if (tilting) {
            tilting_count = 2;
            //Only cooldown the inactive axis
            if (tilting_) {
                cooldownForce(cameraPsiForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotation_force_cooldown);
            } else  {
                cooldownForce(cameraThetaForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotation_force_cooldown);
            }
        } else {
            if (0 < tilting_count) {
                //Might be a bogus update between mouse updates, don't apply cooldown yet
                tilting_count -= 1;
            } else {
                cooldownForce(cameraPsiForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotation_force_cooldown);
                cooldownForce(cameraThetaForce, CAMERA_KBD_ANGLE_SPEED_DELTA * rotation_force_cooldown);
            }
        }
		
		coordinate().check(restricted());
	}

	update();
}

void terCameraType::setTarget(const CameraCoordinate& coord, int duration) 
{ 
	interpolationPoints_[0] = interpolationPoints_[1] = coordinate_;
	interpolationPoints_[2] = interpolationPoints_[3] = coord;
	interpolationTimer_ = frame_time();
	interpolationDuration_ = duration; 
}

void terCameraType::QuantCameraFollow(float delta_time)
{
	coordinate().position() += (unit_follow->position() - coordinate().position())*CAMERA_FOLLOW_AVERAGE_TAU*unitFollowTimer_();
}

void terCameraType::SaveCamera(int n)
{
	xassert(n < 5);
	
	if(!cameraSavePoints[n])
		cameraSavePoints[n] = new CameraCoordinate;
	
	*cameraSavePoints[n] = CameraCoordinate(coordinate().position(), coordinate().psi(), coordinate().theta(), coordinate().distance());
}

void terCameraType::RestoreCamera(int n)
{
	xassert(n < 5);
	
	if(!cameraSavePoints[n])
		return;
	
	setTarget(*cameraSavePoints[n], 1000);
}

void terCameraType::SetCameraFollow(terUnitBase* unit, int transitionTime)
{
	unit_follow = unit;
	unitFollowTimer_.start(transitionTime + 1);
}

void terCameraType::destroyLink()
{
	if(unit_follow && (!unit_follow->alive() 
	  || (unit_follow->attr()->ID == UNIT_ATTRIBUTE_SQUAD && safe_cast<terUnitSquad*>(unit_follow)->Empty()))){
		SetCameraFollow(0);
	}
}

void terCameraType::startReplayPath(int duration, int cycles)
{
	interpolationDuration_ = duration; 

	if(path_.empty())
		return;
	
	if(path_.size() == 1){
		if(!duration){
			setCoordinate(path_.front());
			return;
		}
		else{
			CameraCoordinate coord = path_.front();
			path_.push_back(coord);
		}
	}

	replayIndexMax_ = cycles*path_.size() - 1;
	setPath(replayIndex_ = 0);
}

void terCameraType::stopReplayPath()
{
	replayIndex_ = -1;
	interpolationTimer_ = 0;
}

void terCameraType::setPath(int index) 
{ 
	xassert(!path_.empty());
	index %= path_.size();
	if(replayIndexMax_ > path_.size()){ // Зацикленное повторение последовательности
		interpolationPoints_[0] = path_[(index - 1 + path_.size()) % path_.size()];
		interpolationPoints_[1] = path_[index];
		interpolationPoints_[2] = path_[(index + 1) % path_.size()];
		interpolationPoints_[3] = path_[(index + 2) % path_.size()];
	}
	else{
		interpolationPoints_[0] = path_[clamp(index - 1, 0, path_.size() - 1)];
		interpolationPoints_[1] = path_[index];
		interpolationPoints_[2] = path_[clamp(index + 1, 0, path_.size() - 1)];
		interpolationPoints_[3] = path_[clamp(index + 2, 0, path_.size() - 1)];
	}

	interpolationTimer_ = frame_time();
}

void terCameraType::erasePath() 
{ 
	stopReplayPath();
	path_.clear(); 
	coordinate().psi() = cycle(coordinate().psi(), 2*XM_PI);
}

void terCameraType::savePath(SaveCameraSplineData& data)
{
	if(!path_.empty()){
		CoordList::iterator ci;
		FOR_EACH(path_, ci){
			data.path.push_back(SaveCameraData());
			ci->save(data.path.back());
		}
	}
	else{
		data.path.push_back(SaveCameraData());
		coordinate().save(data.path.back());
	}
}

void terCameraType::loadPath(const SaveCameraSplineData& data, bool addCurrentPosition)
{
//	xassert(!isPlayingBack());
	path_.clear();
	if(addCurrentPosition){
		CameraCoordinate coord = coordinate();
		CameraCoordinate coord0;
		coord0.load(data.path.front());
		coord.uncycle(coord0);
		path_.push_back(coord);
	}
	std::vector<SaveCameraData>::const_iterator i;
	FOR_EACH(data.path, i){
		path_.emplace_back();
		path_.back().load(*i);
	}
}

void CameraCoordinate::save(SaveCameraData& data) const 
{
	data.position = position_;
	data.psi = psi_;
	data.theta = theta_;
	data.distance = distance_;
}

void CameraCoordinate::load(const SaveCameraData& data)
{
	position_ = to3D(data.position, 0);
	psi_ = data.psi;
	theta_ = data.theta;
	distance_ = data.distance;
}


//-----------------------------------
void terCameraType::startOscillation(int duration, float factor)
{
	explodingFactor_ = factor;
	oscillatingTimer_.start(explodingDuration_ = duration);
}

void terCameraType::reset()
{
	oscillatingTimer_.stop();
	stopReplayPath();
}


