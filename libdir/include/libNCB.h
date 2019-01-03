//
// MATLAB Compiler: 6.6 (R2018a)
// Date: Thu Jan  3 09:42:52 2019
// Arguments: "-B""macro_default""-W""cpplib:libNCB""-T""link:lib""NCB.m"
//

#ifndef __libNCB_h
#define __libNCB_h 1

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
#ifndef LIB_libNCB_C_API 
#define LIB_libNCB_C_API /* No special import/export declaration */
#endif

/* GENERAL LIBRARY FUNCTIONS -- START */

extern LIB_libNCB_C_API 
bool MW_CALL_CONV libNCBInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_libNCB_C_API 
bool MW_CALL_CONV libNCBInitialize(void);

extern LIB_libNCB_C_API 
void MW_CALL_CONV libNCBTerminate(void);

extern LIB_libNCB_C_API 
void MW_CALL_CONV libNCBPrintStackTrace(void);

/* GENERAL LIBRARY FUNCTIONS -- END */

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

extern LIB_libNCB_C_API 
bool MW_CALL_CONV mlxNCB(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */

#ifdef __cplusplus
}
#endif


/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__MINGW64__)

#ifdef EXPORTING_libNCB
#define PUBLIC_libNCB_CPP_API __declspec(dllexport)
#else
#define PUBLIC_libNCB_CPP_API __declspec(dllimport)
#endif

#define LIB_libNCB_CPP_API PUBLIC_libNCB_CPP_API

#else

#if !defined(LIB_libNCB_CPP_API)
#if defined(LIB_libNCB_C_API)
#define LIB_libNCB_CPP_API LIB_libNCB_C_API
#else
#define LIB_libNCB_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_libNCB_CPP_API void MW_CALL_CONV NCB(int nargout, mwArray& data_num, mwArray& datatobroadcast, mwArray& maxnum, mwArray& result, mwArray& delay, const mwArray& Cache, const mwArray& Request, const mwArray& Vehicle, const mwArray& Fog, const mwArray& Rate, const mwArray& L);

/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */
#endif

#endif
