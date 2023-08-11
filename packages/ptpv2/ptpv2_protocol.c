/* protocol.c */

#include "ptpv2_main.h"
#include <wizchip_socket.h>
#include "uc_wiota_api.h"
#include "easy_ptp.h"

Boolean doInit(RunTimeOpts *, PtpClock *);
void doState(RunTimeOpts *, PtpClock *);
void toState(UInteger8, RunTimeOpts *, PtpClock *);

void handle(RunTimeOpts *, PtpClock *);
void handleAnnounce(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handleSync(MsgHeader *, Octet *, ssize_t, TimeInternal *, Boolean, RunTimeOpts *, PtpClock *);
void handleFollowUp(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handlePDelayReq(MsgHeader *, Octet *, ssize_t, TimeInternal *, Boolean, RunTimeOpts *, PtpClock *);
void handleDelayReq(MsgHeader *, Octet *, ssize_t, TimeInternal *, Boolean, RunTimeOpts *, PtpClock *);
void handlePDelayResp(MsgHeader *, Octet *, TimeInternal *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handleDelayResp(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handlePDelayRespFollowUp(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handleManagement(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);
void handleSignaling(MsgHeader *, Octet *, ssize_t, Boolean, RunTimeOpts *, PtpClock *);

void issueAnnounce(RunTimeOpts *, PtpClock *);
void issueSync(RunTimeOpts *, PtpClock *);
void issueFollowup(TimeInternal *, RunTimeOpts *, PtpClock *);
void issuePDelayReq(RunTimeOpts *, PtpClock *);
void issueDelayReq(RunTimeOpts *, PtpClock *);
void issuePDelayResp(TimeInternal *, MsgHeader *, RunTimeOpts *, PtpClock *);
void issueDelayResp(TimeInternal *, MsgHeader *, RunTimeOpts *, PtpClock *);
void issuePDelayRespFollowUp(TimeInternal *, MsgHeader *, RunTimeOpts *, PtpClock *);
void issueManagement(MsgHeader *, MsgManagement *, RunTimeOpts *, PtpClock *);

void addForeign(Octet *, MsgHeader *, PtpClock *);

//小功能函数库
//数组转换为UINT32类型(大端字节数组，高字节在低地址)，转换为UINT32类型
static uint32_t BufToU32(uint8_t *in, uint8_t len)
{
    uint32_t value = 0;
    uint8_t i;

    if ((in == NULL) || (len > 4))
    {
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        value = value << 8;

        value |= in[i];
    }

    return value;
}

/* loop forever. doState() has a switch for the actions and events to be
   checked for 'port_state'. the actions and events may or may not change
   'port_state' by calling toState(), but once they are done we loop around
   again and perform the actions required for the new 'port_state'. */
void protocol(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    DBG("event POWERUP\n");

    toState(PTP_INITIALIZING, rtOpts, ptpClock);

    for (;;)
    {
        if (ptpClock->portState != PTP_INITIALIZING)
        {
            doState(rtOpts, ptpClock);
            rt_thread_mdelay(2);
        }
        else if (!doInit(rtOpts, ptpClock))
        {
            ERROR("wait for dhcp allocate ip...\r\n");

            rt_thread_mdelay(1000);
        }
    }
}

/* perform actions required when leaving 'port_state' and entering 'state' */
void toState(UInteger8 state, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    ptpClock->message_activity = TRUE;

    /* leaving state tasks */
    switch (ptpClock->portState)
    {
    case PTP_MASTER:
        timerStop(SYNC_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        break;

    case PTP_SLAVE:
        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);

        if (rtOpts->E2E_mode)
        {
            timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        }
        else
        {
            timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        }

        initClock(rtOpts, ptpClock);
        break;

    case PTP_PASSIVE:
        timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
        break;

    case PTP_LISTENING:
        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
        break;

    default:
        break;
    }

    /* entering state tasks */
    /*No need of PRE_MASTER state because of only ordinary clock implementation*/

    switch (state)
    {
    case PTP_INITIALIZING:
        DBG("state PTP_INITIALIZING\n");
        ptpClock->portState = PTP_INITIALIZING;
        break;

    case PTP_FAULTY:
        DBG("state PTP_FAULTY\n");
        ptpClock->portState = PTP_FAULTY;
        break;

    case PTP_DISABLED:
        DBG("state PTP_DISABLED\n");
        ptpClock->portState = PTP_DISABLED;
        break;

    case PTP_LISTENING:
        DBG("state PTP_LISTENING\n");
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->announceReceiptTimeout) * (pow(2, ptpClock->logAnnounceInterval)), ptpClock->itimer);
        ptpClock->portState = PTP_LISTENING;
        break;

    case PTP_MASTER:
        DBG("state PTP_MASTER\n");

        timerStart(SYNC_INTERVAL_TIMER, pow(2, ptpClock->logSyncInterval), ptpClock->itimer);
        DBG("SYNC INTERVAL TIMER : %f \n", pow(2, ptpClock->logSyncInterval));
        timerStart(ANNOUNCE_INTERVAL_TIMER, pow(2, ptpClock->logAnnounceInterval), ptpClock->itimer);
        timerStart(PDELAYREQ_INTERVAL_TIMER, pow(2, ptpClock->logMinPdelayReqInterval), ptpClock->itimer);
        ptpClock->portState = PTP_MASTER;
        break;

    case PTP_PASSIVE:
        DBG("state PTP_PASSIVE\n");

        timerStart(PDELAYREQ_INTERVAL_TIMER, pow(2, ptpClock->logMinPdelayReqInterval), ptpClock->itimer);
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->announceReceiptTimeout) * (pow(2, ptpClock->logAnnounceInterval)), ptpClock->itimer);

        ptpClock->portState = PTP_PASSIVE;
        break;

    case PTP_UNCALIBRATED:
        DBG("state PTP_UNCALIBRATED\n");
        ptpClock->portState = PTP_UNCALIBRATED;
        break;

    case PTP_SLAVE:
        DBG("state PTP_SLAVE\n");
        initClock(rtOpts, ptpClock);

        ptpClock->waitingForFollow = FALSE;
        ptpClock->pdelay_req_send_time.seconds = 0;
        ptpClock->pdelay_req_send_time.nanoseconds = 0;
        ptpClock->pdelay_req_receive_time.seconds = 0;
        ptpClock->pdelay_req_receive_time.nanoseconds = 0;
        ptpClock->pdelay_resp_send_time.seconds = 0;
        ptpClock->pdelay_resp_send_time.nanoseconds = 0;
        ptpClock->pdelay_resp_receive_time.seconds = 0;
        ptpClock->pdelay_resp_receive_time.nanoseconds = 0;

        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->announceReceiptTimeout) * (pow(2, ptpClock->logAnnounceInterval)), ptpClock->itimer);

        if (rtOpts->E2E_mode)
        {
            timerStart(DELAYREQ_INTERVAL_TIMER, pow(2, ptpClock->logMinDelayReqInterval), ptpClock->itimer);
        }

        else
        {
            timerStart(PDELAYREQ_INTERVAL_TIMER, pow(2, ptpClock->logMinPdelayReqInterval), ptpClock->itimer);
        }

        ptpClock->portState = PTP_SLAVE;
        break;

    default:
        DBG("to unrecognized state\n");
        break;
    }

    if (rtOpts->displayStats)
    {
        displayStats(rtOpts, ptpClock);
    }
}

