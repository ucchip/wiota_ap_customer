#ifndef	__CCE_H_
#define	__CCE_H_

#ifndef	__CCE_C_
#define	EXT_CCE			extern
#else
#define	EXT_CCE
#endif

#define	CCE_ADC			(0)
#define	CCE_AGC			(1)
#define	CCE_0416_NEW	(2)
#define	CCE_0416_OLD	(3)
#define	CCE_0601		(4)

#define	CCE_TYPE		CCE_AGC

#define ReverseByte(d) 	((((d) & 0xFF000000) >> 24) | (((d) & 0xFF0000) >> 8 ) | (((d) & 0xFF00) << 8 ) | (((d) & 0xFF) << 24))

// function
EXT_CCE void InitCce();
EXT_CCE void CceLoad();

#endif
