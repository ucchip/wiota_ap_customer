/**
 *\author van Kempen Alexandre
 * \mainpage Ptpd v2 Documentation
 * \version 0.1
 * \date 12 may 2009
 * \section implementation Implementation
 * PTTdV2 is not a full implementation of 1588 - 2008 standard.
 * It is implemented only with use of Transparent Clock and Peer delay mechanism, according to 802.1AS requierements.
*/

/**
*\file
* \brief Main functions used in ptpdv2
*
* This header file includes all others headers.
* It defines functions which are not dependant of the operating system.
 */

#ifndef PTPD_H_
#define PTPD_H_

#include "rtthread.h"
#include "ptpv2_constants.h"
#include "limits.h"
#include "ptpv2_constants_dep.h"
#include "ptpv2_datatypes_dep.h"
#include "ptpv2_datatypes.h"
#include "ptpv2_dep.h"
#include <W5500/w5500.h>
#include <sys/socket.h>
//#include <dfs_select.h>
#include "netdev.h"
#include "wiz.h"
#include "board.h"
#include "uc_timer.h"
#include "uc_wiota_api.h"

#ifdef USING_STATIC_LIST
#include "flash_data_manage.h"
#endif
/** \name arith.c
 * -Timing management and arithmetic*/
/**\{*/
/* arith.c */

/**
 * \brief Convert Integer64 into TimeInternal structure
 */
void integer64_to_internalTime(Integer64, TimeInternal *);
/**
 * \brief Convert TimeInternal into Timestamp structure (defined by the spec)
 */
void fromInternalTime(TimeInternal *, Timestamp *);

/**
 * \brief Convert Timestamp to TimeInternal structure (defined by the spec)
 */
void toInternalTime(TimeInternal *, Timestamp *);

/**
 * \brief Use to normalize a TimeInternal structure
 *
 * The nanosecondsField member must always be less than 10⁹
 * This function is used after adding or substracting TimeInternal
 */
void normalizeTime(TimeInternal *);

/**
 * \brief Add two InternalTime structure and normalize
 */
void addTime(TimeInternal *, TimeInternal *, TimeInternal *);

/**
 * \brief Substract two InternalTime structure and normalize
 */
void subTime(TimeInternal *, TimeInternal *, TimeInternal *);
/** \}*/

/** \name bmc.c
 * -Best Master Clock Algorithm functions*/
/**\{*/
/* bmc.c */
/**
 * \brief Compare data set of foreign masters and local data set
 * \return The recommended state for the port
 */
UInteger8 bmc(ForeignMasterRecord *, RunTimeOpts *, PtpClock *);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(PtpClock *);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader *, MsgAnnounce *, PtpClock *);

/**
 * \brief Initialize datas
 */
void initData(RunTimeOpts *, PtpClock *);
/** \}*/

/** \name protocol.c
 * -Execute the protocol engine*/
/**\{*/
/**
 * \brief Protocol engine
 */
/* protocol.c */
void protocol(RunTimeOpts *, PtpClock *);
/** \}*/

ssize_t netSendEvent(Octet *buf, UInteger16 length, NetPath *netPath);

ssize_t netSendGeneral(Octet *buf, UInteger16 length, NetPath *netPath);

//Diplay functions usefull to debug
void displayRunTimeOpts(RunTimeOpts *);
void displayDefault(PtpClock *);
void displayCurrent(PtpClock *);
void displayParent(PtpClock *);
void displayGlobal(PtpClock *);
void displayPort(PtpClock *);
void displayForeignMaster(PtpClock *);
void displayOthers(PtpClock *);
void displayBuffer(PtpClock *);
void displayPtpClock(PtpClock *);
void timeInternal_display(TimeInternal *);
void clockIdentity_display(ClockIdentity);
void netPath_display(NetPath *);
void intervalTimer_display(IntervalTimer *);
void integer64_display(Integer64 *);
void timeInterval_display(TimeInterval *);
void portIdentity_display(PortIdentity *);
void clockQuality_display(ClockQuality *);
void iFaceName_display(Octet *);
void unicast_display(Octet *);

void msgHeader_display(MsgHeader *);
void msgAnnounce_display(MsgAnnounce *);
void msgSync_display(MsgSync *sync);
void msgFollowUp_display(MsgFollowUp *);
void msgPDelayReq_display(MsgPDelayReq *);
void set_ptp_real_mode(uint8_t val);

//0:default master,
//1:real master
//2:real slave

uint8_t get_ptp_real_mode(void);

#endif /*PTPD_H_*/