Boolean doInit(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    if (!wiz_ip_is_assign())
    {
        return FALSE;
    }

    DBG("manufacturerIdentity: %s\n", MANUFACTURER_ID);

    /* initialize networking */
    //    netShutdown(&ptpClock->netPath);

    if (!netInit(&ptpClock->netPath, rtOpts, ptpClock))
    {
        ERROR("failed to initialize network\n");
        toState(PTP_FAULTY, rtOpts, ptpClock);
        return FALSE;
    }

    /* initialize other stuff */
    initData(rtOpts, ptpClock);

    initTimer();

    initClock(rtOpts, ptpClock);

    m1(ptpClock);

    msgPackHeader(ptpClock->msgObuf, ptpClock);

    toState(PTP_LISTENING, rtOpts, ptpClock);

    return TRUE;
}

/* handle actions and events for 'port_state' */
void doState(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    UInteger8 state;

    // s_systime timetemp = {0, 0};

    //  static rt_uint32_t last_sync_sec=0;

    ptpClock->message_activity = FALSE;

    switch (ptpClock->portState)
    {
    case PTP_LISTENING:
    case PTP_PASSIVE:
    case PTP_SLAVE:

    case PTP_MASTER:
        /*State decision Event*/
        if (ptpClock->record_update)
        {
            DBGV("event STATE_DECISION_EVENT\n");
            ptpClock->record_update = FALSE;
            state = bmc(ptpClock->foreign, rtOpts, ptpClock);

            if (state == PTP_MASTER)
            {
                set_ptp_real_mode(1);
            }
            else if (state == PTP_SLAVE)
            {
                set_ptp_real_mode(2);
            }

            if (state != ptpClock->portState)
            {

                toState(state, rtOpts, ptpClock);
            }
        }
        break;

    default:
        break;
    }

    switch (ptpClock->portState)
    {
    case PTP_FAULTY:
        /* imaginary troubleshooting */

        DBG("event FAULT_CLEARED\n");
        toState(PTP_INITIALIZING, rtOpts, ptpClock);
        return;

    case PTP_LISTENING:
    case PTP_PASSIVE:
    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        handle(rtOpts, ptpClock);

        if (timerExpired(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer))
        {
            DBGV("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");
            ptpClock->number_foreign_records = 0;
            ptpClock->foreign_record_i = 0;
            if (!ptpClock->slaveOnly && ptpClock->clockQuality.clockClass != 255)
            {
                m1(ptpClock);
                toState(PTP_MASTER, rtOpts, ptpClock);
            }
            else if (ptpClock->portState != PTP_LISTENING)
                toState(PTP_LISTENING, rtOpts, ptpClock);
        }

        if (rtOpts->E2E_mode)
        {
            if (timerExpired(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer))
            {
                DBGV("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
                issueDelayReq(rtOpts, ptpClock);
            }
        }

        else
        {
            if (timerExpired(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer))
            {
                DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
                issuePDelayReq(rtOpts, ptpClock);
            }
        }

        break;

    case PTP_MASTER:

        // sys_time_get(&timetemp);

        // if((timetemp.sec-last_sync_sec)>=1)
        // {
        // 	last_sync_sec=timetemp.sec;

        // 	issueSync(rtOpts,ptpClock);

        // }

        if (timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer))
        {

            DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
            issueSync(rtOpts, ptpClock);
        }

        if (timerExpired(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer))
        {

            DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
            issueAnnounce(rtOpts, ptpClock);
        }

        if (!rtOpts->E2E_mode)
        {
            if (timerExpired(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer))
            {
                DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
                issuePDelayReq(rtOpts, ptpClock);
            }
        }

        handle(rtOpts, ptpClock);

        if (ptpClock->slaveOnly || ptpClock->clockQuality.clockClass == 255)
        {
            toState(PTP_LISTENING, rtOpts, ptpClock);
        }

        break;

    case PTP_DISABLED:
        handle(rtOpts, ptpClock);
        break;

    default:
        DBG("(doState) do unrecognized state\n");
        break;
    }
}

