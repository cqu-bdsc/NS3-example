//
// MATLAB Compiler: 6.6 (R2018a)
// Date: Thu Jan  3 10:11:35 2019
// Arguments: "-B""macro_default""-W""cpplib:libISXD""-T""link:lib""ISXD.m"
//

#ifndef __libISXD_h
#define __libISXD_h 1

#if defined(__cplusplus) && !defined(mclmcrrt_h) && defined(__linux__)
#  pragma implementation "mclmcrrt.h"
#endif
#include "mclmcrrt.h"
#include "mclcppclass.h"
#ifdef __cplusplus
extern "C" {
#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_libISXD_C_API 
#define LIB_libISXD_C_API /* No special import/export declaration */
#endif

/* GENERAL LIBRARY FUNCTIONS -- START */

extern LIB_libISXD_C_API 
bool MW_CALL_CONV libISXDInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_libISXD_C_API 
bool MW_CALL_CONV libISXDInitialize(void);

extern LIB_libISXD_C_API 
void MW_CALL_CONV libISXDTerminate(void);

extern LIB_libISXD_C_API 
void MW_CALL_CONV libISXDPrintStackTrace(void);

/* GENERAL LIBRARY FUNCTIONS -- END */

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

extern LIB_libISXD_C_API 
bool MW_CALL_CONV mlxISXD(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */

#ifdef __cplusplus
}
#endif


/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__MINGW64__)

#ifdef EXPORTING_libISXD
#define PUBLIC_libISXD_CPP_API __declspec(dllexport)
#else
#define PUBLIC_libISXD_CPP_API __declspec(dllimport)
#endif

#define LIB_libISXD_CPP_API PUBLIC_libISXD_CPP_API

#else

#if !defined(LIB_libISXD_CPP_API)
#if defined(LIB_libISXD_C_API)
#define LIB_libISXD_CPP_API LIB_libISXD_C_API
#else
#define LIB_libISXD_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_libISXD_CPP_API void MW_CALL_CONV ISXD(int nargout, mwArray& Num, mwArray& Temp, mwArray& delay, const mwArray& Cache, const mwArray& Request, const mwArray& Vehicle, const mwArray& Fog, const mwArray& Rate, const mwArray& L);

/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */
#endif

#endif
