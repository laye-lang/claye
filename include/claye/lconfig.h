#ifndef LCONFIG_H_
#define LCONFIG_H_

/*
@@ LAYE_USE_C11 controlls the use of non-C11 features.
** Define it if you want 'claye' to avoid the use of C23 features,
** or Windows-specific features on Windows.
*/
/* #define LAYE_USE_C11 */

#if !defined(LAYE_USE_C11) && defined(_WIN32)
#    define LAYE_USE_WINDOWS
#endif

#if defined(LAYE_USE_WINDOWS)
#    define LAYE_USE_DLL
#    if defined(_MSC_VER)
#        define LAYE_USE_C11
#    endif
#endif

#if defined(LAYE_USE_LINUX)
#    define LAYE_USE_POSIX
#    define LAYE_USE_DLOPEN
#endif

#if defined(LAYE_BUILD_AS_DLL)
#    if defined(LAYE_LIB)
#        define LAYE_API __declspec(dllexport)
#    else
#        define LAYE_API __declspec(dllimport)
#    endif
#else /* LAYE_BUILD_AS_DLL */
#    define LAYE_API extern
#endif

#endif /* LCONFIG_H_ */