/* check and handle received messages */
void handle(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    static rt_uint8_t local_time_init = 0;
    uint8_t remote_ip[4] = {0};
    uint16_t remote_port = 0;
    ssize_t length = 0; //, sendlen = 0;
    Boolean isFromSelf;
    s_systime timetemp = {0, 0};
    TimeInternal time = {0, 0};
    int socket = -1;

    if (!ptpClock->message_activity)
    {
        length = getSn_RX_RSR(ptpClock->netPath.eventSock);

        if (length <= 0)
        {
            length = getSn_RX_RSR(ptpClock->netPath.generalSock);

            if (length <= 0)
            {
                return;
            }
            else
            {
                socket = ptpClock->netPath.generalSock;
            }
        }
        else
        {
            socket = ptpClock->netPath.eventSock;
        }

        wizchip_recvfrom(socket, (uint8_t *)ptpClock->msgIbuf, length, remote_ip, &remote_port);
#if 0
       if (ptpClock->portState != PTP_MASTER)
        {
            //loopback
            if (socket == ptpClock->netPath.eventSock)
            {
                wizchip_sendto(socket, (uint8_t *)ptpClock->msgIbuf, sendlen, remote_ip, PTP_EVENT_PORT);
            }
            if (socket == ptpClock->netPath.generalSock)
            {
                wizchip_sendto(socket, (uint8_t *)ptpClock->msgIbuf, sendlen, remote_ip, PTP_GENERAL_PORT);
            }
        }
#endif
    }

    if (!length)
    {
        return;
    }

    ptpClock->message_activity = TRUE;

    if (length < HEADER_LENGTH)
    {
        ERROR("message shorter than header length\r\n");
        toState(PTP_FAULTY, rtOpts, ptpClock);
        return;
    }

    msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);

    switch (ptpClock->msgTmpHeader.messageType)
    {

    case SYNC:

        if (local_time_init == 0)
        {
            timetemp.sec = BufToU32((uint8_t *)&ptpClock->msgIbuf[36], 4);
            timetemp.usec = BufToU32((uint8_t *)&ptpClock->msgIbuf[40], 4) / 1000;
            sys_time_init(&timetemp);

            local_time_init = 1;
        }
        else
        {
            if (ptpClock->portState == PTP_MASTER)
            {
                time.seconds = BufToU32((uint8_t *)&ptpClock->msgIbuf[36], 4);
                time.nanoseconds = BufToU32((uint8_t *)&ptpClock->msgIbuf[40], 4);
            }
            else
            {
                sys_time_get(&timetemp);
                time.seconds = timetemp.sec;
                time.nanoseconds = timetemp.usec * 1000;
            }
        }

        break;
    case DELAY_REQ:
        sys_time_get(&timetemp);
        time.seconds = timetemp.sec;
        time.nanoseconds = timetemp.usec * 1000;
        break;
    case PDELAY_REQ:
        //            time.seconds = BufToU32(&ptpClock->msgIbuf[36],4);
        //            time.nanoseconds = BufToU32(&ptpClock->msgIbuf[40],4)*1000;
        //            ptp_time_get(&time);

        break;
    case PDELAY_RESP:
        //            time.seconds = BufToU32(&ptpClock->msgIbuf[36],4);
        //            time.nanoseconds = BufToU32(&ptpClock->msgIbuf[40],4)*1000;
        //            ptp_time_get(&time);

        break;
    }

    if (ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber)
    {
        DBGV("ignore version %d message\r\n", ptpClock->msgTmpHeader.versionPTP);
        return;
    }

    if (ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber)
    {
        DBGV("ignore message from domainNumber %d\r\n", ptpClock->msgTmpHeader.domainNumber);
        return;
    }

    /*Spec 9.5.2.2*/
    //sourcePortIdentity.clockIdentity:源MAC地址
    //portIdentity.clockIdentity      :本地MAC地址
    isFromSelf = (ptpClock->portIdentity.portNumber == ptpClock->msgTmpHeader.sourcePortIdentity.portNumber && !memcmp(ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity, ptpClock->portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));

    /* subtract the inbound latency adjustment if it is not a loop back and the
    time stamp seems reasonable */
    if (!isFromSelf && time.seconds > 0)
    {
        subTime(&time, &time, &rtOpts->inboundLatency);
    }
    switch (ptpClock->msgTmpHeader.messageType)
    {

    case ANNOUNCE:
        handleAnnounce(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    case SYNC:
        handleSync(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, &time, isFromSelf, rtOpts, ptpClock);
        break;

    case FOLLOW_UP:
        handleFollowUp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    case DELAY_REQ:
        handleDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, &time, isFromSelf, rtOpts, ptpClock);
        break;

    case PDELAY_REQ:
        handlePDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, &time, isFromSelf, rtOpts, ptpClock);
        break;

    case DELAY_RESP:
        handleDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    case PDELAY_RESP:
        handlePDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, &time, length, isFromSelf, rtOpts, ptpClock);
        break;

    case PDELAY_RESP_FOLLOW_UP:
        handlePDelayRespFollowUp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    case MANAGEMENT:
        handleManagement(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    case SIGNALING:
        handleSignaling(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, length, isFromSelf, rtOpts, ptpClock);
        break;

    default:
        DBG("handle: unrecognized message\r\n");
        break;
    }
}

