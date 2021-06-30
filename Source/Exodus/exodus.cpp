#include "exodus.h"
#include "tweaks.h"
#include <cstring>
#include <cctype>
#include <thread>
#include <sys/stat.h>
#include <SDL.h>
#include <pwd.h>
#include <string>
#include <filesystem>
#include <fcntl.h>
#include <SimpleIni.h>
#include "xerrhand.h"
#include "xutl.h"

//Usual open but with path conversion
int _open(const char* path, int oflags, int sflags) {
    return open(convert_path(path).c_str(), oflags, sflags);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int _controlfp(unsigned int newval, unsigned int mask) {
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

short GetAsyncKeyState(int vKey) {
    //TODO use SDL2?
    return 0;
}

void SetFocus(HWND hwnd) {
    SDL_RaiseWindow(fromHWND(hwnd));
}

void ShowCursor(bool show) {
    SDL_ShowCursor(show);
}

void SetCursor(HCURSOR cursor) {
    SDL_SetCursor(cursor);
}

HANDLE LoadImage(void*, const char* name, UINT type, int width, int height, UINT) {
    SDL_Surface* surface = SDL_LoadBMP(convert_path_resource(name).c_str());
    
    if (!surface) {
        fprintf(stderr, "LoadImage %s\n", name);
        SDL_PRINT_ERROR("LoadImage SDL_LoadBMP");
    } else {
        if (type == IMAGE_CURSOR) {
            SDL_Cursor* cursor = SDL_CreateColorCursor(
                    surface, 0, 0
            );
            if (cursor) {
                return reinterpret_cast<HANDLE>(cursor);
            } else {
                SDL_PRINT_ERROR("LoadImage SDL_CreateColorCursor");
            }
        }
    }
    
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetPrivateProfileString(const char* section,const char* key,const char* defaultVal,
                              char* returnBuffer, DWORD bufferSize, const char* filePath) {
    CSimpleIniA ini;
    SI_Error rc = ini.LoadFile(filePath);
    if (rc < 0) {
        fprintf(stderr, "Error reading %s file: %d", filePath, rc);
        return 0;
    };
    const char* val = ini.GetValue(section, key, defaultVal);
    if (val) {
        SDL_strlcpy(returnBuffer, val, bufferSize);
    } else {
        *returnBuffer = 0;
    }
    return 1;
}

DWORD WritePrivateProfileString(const char* section,const char* key,const char* value, const char* filePath) {
    CSimpleIniA ini;
    SI_Error rc = ini.LoadFile(filePath);
    if (rc < 0) {
        fprintf(stderr, "Error writing %s file: %d", filePath, rc);
        return 0;
    };
    ini.SetValue(section, key, value);
    return 1;
}

bool GetComputerName(char* out, DWORD* size) {
    if (gethostname(out, *size) == 0) {
        *size = SDL_strlen(out) + 1;
        return true;
    }
    *out = 0;
    *size = 0;
    return false;
}

bool GetUserName(char* out, DWORD* size) {
    struct passwd *pwd = getpwuid(getuid());
    if (pwd) {
        size_t maxsize = *size;
        SDL_strlcpy(out, pwd->pw_name, maxsize);
        *size = std::min(maxsize, SDL_strlen(out) + 1);
        return true;
    }
    
    *out = 0;
    *size = 0;
    return false;
}

void ZeroMemory(void *p, size_t n) {
    memset(p, 0, n);
}

void Sleep(uint32_t millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

char* _strlwr(char* str)
{
    unsigned char* p = (unsigned char *) str;

    while (*p) {
        *p = tolower((unsigned char) *p);
        p++;
    }

    return str;
}

char* _strupr(char* str)
{
    unsigned char* p = (unsigned char *) str;

    while (*p) {
        *p = toupper((unsigned char) *p);
        p++;
    }

    return str;
}

//According to MSDN: Both __iscsym and __iswcsym return a nonzero value if c is a letter, underscore, or digit. 
int __iscsym(int c) {
    if (c == '_') return 1;
    return isalnum(c);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _mkdir(const char* path) {
    std::filesystem::create_directories(path);
}

char* _fullpath(char* absolutePath, const char* relativePath, size_t maxLength) {
    if (relativePath) {
        bool malloced = absolutePath == nullptr;
        if (malloced) {
            absolutePath = static_cast<char*>(malloc(maxLength));
        }
        if (realpath(convert_path(relativePath).c_str(), absolutePath) != nullptr) {
            return absolutePath;
        }
        if (malloced) {
            free(absolutePath);
        }
    }
    return nullptr;
}

void _splitpath(const char* path, char*, char* dir, char* fname, char* ext) {
    std::string path_str = convert_path_posix(path);
    
    //Get dir and file
    std::size_t lastpath_limiter = path_str.find_last_of('/');
    std::string dir_str = path_str.substr(0, lastpath_limiter);
    SDL_strlcpy(dir, dir_str.c_str(), _MAX_DIR);
    std::string file_str = path_str.substr(lastpath_limiter, path_str.length());
    
    //Get file name and ext
    std::size_t lastext_limiter = file_str.find_last_of('.');
    std::string filename_str = file_str.substr(0, lastext_limiter);
    SDL_strlcpy(fname, filename_str.c_str(), _MAX_FNAME);
    std::string ext_str = file_str.substr(lastext_limiter, file_str.length());
    SDL_strlcpy(ext, ext_str.c_str(), _MAX_EXT);
}

void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext) {
    std::string fullpath = std::string() + dir + fname + ext;
    SDL_strlcpy(path, fullpath.c_str(), MAX_PATH);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EnterCriticalSection(CRITICAL_SECTION *m) {
    pthread_mutex_lock(m);
}

void LeaveCriticalSection(CRITICAL_SECTION *m) {
    pthread_mutex_unlock(m);
}

void InitializeCriticalSection(CRITICAL_SECTION *m) {
    pthread_mutex_init(m, nullptr);
}

void DeleteCriticalSection(CRITICAL_SECTION *m) {
    pthread_mutex_destroy(m);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CreateEvent(int, bool manualReset, bool initialState, int) {
    return neosmart::CreateEvent(manualReset, initialState);
}

void DestroyEvent(HANDLE event) {
    neosmart::DestroyEvent(reinterpret_cast<neosmart::neosmart_event_t>(event));
}

void SetEvent(HANDLE event) {
    neosmart::SetEvent(reinterpret_cast<neosmart::neosmart_event_t>(event));
}

void ResetEvent(HANDLE event) {
    neosmart::ResetEvent(reinterpret_cast<neosmart::neosmart_event_t>(event));
}

DWORD WaitForSingleObject(HANDLE event, uint64_t milliseconds) {
    return neosmart::WaitForEvent(reinterpret_cast<neosmart::neosmart_event_t>(event), milliseconds);
}

DWORD WaitForMultipleObjects(int count, HANDLE* events, bool waitAll, uint64_t milliseconds) {
    return neosmart::WaitForMultipleEvents(reinterpret_cast<neosmart::neosmart_event_t*>(events), count, waitAll, milliseconds);
}

HANDLE CreateThread(void*, size_t,  void *(*start_address) (void *), void* arg, DWORD, THREAD_ID* tid) {
    if (pthread_create(tid, nullptr, start_address, arg) != 0) {
        *tid = 0;
        return nullptr;
    }
    return neosmart::CreateEvent(true, false);
}
