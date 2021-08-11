#ifndef __WCDTYPES_H
#define __WCDTYPES_H

#ifndef NULL
#define	NULL		((void *) 0)
#endif
#define	VOID			void
#define BIT(x)			 (1 << x)

#define	UNUSED(x)		( void )(x)

#ifndef __GPRS_SIM__  //different boolean types are defined by Windows
typedef enum
{
	FALSE,				// FALSE
	TRUE				// TRUE
} BOOL;

typedef boolean					BOOLEAN;
#endif //__GPRS_SIM__

#if defined(RISCV)

typedef sl32_t				INT32;
typedef float					FLOAT;
typedef double					DOUBLE;

typedef ul32_t			UINT32;

 
typedef volatile s8_t			HREG8;	 
typedef volatile s16_t			HREG16;	
typedef volatile sl32_t			HREG32;
typedef volatile u8_t			HUREG8;	
typedef volatile u16_t			HUREG16;
typedef volatile ul32_t			HUREG32;

#elif __GPRS_SIM__

#define FALSE 0
#define TRUE 1

//typedef unsigned          GSM_SIZE_T; /* Should be same as native size_t */

/* Note that the following must always be vanilla ints!
   (ANSI C says that the base type of a bit field IS s32_t) */

 
     // 32-bit signed integer
typedef float         FLOAT;   // 32-bit floating point
typedef double        DOUBLE;  // 64-bit extended floating point
   // 8-bit signed integer
   // 16-bit signed integer
    // 32-bit signed integer
// Unsigned types
//
// ARM/THUMB representation:
//
    // 32-bit unsigned integer
	  // 8-bit unsigned character
	  // 16-bit unsigned integer
	  // 32-bit unsigned integer

// Character type								  
 

// Hardware register types
//
typedef volatile s8_t   HREG8;   
typedef volatile s16_t  HREG16;  
typedef volatile sl32_t  HREG32;
typedef volatile u8_t  HUREG8;  
typedef volatile u16_t HUREG16;
typedef volatile ul32_t HUREG32;

#else

#endif

// Contains the GSM native size_t
// <group Global_Data_Types>

#endif	// end __WCDTYPES_H

