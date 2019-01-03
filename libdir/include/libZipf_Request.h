//
// MATLAB Compiler: 6.6 (R2018a)
// Date: Fri Nov 30 15:14:43 2018
// Arguments:
// "-B""macro_default""-W""cpplib:libZipf_Request""-T""link:lib""Zipf_Request.m"
//

#ifndef __libZipf_Request_h
#define __libZipf_Request_h 1

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
#ifndef LIB_libZipf_Request_C_API 
#define LIB_libZipf_Request_C_API /* No special import/export declaration */
#endif

/* GENERAL LIBRARY FUNCTIONS -- START */

extern LIB_libZipf_Request_C_API 
bool MW_CALL_CONV libZipf_RequestInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_libZipf_Request_C_API 
bool MW_CALL_CONV libZipf_RequestInitialize(void);

extern LIB_libZipf_Request_C_API 
void MW_CALL_CONV libZipf_RequestTerminate(void);

extern LIB_libZipf_Request_C_API 
void MW_CALL_CONV libZipf_RequestPrintStackTrace(void);

/* GENERAL LIBRARY FUNCTIONS -- END */

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

extern LIB_libZipf_Request_C_API 
bool MW_CALL_CONV mlxZipf_Request(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

/* C INTERFACE -- MLX WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */

#ifdef __cplusplus
}
#endif


/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- START */

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__MINGW64__)

#ifdef EXPORTING_libZipf_Request
#define PUBLIC_libZipf_Request_CPP_API __declspec(dllexport)
#else
#define PUBLIC_libZipf_Request_CPP_API __declspec(dllimport)
#endif

#define LIB_libZipf_Request_CPP_API PUBLIC_libZipf_Request_CPP_API

#else

#if !defined(LIB_libZipf_Request_CPP_API)
#if defined(LIB_libZipf_Request_C_API)
#define LIB_libZipf_Request_CPP_API LIB_libZipf_Request_C_API
#else
#define LIB_libZipf_Request_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_libZipf_Request_CPP_API void MW_CALL_CONV Zipf_Request(int nargout, mwArray& Request, const mwArray& NUM, const mwArray& D_NUM, const mwArray& V_NUM, const mwArray& THETA, const mwArray& Encode_Num);

/* C++ INTERFACE -- WRAPPERS FOR USER-DEFINED MATLAB FUNCTIONS -- END */
#endif

#endif
