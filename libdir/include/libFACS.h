//
// MATLAB Compiler: 6.6 (R2018a)
// Date: Wed Jan  2 20:53:27 2019
// Arguments: "-B""macro_default""-W""cpplib:libFACS""-T""link:lib""FACS.m"
//

#ifndef __libFACS_h
#define __libFACS_h 1

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
#ifndef LIB_libFACS_C_API 
#define LIB_libFACS_C_API /* No special import/export declaration */
#endif

/* GENERAL LIBRARY FUNCTIONS -- START */

extern LIB_libFACS_C_API 
bool MW_CALL_CONV libFACSInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_libFACS_C_API 
bool MW_CALL_CONV libFACSInitialize(void);

extern LIB_libFACS_C_API 
void MW_CALL_CONV libFACSTerminate(void);

extern LIB_libFACS_C_API 
void MW_CALL_CONV libFACSPrintStackTrace(void);

/* GENERAL LIBRARY FUNCTIONS -- END */

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

extern LIB_libFACS_C_API 
bool MW_CALL_CONV mlxFACS(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */

#ifdef __cplusplus
}
#endif


/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__MINGW64__)

#ifdef EXPORTING_libFACS
#define PUBLIC_libFACS_CPP_API __declspec(dllexport)
#else
#define PUBLIC_libFACS_CPP_API __declspec(dllimport)
#endif

#define LIB_libFACS_CPP_API PUBLIC_libFACS_CPP_API

#else

#if !defined(LIB_libFACS_CPP_API)
#if defined(LIB_libFACS_C_API)
#define LIB_libFACS_CPP_API LIB_libFACS_C_API
#else
#define LIB_libFACS_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_libFACS_CPP_API void MW_CALL_CONV FACS(int nargout, mwArray& Num, mwArray& Result1, mwArray& weight_clique, const mwArray& Cache, const mwArray& Request, const mwArray& Vehicle, const mwArray& Neighbor, const mwArray& Fog, const mwArray& Rate, const mwArray& L);

/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */
#endif

#endif
