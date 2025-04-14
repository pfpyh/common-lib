#pragma once

#ifdef WINDOWS
    #ifndef COMMON_LIB_API
        #define COMMON_LIB_API __declspec(dllexport)
    #endif
#else
    #ifndef COMMON_LIB_API
        #define COMMON_LIB_API
    #endif
#endif