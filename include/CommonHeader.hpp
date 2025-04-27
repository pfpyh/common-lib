#pragma once

#ifdef WINDOWS
    #ifdef BUILDING_COMMON_LIB
        #define COMMON_LIB_API __declspec(dllexport)
    #else
        #define COMMON_LIB_API __declspec(dllimport)
    #endif
#else
    #ifndef COMMON_LIB_API
        #define COMMON_LIB_API
    #endif
#endif