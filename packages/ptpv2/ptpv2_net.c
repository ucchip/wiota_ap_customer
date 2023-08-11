/* net.c */
#include "ptpv2_main.h"
#include <wizchip_socket.h>
#include "wiz_socket.h"
#include "wizchip_conf.h"

/* shut down the UDP stuff */
Boolean netShutdown(NetPath *netPath)
{
    struct ip_mreq imr;
    // int8_t socket = -1;

    /*Close General Multicast*/
    imr.imr_multiaddr.s_addr = netPath->multicastAddr;
    imr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (netPath->eventSock >= 0)
    {
        setsockopt(netPath->eventSock, SOL_SOCKET, IP_DROP_MEMBERSHIP, &imr, sizeof(struct ip_mreq));
        closesocket(netPath->eventSock);
    }
    if (netPath->generalSock >= 0)
    {
        setsockopt(netPath->generalSock, SOL_SOCKET, IP_DROP_MEMBERSHIP, &imr, sizeof(struct ip_mreq));
        closesocket(netPath->generalSock);
    }

    netPath->multicastAddr = 0;
    netPath->unicastAddr = 0;
    netPath->peerMulticastAddr = 0;

    netPath->eventSock = -1;
    netPath->generalSock = -1;

    return TRUE;
}

/*Test if network layer is OK for PTP*/
UInteger8 lookupCommunicationTechnology(UInteger8 communicationTechnology)
{
#if defined(linux)

    switch (communicationTechnology)
    {
    case ARPHRD_ETHER:
    case ARPHRD_EETHER:
    case ARPHRD_IEEE802:
        return PTP_ETHER;

    default:
        break;
    }

#elif defined(BSD_INTERFACE_FUNCTIONS)

#endif

    return PTP_DEFAULT;
}

#define in_range(c, lo, up) ((uint8_t)c >= lo && (uint8_t)c <= up)
#define isdigit(c) in_range(c, '0', '9')
#define isspace(c) (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

static uint32_t ipstr_atol(const char *nptr)
{
    uint32_t total = 0;
    char sign = '+';
    /* jump space */
    while (isspace(*nptr))
    {
        ++nptr;
    }

    if (*nptr == '-' || *nptr == '+')
    {
        sign = *nptr++;
    }

    while (isdigit(*nptr))
    {
        total = 10 * total + ((*nptr++) - '0');
    }
    return (sign == '-') ? -total : total;
}

/* IP address to unsigned int type */
static uint32_t ipstr_to_u32(char *ipstr)
{
    char ipBytes[4] = {0};
    uint32_t i;
    uint32_t ip = 0;

    for (i = 0; i < 4; i++, ipstr++)
    {
        ipBytes[i] = (char)ipstr_atol(ipstr);
        if ((ipstr = strchr(ipstr, '.')) == RT_NULL)
        {
            break;
        }
    }
    rt_memcpy(&ip, ipBytes, 4);
    return ip;
}

/* Find interface to  be used.  uuid should be filled with MAC address of the interface.
      Will return the IPv4 address of  the interface. */
UInteger32 findIface(Octet *ifaceName, UInteger8 *communicationTechnology, Octet *uuid, NetPath *netPath)
{
    wiz_NetInfo netinfo = {RT_NULL};

    // struct sockaddr_in send_addr;
    struct in_addr netAddr;
    char addrStr[NET_ADDRESS_LENGTH] = {0};
    // UInteger32 test = 0;

    wiz_get_net_info(&netinfo);

    rt_snprintf(addrStr, NET_ADDRESS_LENGTH, "%u.%u.%u.%u", netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);

    // test =
    ipstr_to_u32(addrStr);

    inet_aton(addrStr, &netAddr);
    // send_addr.sin_addr.s_addr = netAddr.s_addr;

    memcpy(uuid, netinfo.mac, PTP_UUID_LENGTH);

    return (UInteger32)netAddr.s_addr;
}

