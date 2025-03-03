#include "StdAfx.h"

#include "UnitAttribute.h"
#include "InterfaceScript.h"
#include "EditArchive.h"
#include "XPrmArchive.h"
#include "BinaryArchive.h"

SINGLETON_PRM(InterfaceAttributes, "InterfaceAttributes", "Scripts\\InterfaceAttributes")  interfaceAttr;

BEGIN_ENUM_DESCRIPTOR(ShellControlType,"ShellControlType");
REGISTER_ENUM(SQSH_GENERAL_TYPE,"SQSH_GENERAL_TYPE");
REGISTER_ENUM(SQSH_MAP_WINDOW_TYPE,"SQSH_MAP_WINDOW_TYPE");
REGISTER_ENUM(SQSH_PUSH_BUTTON_TYPE,"SQSH_PUSH_BUTTON_TYPE");
REGISTER_ENUM(SQSH_REPLAY_PLAYER_BUTTON_TYPE,"SQSH_REPLAY_PLAYER_BUTTON_TYPE");
REGISTER_ENUM(SQSH_BUTTON_TYPE,"SQSH_BUTTON_TYPE");
REGISTER_ENUM(SQSH_LEGION_BUTTON_TYPE,"SQSH_LEGION_BUTTON_TYPE");
REGISTER_ENUM(SQSH_LIST_BOX_TYPE,"SQSH_LIST_BOX_TYPE");
REGISTER_ENUM(SQSH_CHAT_BOX_TYPE,"SQSH_CHAT_BOX_TYPE");
REGISTER_ENUM(SQSH_STATS_LIST_TYPE,"SQSH_STATS_LIST_TYPE");
REGISTER_ENUM(SQSH_SLIDER_TYPE,"SQSH_SLIDER_TYPE");
REGISTER_ENUM(SQSH_COMBO_TYPE,"SQSH_COMBO_TYPE");
REGISTER_ENUM(SQSH_COLOR_COMBO_TYPE,"SQSH_COLOR_COMBO_TYPE");
REGISTER_ENUM(SQSH_EDIT_BOX_TYPE,"SQSH_EDIT_BOX_TYPE");
REGISTER_ENUM(SQSH_INGAME_CHAT_EDIT_BOX_TYPE,"SQSH_INGAME_CHAT_EDIT_BOX_TYPE");
REGISTER_ENUM(SQSH_TEXT_WINDOW_TYPE,"SQSH_TEXT_WINDOW_TYPE");
REGISTER_ENUM(SQSH_TEXT_STRING_WINDOW_TYPE,"SQSH_TEXT_STRING_WINDOW_TYPE");
REGISTER_ENUM(SQSH_TEXT_SCROLLABLE_WINDOW_TYPE,"SQSH_TEXT_SCROLLABLE_WINDOW_TYPE");
REGISTER_ENUM(SQSH_COMPLEX_PUSH_BUTTON_TYPE,"SQSH_COMPLEX_PUSH_BUTTON_TYPE");
REGISTER_ENUM(SQSH_ATOM_BUTTON_TYPE,"SQSH_ATOM_BUTTON_TYPE");
REGISTER_ENUM(SQSH_MULTITEX_WINDOW,"SQSH_MULTITEX_WINDOW");
REGISTER_ENUM(SQSH_MOVE_BUTTON_TYPE,"SQSH_MOVE_BUTTON_TYPE");
REGISTER_ENUM(SQSH_SCALE_BUTTON_TYPE,"SQSH_SCALE_BUTTON_TYPE");
REGISTER_ENUM(SQSH_SCALE_RESULT_BUTTON_TYPE,"SQSH_SCALE_RESULT_BUTTON_TYPE");
REGISTER_ENUM(SQSH_PUSHSCALE_BUTTON_TYPE,"SQSH_PUSHSCALE_BUTTON_TYPE");
REGISTER_ENUM(SQSH_DLG_TYPE,"SQSH_DLG_TYPE");
REGISTER_ENUM(SQSH_WORLD_WINDOW_TYPE,"SQSH_WORLD_WINDOW_TYPE");
REGISTER_ENUM(SQSH_SPLASHSCREEN,"SQSH_SPLASHSCREEN");
REGISTER_ENUM(SQSH_MAPWINDOW,"SQSH_MAPWINDOW");
REGISTER_ENUM(SQSH_PORTRAIT_TYPE,"SQSH_PORTRAIT_TYPE");
REGISTER_ENUM(SQSH_NOMAD_LOGO_TYPE,"SQSH_NOMAD_LOGO_TYPE");
REGISTER_ENUM(SQSH_CREDITS_TEXT_TYPE,"SQSH_CREDITS_TEXT_TYPE");
REGISTER_ENUM(SQSH_TERRAINBUILDBUTTON_TYPE,"SQSH_TERRAINBUILDBUTTON_TYPE");
REGISTER_ENUM(SQSH_TABSHEET_TYPE,"SQSH_TABSHEET_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_ENERGY_TYPE,"SQSH_PROGRESS_ENERGY_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_COLLECTED_TYPE,"SQSH_PROGRESS_COLLECTED_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_SHIELD_TYPE,"SQSH_PROGRESS_SHIELD_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_TERRAIN_TYPE,"SQSH_PROGRESS_TERRAIN_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_MUTATION_TYPE,"SQSH_PROGRESS_MUTATION_TYPE");
REGISTER_ENUM(SQSH_PROGRESS_UNIT_CHARGE_TYPE,"SQSH_PROGRESS_UNIT_CHARGE_TYPE");
REGISTER_ENUM(SQSH_PLAYER_COLOR_WND_TYPE,"SQSH_PLAYER_COLOR_WND_TYPE");
REGISTER_ENUM(SQSH_INFOWND_TYPE,"SQSH_INFOWND_TYPE");
REGISTER_ENUM(SQSH_HINTWND_TYPE,"SQSH_HINTWND_TYPE");
REGISTER_ENUM(SQSH_BACKGROUND_TYPE,"SQSH_BACKGROUND_TYPE");
REGISTER_ENUM(SQSH_GENERAL_WND_TYPE,"SQSH_GENERAL_WND_TYPE");
REGISTER_ENUM(SQSH_CHATINFO_WND_TYPE,"SQSH_CHATINFO_WND_TYPE");
REGISTER_ENUM(SQSH_NETLATENCYINFOWND_TYPE, "SQSH_NETLATENCYINFOWND_TYPE");
END_ENUM_DESCRIPTOR(ShellControlType);

BEGIN_ENUM_DESCRIPTOR(SHELL_CONTROL_STATE,"SHELL_CONTROL_STATE");
REGISTER_ENUM(SQSH_VISIBLE,"SQSH_VISIBLE");
REGISTER_ENUM(SQSH_ENABLED,"SQSH_ENABLED");
REGISTER_ENUM(SQSH_SWITCH,"SQSH_SWITCH");
REGISTER_ENUM(SQSH_NOEFFECT,"SQSH_NOEFFECT");
REGISTER_ENUM(SQSH_MARKED,"SQSH_MARKED");
REGISTER_ENUM(SQSH_CHECK2,"SQSH_CHECK2");
REGISTER_ENUM(SQSH_TRANSPARENT,"SQSH_TRANSPARENT");
END_ENUM_DESCRIPTOR(SHELL_CONTROL_STATE);

BEGIN_ENUM_DESCRIPTOR(SHELL_ALIGN,"SHELL_ALIGN");
REGISTER_ENUM(SHELL_ALIGN_LEFT,"SHELL_ALIGN_LEFT");
REGISTER_ENUM(SHELL_ALIGN_CENTER,"SHELL_ALIGN_CENTER");
REGISTER_ENUM(SHELL_ALIGN_RIGHT,"SHELL_ALIGN_RIGHT");
END_ENUM_DESCRIPTOR(SHELL_ALIGN);

BEGIN_ENUM_DESCRIPTOR(ControlHitTestMode,"ControlHitTestMode");
REGISTER_ENUM(HITTEST_DEFAULT,"HITTEST_DEFAULT");
REGISTER_ENUM(HITTEST_NONE,"HITTEST_NONE");
END_ENUM_DESCRIPTOR(ControlHitTestMode);

BEGIN_ENUM_DESCRIPTOR(InterfaceEventCode,"InterfaceEventCode");
REGISTER_ENUM(EVENT_PRESSED,"EVENT_PRESSED");
REGISTER_ENUM(EVENT_UNPRESSED,"EVENT_UNPRESSED");
REGISTER_ENUM(EVENT_RPRESSED,"EVENT_RPRESSED");
REGISTER_ENUM(EVENT_RUNPRESSED,"EVENT_RUNPRESSED");
REGISTER_ENUM(EVENT_HOLD,"EVENT_HOLD");
REGISTER_ENUM(EVENT_SLIDERUPDATE,"EVENT_SLIDERUPDATE");
REGISTER_ENUM(EVENT_CREATEWND,"EVENT_CREATEWND");
REGISTER_ENUM(EVENT_SHOWWND,"EVENT_SHOWWND");
REGISTER_ENUM(EVENT_HIDEWND,"EVENT_HIDEWND");
REGISTER_ENUM(EVENT_DRAWWND,"EVENT_DRAWWND");
REGISTER_ENUM(EVENT_WHEEL,"EVENT_WHEEL");
REGISTER_ENUM(EVENT_DOUBLECLICK,"EVENT_DOUBLECLICK");
REGISTER_ENUM(EVENT_TAB_CHANGED,"EVENT_TAB_CHANGED");
REGISTER_ENUM(EVENT_PRESSED_DISABLED,"EVENT_PRESSED_DISABLED");
REGISTER_ENUM(EVENT_ON_WINDOW,"EVENT_ON_WINDOW");
END_ENUM_DESCRIPTOR(InterfaceEventCode);

BEGIN_ENUM_DESCRIPTOR(iEventID,"iEventID");
REGISTER_ENUM(iEVENT_ID_EFFECT,"iEVENT_ID_EFFECT");
END_ENUM_DESCRIPTOR(iEventID);

