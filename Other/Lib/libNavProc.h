#ifndef	__LIB_NAV_PROC_H_
#define	__LIB_NAV_PROC_H_

#ifndef	__LIB_NAV_PROC_C_
#define	EXT_NAVPROC		extern
#else
#define	EXT_NAVPROC
#endif

typedef enum
{
	PVT_MODE_GPS = 0, 
	PVT_MODE_BDS = 1
}ENUM_PVT_MODE;

typedef enum
{
	WORK_MODE_TST_GAIN = 0, 
	WORK_MODE_TST_AGC = 1, 
	WORK_MODE_NORMAL = 2
}ENUM_WORK_MODE;

typedef enum
{
	RST_MODE_COLD = 1,
	RST_MODE_WARM = 2, 	// warm 
	RST_MODE_HOT = 3, 
	RST_MODE_AGNSS = 4,
	RST_READ_POS_REC = 5
}ENUM_RST_MODE;

typedef enum
{
	SCENE_STATIONARY = 1, 	// stationary
	SCENE_PEDESTRIAN = 2, 	// pedestrian
	SCENE_PORTABLE = 3, 	// portable
	SCENE_AUTOMOTIVE = 4, 	// automotive
	SCENE_WRIST = 5, 		// wrist
	SCENE_SEA = 6, 			// sea
	SCENE_AIRBORNE_1G = 7, 	// airborne < 1g
	SCENE_AIRBORNE_2G = 8, 	// airborne < 2g
	SCENE_AIRBORNE_4G = 9, 	// airborne < 4g
	SCENE_CUSTOMIZED = 10	// customized
}ENUM_SCENE_MODE;

typedef union
{
	S32 iData;
	F32 fData;
}UNION_MUT_CONV;

typedef struct
{
	F32 fDat;
	F32 fErr;
}TYP_F32, *PTYP_F32;

typedef struct
{
	S16 sYear;
	U08 ucMon;
	U08 ucDay;
	U08 ucHour;
	U08 ucMin;
	F32 fSec;
}STU_RTC, *PSTU_RTC;

typedef enum
{
	PVT_STATE_NULL = 0, 	// default is not position
	PVT_STATE_OK = 1, 		// positioning successful
	PVT_STATE_INV = 2, 		// inversion failure
	PVT_STATE_ITR = 3, 		// iteration failed
	PVT_STATE_FAULT = 4, 	// there is a faulty satellite that can't be removed
	PVT_STATE_FAILED = 5, 	// positioning failed
	PVT_STATE_ABNORMAL_PDOP = 6, 	// abnormal pdop
	PVT_STATE_ABNORMAL_XYZT = 7, 	// abnormal xyzt
	PVT_STATE_ABNORMAL_ALT = 8, 	// abnormal of alt
	PVT_STATE_ABNORMAL_PRC = 9, 	// data proc is abnormal
	PVT_STATE_LACK = 10		// the number of available satellites is not enough to positioning
}ENUM_PVT_STATE;

typedef struct
{
	TYP_F32 tX;
	TYP_F32 tY;
	TYP_F32 tZ;
	TYP_F32 tT;
}STU_COOR_XYZ, *PSTU_COOR_XYZ;

typedef struct
{
	TYP_F32 tLon;	// longitude[unit:rad]
	TYP_F32 tLat;	// latitude[unit:rad]
	F32 fAlt;		// altitude[unit:m]
}STU_COOR_LLA, *PSTU_COOR_LLA;

typedef struct
{
	F32 fEst;		// eastward speed[unit:m/s]
	F32 fNrt;		// northward speed[unit:m/s]
	F32 fUp;		// upward speed[unit:m/s]
	F32 fV;			// speed over ground[unit:m/s]
	F32 fHeading;	// course over ground[unit:rad]
}STU_COOR_ENU, *PSTU_COOR_ENU;

typedef struct
{
	STU_COOR_XYZ tPos;
	STU_COOR_XYZ tVel;
}STU_PVT_XYZ, *PSTU_PVT_XYZ;

typedef struct
{
	ENUM_PVT_STATE eState;
	STU_PVT_XYZ tXyz;
	STU_COOR_LLA tLla;
	STU_COOR_ENU tEnu;
	F32 fPdop;
	F32 fHdop;
	F32 fVdop;
	F32 fTdop;
	F32 fGdop;
}STU_USR_PVT, *PSTU_USR_PVT;

typedef void (*CALL_BACK)(IN const PSTU_RTC pRtc, IN const PSTU_USR_PVT pPvt);

EXT_NAVPROC void pvt_start_service(IN ENUM_PVT_MODE eMode, IN ENUM_RST_MODE eRst, IN ENUM_SCENE_MODE eScene, IN ENUM_WORK_MODE eWork, IN const CALL_BACK GetPvt);
EXT_NAVPROC void pvt_stop_service();
EXT_NAVPROC void TicIsr();
EXT_NAVPROC void pvt_init_sv(unsigned char ucSv[12]);
EXT_NAVPROC void UnpackData(IN U08 *ptrData, IN U16 usLen);

#endif