/* start all of the UDP stuff */
/* must specify 'subdomainName', optionally 'ifaceName', if not then pass ifaceName == "" */
/* returns other args */
/* on socket options, see the 'socket(7)' and 'ip' man pages */
Boolean netInit(NetPath *netPath, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    // int temp;
    struct in_addr interfaceAddr, netAddr;
    struct sockaddr_in addr;
    // struct ip_mreq imr;
    char addrStr[NET_ADDRESS_LENGTH];
    struct timeval timeout;
    rt_uint8_t DEFAULT_PTP_DOMAIN_ADDR[4] = {224, 0, 1, 129};
    // rt_uint8_t PEER_PTP_DOMAIN_ADDR[4] = {224, 0, 0, 107};
    rt_uint8_t DHAR[6] = {0x01, 0x00, 0x5e, 0x00, 0x01, 0x81};

    DBG("netInit\n");

    /**************************************step1****************************/
    //find local ip and mac addr

    /* find a network interface */
    if (!(interfaceAddr.s_addr = findIface(rtOpts->ifaceName, &ptpClock->port_communication_technology, ptpClock->port_uuid_field, netPath)))
    {
        return FALSE;
    }

    rt_kprintf("findiface uuid=%02x-%02x-%02x-%02x-%02x-%02x\r\n", ptpClock->port_uuid_field[0], ptpClock->port_uuid_field[1], ptpClock->port_uuid_field[2], ptpClock->port_uuid_field[3], ptpClock->port_uuid_field[4], ptpClock->port_uuid_field[5]);
    rt_kprintf("Local IP address used : %s \r\n", inet_ntoa(interfaceAddr.s_addr));

    /**************************************step2****************************/
    // create event sockets
    if ((netPath->eventSock = wiz_socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        PERROR("failed to initalize event sockets\r\n");
        return FALSE;
    }

    // create general sockets
    if ((netPath->generalSock = wiz_socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        PERROR("failed to initalize general sockets\r\n");
        return FALSE;
    }

    //SO_REUSEADDR,主要用于配置peer数据包，暂时没用到peer,没有设置reuse
    //允许服务器bind一个地址，即使这个地址当前已经存在已建立的连接
    //两个socket绑定在同一个端口，加入不同的组播组，最终导致两个socket能够收到两个不同组播组发送的数据。

    /**************************************step3****************************/
    //Establish the appropriate UDP bindings/connections for events

    /* need INADDR_ANY to allow receipt of multi-cast and uni-cast messages */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PTP_GENERAL_PORT);

    rt_memset(&(addr.sin_zero), 0, sizeof(addr.sin_zero)); // 这个字段只起填充作用，使得 sockaddr_in 与 sockaddr 长度一样

    if (wiz_bind(netPath->generalSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        PERROR("failed to bind event socket\r\n");
        return FALSE;
    }

    addr.sin_port = htons(PTP_EVENT_PORT);
    if (wiz_bind(netPath->eventSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        PERROR("failed to bind event socket\r\n");
        return FALSE;
    }

    /**************************************step4****************************/
    // Init General multicast IP address

    memcpy(addrStr, DEFAULT_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);

    if (!inet_aton(addrStr, &netAddr))
    {
        PERROR("netInit: failed to encode multi-cast address: %s\n", addrStr);

        return FALSE;
    }
    netPath->multicastAddr = netAddr.s_addr;

    memcpy(addrStr, PEER_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);

    if (!inet_aton(addrStr, &netAddr))
    {
        PERROR("netInit: failed to encode multi-cast address: %s\n", addrStr);

        return FALSE;
    }
    netPath->peerMulticastAddr = netAddr.s_addr;

    /**************************************step3****************************/
    //disable unicast

    netPath->unicastAddr = 0;

    /**************************************step5****************************/
    /* Join multicast group (for receiving) on specified interface */

    setSn_CR(netPath->eventSock, Sn_CR_CLOSE);
    setSn_TTL(netPath->eventSock, 128); //需在open之前设置
    setSn_DHAR(netPath->eventSock, DHAR);
    setSn_DIPR(netPath->eventSock, DEFAULT_PTP_DOMAIN_ADDR); //224.0.1.129
    setSn_DPORT(netPath->eventSock, PTP_EVENT_PORT);         //319
    setSn_MR(netPath->eventSock, Sn_MR_UDP | Sn_MR_MULTI);
    setSn_CR(netPath->eventSock, Sn_CR_OPEN);

    setSn_CR(netPath->generalSock, Sn_CR_CLOSE);
    setSn_TTL(netPath->generalSock, 128); //需在open之前设置
    setSn_DHAR(netPath->generalSock, DHAR);
    setSn_DIPR(netPath->generalSock, DEFAULT_PTP_DOMAIN_ADDR); //224.0.1.129
    setSn_DPORT(netPath->generalSock, PTP_GENERAL_PORT);       //320
    setSn_MR(netPath->generalSock, Sn_MR_UDP | Sn_MR_MULTI);
    setSn_CR(netPath->generalSock, Sn_CR_OPEN);

    /**************************************step6****************************/
    /* set socket time-to-live to 1 */ //socket ttl
                                       //    if( setsockopt(netPath->eventSock, SOL_SOCKET, IP_MULTICAST_TTL, &temp, sizeof(int)) < 0
                                       //    || setsockopt(netPath->generalSock, SOL_SOCKET, IP_MULTICAST_TTL, &temp, sizeof(int)) < 0 )
                                       //    {
                                       //        PERROR("failed to set the multi-cast time-to-live\r\n");
                                       //        return FALSE;
                                       //    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    /* set receive and send timeout */
    setsockopt(netPath->eventSock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
    setsockopt(netPath->generalSock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
    setsockopt(netPath->eventSock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));
    setsockopt(netPath->generalSock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));

    return TRUE;
}

/*store received data from network to "buf" , get and store the SO_TIMESTAMP value in "time" for an event message*/
ssize_t netRecvEvent(Octet *buf, TimeInternal *time, NetPath *netPath)
{
    // ssize_t ret = 0;
    socklen_t fromlen;
    struct sockaddr_in from;
    int inlen;
    // struct timeval *tv;
    // char from_ip[20] = {0};

    inlen = recvfrom(netPath->eventSock, buf, PACKET_SIZE, 0, (struct sockaddr *)&from, &fromlen);

    if (inlen > 0)
    {
        return inlen;
    }
    else
    {
        return 0;
    }
}

/*store received data from network to "buf" get and store the SO_TIMESTAMP value in "time" for a general message*/
ssize_t netRecvGeneral(Octet *buf, TimeInternal *time, NetPath *netPath)
{
    // ssize_t ret = 0;
    socklen_t fromlen;
    struct sockaddr_in from;
    int inlen;
    // struct timeval *tv;

    inlen = recvfrom(netPath->generalSock, buf, PACKET_SIZE, 0, (struct sockaddr *)&from, &fromlen);

    if (inlen > 0)
    {
        return inlen;
    }
    else
    {
        return 0;
    }
}

ssize_t netSendEvent(Octet *buf, UInteger16 length, NetPath *netPath)
{
    ssize_t ret;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PTP_EVENT_PORT);
    addr.sin_addr.s_addr = netPath->multicastAddr;

    ret = wizchip_sendto(netPath->eventSock, (uint8_t *)buf, length, (uint8_t *)&addr, sizeof(struct sockaddr_in));
    if (ret <= 0)
        DBG("error sending multi-cast event message\n");

    if (netPath->unicastAddr)
    {
        addr.sin_addr.s_addr = netPath->unicastAddr;

        ret = wizchip_sendto(netPath->eventSock, (uint8_t *)buf, length, (uint8_t *)&addr, sizeof(struct sockaddr_in));
        if (ret <= 0)
            DBG("error sending uni-cast event message\n");
    }

    return ret;
}

ssize_t netSendGeneral(Octet *buf, UInteger16 length, NetPath *netPath)
{
    ssize_t ret;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PTP_GENERAL_PORT);
    addr.sin_addr.s_addr = netPath->multicastAddr;

    ret = wizchip_sendto(netPath->generalSock, (uint8_t *)buf, length, (uint8_t *)&addr, sizeof(struct sockaddr_in));
    if (ret <= 0)
        DBG("error sending multi-cast general message\n");

    if (netPath->unicastAddr)
    {
        addr.sin_addr.s_addr = netPath->unicastAddr;

        ret = wizchip_sendto(netPath->generalSock, (uint8_t *)buf, length, (uint8_t *)&addr, sizeof(struct sockaddr_in));
        if (ret <= 0)
            DBG("error sending uni-cast general message\n");
    }

    return ret;
}
