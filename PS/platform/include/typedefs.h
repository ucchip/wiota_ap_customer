/*
+-------------------------------------------------------------------+
| PROJECT: GSM-PS (6147)              $Workfile::   typedefs.h     $|
| Author:: Mp                         $Revision::   1.2            $|
| CREATED: 01.09.99                   $Modtime::   31 Jan 2003 16:5$|
+-------------------------------------------------------------------+

   MODULE  : TYPEDEFS.H

   PURPOSE : Standard Definitions

  $History: typedefs.h $
 *
 * *****************  Version 14  *****************
 * User: Mp           Date: 18.02.00   Time: 14:36
 * Updated in $/GPF/INC
 * new TIF structure

  $NoKeywordfs:$

*/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H
#include "uctypes.h"
/*==== CONSTANTS ==================================================*/

#define IMPORT  EXTERN
#ifndef __cplusplus
#define EXTERN  extern
#else
#define EXTERN  extern "C"
#endif
#define LOCAL   static
#define GLOBAL
#define EXPORT GLOBAL

#define EQ  ==
#define NEQ !=
#define AND &&
#define OR  ||
#define XOR(A,B) ((!(A) AND (B)) OR ((A) AND !(B)))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

/*==== TYPES ======================================================*/



typedef void                    VOID;
typedef ul32_t *         UNSIGNED_PTR;
typedef u8_t *         BYTE_PTR;

#ifndef _CDEFS_H_

#endif

/*==== EXPORT =====================================================*/

#define MAXIMUM(A,B) (((A)>(B))?(A):(B))

#define MINIMUM(A,B) (((A)<(B))?(A):(B))

// Patch Arul
//#ifdef GPRS
//  #define Sprintf sprintf
//#endif
// End Patch
#endif
