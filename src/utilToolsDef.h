#ifndef __UtilToolsDef_h__
#define __UtilToolsDef_h__


#ifdef _WIN32
#ifndef __cplusplus
#ifndef UTILTOOLS_API_EXPORTS
#define UTILTOOLS_API extern     __declspec(dllimport)
#else
#define UTILTOOLS_API extern     __declspec(dllexport)
#endif
#else
#ifndef UTILTOOLS_API_EXPORTS
#define UTILTOOLS_API extern "C" __declspec(dllimport)
#else
#define UTILTOOLS_API extern "C" __declspec(dllexport)
#endif
#endif
#else
#ifndef __cplusplus
#ifndef UTILTOOLS_API
#define UTILTOOLS_API extern 
#endif
#else
#ifndef UTILTOOLS_API
#define UTILTOOLS_API extern "C" __attribute__ ((visibility("default"))) 
#endif
#endif
#endif

#endif