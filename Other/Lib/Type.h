#ifndef __TYPE_H_
#define	__TYPE_H_

#undef IN
#define IN

#undef OUT
#define OUT

#undef IO
#define IO

#undef NULL
#define	NULL	(0)

#undef TRUE
#define	TRUE	(0x01)

#undef FALSE
#define	FALSE	(0x00)

typedef signed char		S08;
typedef signed short	S16;
typedef signed int		S32;
typedef signed long long int	S64;

typedef unsigned char	U08;
typedef unsigned short	U16;
typedef unsigned int	U32;
typedef unsigned int	u32_t;
typedef unsigned long long int	U64;

typedef U08				BOOL;

typedef float			F32;
typedef double			F64;

// user definition
#define	USR_EPSILON		(F32)(1.0e-06)		// calculation error
#define	USR_EPSILON2	(F32)(1.0e-12)		// USR_EPSILON * USR_EPSILON

// officially definition
#define	DBL_EPSILON		(2.2204460492503131e-16)	// very small number of double
#define FLT_EPSILON		(1.192092896e-07f)			// very small number of float

#endif
