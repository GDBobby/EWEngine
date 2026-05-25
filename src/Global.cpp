#include "EWEngine/Global.h"


#include <thread>

namespace EWE{
    std::thread::id mainThreadID;
    void SetMainThread(){
        mainThreadID = std::this_thread::get_id();
    }


#ifdef _WIN32
#defien WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

    bool CheckMainThread() {
        return std::this_thread::get_id() == mainThreadID;
    }
    void NameCurrentThread(std::string_view name){
#ifdef _WIN32
        std::wstring wname(name.begin(), name.end());
        SetThreadDescription(GetCurrentThread(), wname.c_str());

#elif defined(__linux__)
        //linux has a 16 char limit for thread name?
        char buf[16];
        auto len = std::min(name.size(), (size_t)15);
        std::copy_n(name.begin(), len, buf);
        buf[len] = '\0';
        pthread_setname_np(pthread_self(), buf);
#endif
    }
} //namespace EWE