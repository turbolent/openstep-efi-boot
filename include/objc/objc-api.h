// Copyright 1988-1996 NeXT Software, Inc.

#if defined(WIN32)
    #if defined(NSBUILDINGOBJC)
        #define OBJC_EXPORT __declspec(dllexport) extern
    #else
        #if !defined(OBJC_EXPORT)
            #define OBJC_EXPORT __declspec(dllimport) extern
        #endif
    #endif
    #if !defined(OBJC_IMPORT)
        #define OBJC_IMPORT __declspec(dllimport) extern
    #endif
#endif

#if !defined(OBJC_EXPORT)
    #define OBJC_EXPORT extern
#endif

#if !defined(OBJC_IMPORT)
    #define OBJC_IMPORT extern
#endif


// obsolete
#if !defined(NEXTPDO)
    #if defined(WIN32)
        #define NEXTPDO __declspec(dllimport) extern
    #else
        #define NEXTPDO extern
    #endif
#endif