/*spec 9.5.3*/
void handleAnnounce(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Boolean isFromCurrentParent = FALSE;

    DBGV("HandleAnnounce : Announce message received : \r\n");

    if (length < ANNOUNCE_LENGTH)
    {
        ERROR("short Announce message\r\n");
        toState(PTP_FAULTY, rtOpts, ptpClock);
        return;
    }

    switch (ptpClock->portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:

        DBGV("Handleannounce : disreguard \r\n");
        return;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        if (isFromSelf)
        {
            DBGV("HandleAnnounce : Ignore message from self \r\n");
            return;
        }

        ptpClock->record_update = TRUE; // Valid announce message is received : BMC algorithm will be executed

        isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);

        switch (isFromCurrentParent)
        {
        case TRUE:

            msgUnpackAnnounce(ptpClock->msgIbuf, &ptpClock->msgTmp.announce);
            s1(header, &ptpClock->msgTmp.announce, ptpClock);

            /*Reset Timer handling Announce receipt timeout*/
            timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->announceReceiptTimeout) * (pow(2, ptpClock->logAnnounceInterval)), ptpClock->itimer);
            break;

        case FALSE:

            /*addForeign takes care of AnnounceUnpacking*/
            addForeign(ptpClock->msgIbuf, header, ptpClock);

            /*Reset Timer handling Announce receipt timeout*/
            timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->announceReceiptTimeout) * (pow(2, ptpClock->logAnnounceInterval)), ptpClock->itimer);
            break;

        default:
            DBGV("HandleAnnounce : (isFromCurrentParent) strange value ! \r\n");
            return;

        } //switch on (isFromCurrentParrent)
        break;

    case PTP_MASTER:
    default:

        if (isFromSelf)
        {
            DBGV("HandleAnnounce : Ignore message from self \r\n");
            return;
        }

        DBGV("Announce message from another foreign master\r\n");
        addForeign(ptpClock->msgIbuf, header, ptpClock);
        ptpClock->record_update = TRUE;
        break;

    } //switch on (port_state)
}

void handleSync(MsgHeader *header, Octet *msgIbuf, ssize_t length, TimeInternal *time, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    TimeInternal OriginTimestamp;
    TimeInternal correctionField;

    Boolean isFromCurrentParent = FALSE;
    DBGV("Sync message received : \r\n");

    if (length < SYNC_LENGTH)
    {
        ERROR("short Sync message\r\n");
        toState(PTP_FAULTY, rtOpts, ptpClock);
        return;
    }

    switch (ptpClock->portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:

        DBGV("HandleSync : disreguard \r\n");
        return;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        if (isFromSelf)
        {
            DBGV("HandleSync: Ignore message from self \r\n");
            return;
        }

        isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);

        if (isFromCurrentParent) //来至于已注册的时钟源
        {

            ptpClock->sync_receive_time.seconds = time->seconds;
            ptpClock->sync_receive_time.nanoseconds = time->nanoseconds;

            if ((header->flagField[0] & 0x02) == TWO_STEP_FLAG)
            {
                ptpClock->waitingForFollow = TRUE;
                ptpClock->recvSyncSequenceId = header->sequenceId;
                /*Save correctionField of Sync message*/
                integer64_to_internalTime(header->correctionfield, &correctionField);
                ptpClock->lastSyncCorrectionField.seconds = correctionField.seconds;
                ptpClock->lastSyncCorrectionField.nanoseconds = correctionField.nanoseconds;
                break;
            }
            else
            {
                msgUnpackSync(ptpClock->msgIbuf, &ptpClock->msgTmp.sync);
                integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield, &correctionField);
                ptpClock->waitingForFollow = FALSE;
                toInternalTime(&OriginTimestamp, &ptpClock->msgTmp.sync.originTimestamp);

                updateOffset(&OriginTimestamp, &ptpClock->sync_receive_time, &ptpClock->ofm_filt, rtOpts, ptpClock, &correctionField);
                updateClock(rtOpts, ptpClock);
                break;
            }
        }
        break;

    case PTP_MASTER:
    default:
        if (!isFromSelf)
        {
            DBGV("HandleSync: Sync message received from another Master  \r\n");
            break;
        }

        else
        {
            //                ptp_time_get(&synctime);
            /*Add latency*/
            addTime(time, time, &rtOpts->outboundLatency);

            issueFollowup(time, rtOpts, ptpClock);
            break;
        }
    }
}

void handleFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    DBGV("Handlefollowup : Follow up message received \r\n");

    TimeInternal preciseOriginTimestamp;
    TimeInternal correctionField;
    Boolean isFromCurrentParent = FALSE;

    if (length < FOLLOW_UP_LENGTH)
    {
        ERROR("short Follow up message\n");
        toState(PTP_FAULTY, rtOpts, ptpClock);
        return;
    }

    if (isFromSelf)
    {
        DBGV("Handlefollowup : Ignore message from self \r\n");
        return;
    }

    switch (ptpClock->portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:
    case PTP_LISTENING:

        DBGV("Handfollowup : disreguard \r\n");
        return;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);

        if (isFromCurrentParent)
        {
            if (ptpClock->waitingForFollow)
            {
                if ((ptpClock->recvSyncSequenceId == header->sequenceId))
                {
                    //preciseOriginTimestamp is send time   t1  在follow报文里
                    //sync_receive_time is recv time        t2  在sync报文里
                    //correctionField   修正域,各个报文都有，主要在sync报文中，用于补偿网络中的传输延迟
                    msgUnpackFollowUp(ptpClock->msgIbuf, &ptpClock->msgTmp.follow);
                    ptpClock->waitingForFollow = FALSE;
                    toInternalTime(&preciseOriginTimestamp, &ptpClock->msgTmp.follow.preciseOriginTimestamp);
                    integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield, &correctionField);
                    addTime(&correctionField, &correctionField, &ptpClock->lastSyncCorrectionField);

                    //rt_kprintf("********follow time.seconds =%d,  nanoseconds=%d\r\n",preciseOriginTimestamp.seconds,preciseOriginTimestamp.nanoseconds);

                    updateOffset(&preciseOriginTimestamp, &ptpClock->sync_receive_time, &ptpClock->ofm_filt, rtOpts, ptpClock, &correctionField);

                    updateClock(rtOpts, ptpClock);
                    break;
                }
                else
                {
                    DBGV("SequenceID doesn't match with last Sync message \r\n");
                }
            }
            else
            {
                DBGV("Slave was not waiting a follow up message \r\n");
            }
        }
        else
        {
            DBGV("Follow up message is not from current parent \r\n");
        }

    case PTP_MASTER:
        DBGV("Follow up message received from another master \r\n");
        break;

    default:
        DBG("do unrecognized state\r\n");
        break;
    } //Switch on (port_state)
}

void handleDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, TimeInternal *time, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    if (rtOpts->E2E_mode)
    {
        DBGV("delayReq message received : \r\n");

        if (length < DELAY_REQ_LENGTH)
        {
            ERROR("short DelayReq message\r\n");
            toState(PTP_FAULTY, rtOpts, ptpClock);
            return;
        }

        switch (ptpClock->portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
            DBGV("HandledelayReq : disreguard \r\n");
            return;

        case PTP_SLAVE:

            if (isFromSelf)
            {
                /* Get sending timestamp from IP stack with So_TIMESTAMP*/
                ptpClock->delay_req_send_time.seconds = time->seconds;
                ptpClock->delay_req_send_time.nanoseconds = time->nanoseconds;

                /*Add latency*/
                addTime(&ptpClock->delay_req_send_time, &ptpClock->delay_req_send_time, &rtOpts->outboundLatency);
                break;
            }

            break;

        case PTP_MASTER:

            msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->delayReqHeader);
            issueDelayResp(time, &ptpClock->delayReqHeader, rtOpts, ptpClock);
            break;

        default:
            DBG("do unrecognized state\r\n");
            break;
        }
    }

    else //(Peer to Peer mode)
    {
        ERROR("Delay messages are disreguard in Peer to Peer mode \r\n");
    }
}

void handleDelayResp(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    if (rtOpts->E2E_mode)
    {

        Boolean isFromCurrentParent = FALSE;
        Boolean isCurrentRequest = FALSE;
        TimeInternal requestReceiptTimestamp;
        TimeInternal correctionField;

        DBGV("delayResp message received : \r\n");

        if (length < DELAY_RESP_LENGTH)
        {
            ERROR("short DelayResp message\r\n");
            toState(PTP_FAULTY, rtOpts, ptpClock);
            return;
        }

        switch (ptpClock->portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:

            DBGV("HandledelayResp : disreguard \r\n");
            return;

        case PTP_SLAVE:
            msgUnpackDelayResp(ptpClock->msgIbuf, &ptpClock->msgTmp.resp);

            isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);
            isCurrentRequest = !memcmp(ptpClock->portIdentity.clockIdentity, ptpClock->msgTmp.resp.requestingPortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->portIdentity.portNumber == ptpClock->msgTmp.resp.requestingPortIdentity.portNumber);

            if (((ptpClock->sentDelayReqSequenceId - 1) == header->sequenceId) && isCurrentRequest && isFromCurrentParent)
            {
                toInternalTime(&requestReceiptTimestamp, &ptpClock->msgTmp.resp.receiveTimestamp);
                ptpClock->delay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
                ptpClock->delay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;

                integer64_to_internalTime(header->correctionfield, &correctionField);
                updateDelay(&ptpClock->owd_filt, rtOpts, ptpClock, &correctionField);

                ptpClock->logMinDelayReqInterval = header->logMessageInterval;
            }

            else
            {
                DBGV("HandledelayResp : delayResp doesn't match with the delayReq. \r\n");
                break;
            }
        }
    }

    else //(Peer to Peer mode)
    {
        ERROR("Delay messages are disreguard in Peer to Peer mode \r\n");
    }
}

void handlePDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, TimeInternal *time, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    if (!rtOpts->E2E_mode)
    {

        DBGV("PdelayReq message received : \n");

        if (length < PDELAY_REQ_LENGTH)
        {
            ERROR("short PDelayReq message\n");
            toState(PTP_FAULTY, rtOpts, ptpClock);
            return;
        }

        switch (ptpClock->portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
            DBGV("HandlePdelayReq : disreguard \n");
            return;

        case PTP_SLAVE:
        case PTP_MASTER:
        case PTP_PASSIVE:

            if (isFromSelf)
            {
                /* Get sending timestamp from IP stack with So_TIMESTAMP*/
                ptpClock->pdelay_req_send_time.seconds = time->seconds;
                ptpClock->pdelay_req_send_time.nanoseconds = time->nanoseconds;

                /*Add latency*/
                addTime(&ptpClock->pdelay_req_send_time, &ptpClock->pdelay_req_send_time, &rtOpts->outboundLatency);
                break;
            }
            else
            {
                msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->PdelayReqHeader);
                issuePDelayResp(time, header, rtOpts, ptpClock);
                break;
            }

        default:
            DBG("do unrecognized state\n");
            break;
        }
    }

    else //(End to End mode..)
    {
        ERROR("Peer Delay messages are disreguard in End to End mode \n");
    }
}

void handlePDelayResp(MsgHeader *header, Octet *msgIbuf, TimeInternal *time, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    if (!rtOpts->E2E_mode)
    {
        // Boolean isFromCurrentParent = FALSE;
        TimeInternal requestReceiptTimestamp;
        TimeInternal correctionField;

        DBGV("PdelayResp message received : \n");

        if (length < PDELAY_RESP_LENGTH)
        {
            ERROR("short PDelayResp message\n");
            toState(PTP_FAULTY, rtOpts, ptpClock);
            return;
        }

        switch (ptpClock->portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:

            DBGV("HandlePdelayResp : disreguard \n");
            return;

        case PTP_SLAVE:

            if (isFromSelf)
            {
                addTime(time, time, &rtOpts->outboundLatency);
                issuePDelayRespFollowUp(time, &ptpClock->PdelayReqHeader, rtOpts, ptpClock);
                break;
            }

            msgUnpackPDelayResp(ptpClock->msgIbuf, &ptpClock->msgTmp.presp);

            // isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);

            if (!((ptpClock->sentPDelayReqSequenceId == header->sequenceId) && (!memcmp(ptpClock->portIdentity.clockIdentity, ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH)) && (ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber)))

            {
                if ((header->flagField[0] & 0x02) == TWO_STEP_FLAG)
                {
                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
                    ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
                    //

                    /*store t2 (Fig 35)*/
                    toInternalTime(&requestReceiptTimestamp, &ptpClock->msgTmp.presp.requestReceiptTimestamp);
                    ptpClock->pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
                    ptpClock->pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;

                    integer64_to_internalTime(header->correctionfield, &correctionField);
                    ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
                    ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
                    //
                    break;
                } //Two Step Clock

                else //One step Clock
                {
                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
                    ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;

                    integer64_to_internalTime(header->correctionfield, &correctionField);
                    updatePeerDelay(&ptpClock->owd_filt, rtOpts, ptpClock, &correctionField, FALSE);

                    break;
                }
            }
            else
            {
                DBGV("HandlePdelayResp : Pdelayresp doesn't match with the PdelayReq. \n");
                break;
            }

        case PTP_MASTER:
            /*Loopback Timestamp*/
            if (isFromSelf)
            {
                /*Add latency*/
                addTime(time, time, &rtOpts->outboundLatency);

                issuePDelayRespFollowUp(time, &ptpClock->PdelayReqHeader, rtOpts, ptpClock);
                break;
            }

            msgUnpackPDelayResp(ptpClock->msgIbuf, &ptpClock->msgTmp.presp);

            // isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);

            if (!((ptpClock->sentPDelayReqSequenceId == header->sequenceId) && (!memcmp(ptpClock->portIdentity.clockIdentity, ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH)) && (ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber)))

            {
                if ((header->flagField[0] & 0x02) == TWO_STEP_FLAG)
                {
                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
                    ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;

                    /*store t2 (Fig 35)*/
                    toInternalTime(&requestReceiptTimestamp, &ptpClock->msgTmp.presp.requestReceiptTimestamp);
                    ptpClock->pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
                    ptpClock->pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;

                    integer64_to_internalTime(header->correctionfield, &correctionField);
                    ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
                    ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
                    break;
                } //Two Step Clock

                else //One step Clock
                {
                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
                    ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;

                    integer64_to_internalTime(header->correctionfield, &correctionField);
                    updatePeerDelay(&ptpClock->owd_filt, rtOpts, ptpClock, &correctionField, FALSE);
                    break;
                }
            }
        default:
            DBG("do unrecognized state\n");
            break;
        }
    }

    else //(End to End mode..)
    {
        ERROR("Peer Delay messages are disreguard in End to End mode \n");
    }
}

void handlePDelayRespFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{

    if (!rtOpts->E2E_mode)
    {
        // Boolean isFromCurrentParent = FALSE;
        TimeInternal responseOriginTimestamp;
        TimeInternal correctionField;

        DBGV("PdelayRespfollowup message received : \n");

        if (length < PDELAY_RESP_FOLLOW_UP_LENGTH)
        {
            ERROR("short PDelayRespfollowup message\n");
            toState(PTP_FAULTY, rtOpts, ptpClock);
            return;
        }

        switch (ptpClock->portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
            DBGV("HandlePdelayResp : disreguard \n");
            return;

        case PTP_SLAVE:

            if (header->sequenceId == ptpClock->sentPDelayReqSequenceId - 1)
            {
                msgUnpackPDelayRespFollowUp(ptpClock->msgIbuf, &ptpClock->msgTmp.prespfollow);
                toInternalTime(&responseOriginTimestamp, &ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
                ptpClock->pdelay_resp_send_time.seconds = responseOriginTimestamp.seconds;
                ptpClock->pdelay_resp_send_time.nanoseconds = responseOriginTimestamp.nanoseconds;
                integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield, &correctionField);
                addTime(&correctionField, &correctionField, &ptpClock->lastPdelayRespCorrectionField);

                updatePeerDelay(&ptpClock->owd_filt, rtOpts, ptpClock, &correctionField, TRUE);
                break;
            }

        case PTP_MASTER:

            if (header->sequenceId == ptpClock->sentPDelayReqSequenceId - 1)
            {
                msgUnpackPDelayRespFollowUp(ptpClock->msgIbuf, &ptpClock->msgTmp.prespfollow);
                toInternalTime(&responseOriginTimestamp, &ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
                ptpClock->pdelay_resp_send_time.seconds = responseOriginTimestamp.seconds;
                ptpClock->pdelay_resp_send_time.nanoseconds = responseOriginTimestamp.nanoseconds;
                integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield, &correctionField);
                addTime(&correctionField, &correctionField, &ptpClock->lastPdelayRespCorrectionField);

                updatePeerDelay(&ptpClock->owd_filt, rtOpts, ptpClock, &correctionField, TRUE);
                break;
            }

        default:
            DBGV("Disregard PdelayRespFollowUp message  \n");
        }
    }

    else //(End to End mode..)
    {
        ERROR("Peer Delay messages are disreguard in End to End mode \n");
    }
}

void handleManagement(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock) {}
void handleSignaling(MsgHeader *header, Octet *msgIbuf, ssize_t length, Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock) {}

