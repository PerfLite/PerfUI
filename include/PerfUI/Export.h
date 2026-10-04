#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(PERFUI_STATIC)
        #define PERFUI_API
    #elif defined(PERFUI_BUILD_DLL)
        #define PERFUI_API __declspec(dllexport)
    #else
        #define PERFUI_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define PERFUI_API __attribute__((visibility("default")))
    #else
        #define PERFUI_API
    #endif
#endif