/*Pack and send on general multicast ip adress an Announce message*/
void issueAnnounce(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    msgPackAnnounce(ptpClock->msgObuf, ptpClock);

    if (!netSendGeneral(ptpClock->msgObuf, ANNOUNCE_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("Announce message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("Announce MSG sent ! \n");
        ptpClock->sentAnnounceSequenceId++;
    }
}

/*Pack and send on event multicast ip adress a Sync message*/
void issueSync(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;
    s_systime timetemp = {0, 0};
    // static uint32_t printcnt = 0;

    sys_time_get(&timetemp);
    easy_ptp_set_time(); //send ptp time to ap when it's in master mode

    internalTime.seconds = timetemp.sec;
    internalTime.nanoseconds = timetemp.usec * 1000;

    // printcnt++;

    // if (printcnt % 10 == 0)
    // {
    //     rt_kprintf("****ptp master--send sync time=%d.%d\r\n", internalTime.seconds, internalTime.nanoseconds);
    // }

    fromInternalTime(&internalTime, &originTimestamp);

    msgPackSync(ptpClock->msgObuf, &originTimestamp, ptpClock);

    if (!netSendEvent(ptpClock->msgObuf, SYNC_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("Sync message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("Sync MSG sent ! \n");
        ptpClock->sentSyncSequenceId++;
    }
}

/*Pack and send on general multicast ip adress a FollowUp message*/
void issueFollowup(TimeInternal *time, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp preciseOriginTimestamp;

    fromInternalTime(time, &preciseOriginTimestamp);

    msgPackFollowUp(ptpClock->msgObuf, &preciseOriginTimestamp, ptpClock);

    if (!netSendGeneral(ptpClock->msgObuf, FOLLOW_UP_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("FollowUp message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("FOllowUp MSG sent ! \n");
    }
}

/*Pack and send on event multicast ip adress a DelayReq message*/
void issueDelayReq(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;
    s_systime timetemp = {0, 0};

    sys_time_get(&timetemp);
    internalTime.seconds = timetemp.sec;
    internalTime.nanoseconds = timetemp.usec * 1000;

    fromInternalTime(&internalTime, &originTimestamp);

    msgPackDelayReq(ptpClock->msgObuf, &originTimestamp, ptpClock);

    if (!netSendEvent(ptpClock->msgObuf, DELAY_REQ_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("delayReq message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("DelayReq MSG sent ! \n");
        ptpClock->sentDelayReqSequenceId++;

        if (internalTime.seconds != 0)
        {
            addTime(&internalTime, &internalTime, &rtOpts->outboundLatency);
            ptpClock->delay_req_send_time = internalTime;
        }
    }
}

/*Pack and send on event multicast ip adress a PDelayReq message*/
void issuePDelayReq(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;
    s_systime timetemp = {0, 0};

    sys_time_get(&timetemp);

    internalTime.seconds = timetemp.sec;
    internalTime.nanoseconds = timetemp.usec * 1000;

    fromInternalTime(&internalTime, &originTimestamp);

    msgPackPDelayReq(ptpClock->msgObuf, &originTimestamp, ptpClock);

    if (!netSendEvent(ptpClock->msgObuf, PDELAY_REQ_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("PdelayReq message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("PDelayReq MSG sent ! \n");
        ptpClock->sentPDelayReqSequenceId++;
    }
}

/*Pack and send on event multicast ip adress a PDelayResp message*/
void issuePDelayResp(TimeInternal *time, MsgHeader *header, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp requestReceiptTimestamp;
    fromInternalTime(time, &requestReceiptTimestamp);
    msgPackPDelayResp(ptpClock->msgObuf, header, &requestReceiptTimestamp, ptpClock);

    if (!netSendEvent(ptpClock->msgObuf, PDELAY_RESP_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("PdelayResp message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("PDelayResp MSG sent ! \n");
    }
}

/*Pack and send on event multicast ip adress a DelayResp message*/
void issueDelayResp(TimeInternal *time, MsgHeader *header, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp requestReceiptTimestamp;
    fromInternalTime(time, &requestReceiptTimestamp);
    msgPackDelayResp(ptpClock->msgObuf, header, &requestReceiptTimestamp, ptpClock);

    if (!netSendGeneral(ptpClock->msgObuf, PDELAY_RESP_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("delayResp message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("DelayResp MSG sent ! \n");
    }
}

void issuePDelayRespFollowUp(TimeInternal *time, MsgHeader *header, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    Timestamp responseOriginTimestamp;
    fromInternalTime(time, &responseOriginTimestamp);

    msgPackPDelayRespFollowUp(ptpClock->msgObuf, header, &responseOriginTimestamp, ptpClock);

    if (!netSendGeneral(ptpClock->msgObuf, PDELAY_RESP_FOLLOW_UP_LENGTH, &ptpClock->netPath))
    {
        toState(PTP_FAULTY, rtOpts, ptpClock);
        DBGV("PdelayRespFollowUp message can't be sent -> FAULTY state \n");
    }
    else
    {
        DBGV("PDelayRespFollowUp MSG sent ! \n");
    }
}

void issueManagement(MsgHeader *header, MsgManagement *manage, RunTimeOpts *rtOpts, PtpClock *ptpClock) {}

void addForeign(Octet *buf, MsgHeader *header, PtpClock *ptpClock)
{
    int i, j;
    Boolean found = FALSE;

    j = ptpClock->foreign_record_best;

    /*Check if Foreign master is already known*/
    for (i = 0; i < ptpClock->number_foreign_records; i++)
    {
        if (!memcmp(header->sourcePortIdentity.clockIdentity, ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH) && (header->sourcePortIdentity.portNumber == ptpClock->foreign[j].foreignMasterPortIdentity.portNumber))
        {
            /*Foreign Master is already in Foreignmaster data set*/
            ptpClock->foreign[j].foreignMasterAnnounceMessages++;
            found = TRUE;
            DBGV("addForeign : AnnounceMessage incremented \n");
            msgUnpackHeader(buf, &ptpClock->foreign[j].header);
            msgUnpackAnnounce(buf, &ptpClock->foreign[j].announce);
            break;
        }

        j = (j + 1) % ptpClock->number_foreign_records;
    }

    /*New Foreign Master*/
    if (!found)
    {
        if (ptpClock->number_foreign_records < ptpClock->max_foreign_records)
        {
            ptpClock->number_foreign_records++;
        }
        j = ptpClock->foreign_record_i;

        /*Copy new foreign master data set from Announce message*/
        memcpy(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity, header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
        ptpClock->foreign[j].foreignMasterPortIdentity.portNumber = header->sourcePortIdentity.portNumber;
        ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;

        /*header and announce field of each Foreign Master are usefull to run Best Master Clock Algorithm*/
        msgUnpackHeader(buf, &ptpClock->foreign[j].header);
        msgUnpackAnnounce(buf, &ptpClock->foreign[j].announce);
        DBGV("New foreign Master added \n");

        ptpClock->foreign_record_i = (ptpClock->foreign_record_i + 1) % ptpClock->max_foreign_records;
    }
}
