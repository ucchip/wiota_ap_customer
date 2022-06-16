/*
 * File      : at_socket_ml302.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-12-07     liang.shao     multi AT socket client support
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <at_device_ml302.h>

#if !defined(AT_SW_VERSION_NUM) || AT_SW_VERSION_NUM < 0x10300
#error "This AT Client version is older, please check and update latest AT Client!"
#endif

#define LOG_TAG "at.dev"
#include <at_log.h>

#ifdef AT_DEVICE_USING_ML302

#define ML302_WAIT_CONNECT_TIME 30000
#define ML302_THREAD_STACK_SIZE 2048
#define ML302_THREAD_PRIORITY (RT_THREAD_PRIORITY_MAX / 2)
// #define ML302_THREAD_PRIORITY 6

static void ml302_power_on(struct at_device* device)
{
    struct at_device_ml302* ml302 = RT_NULL;

    ml302 = (struct at_device_ml302*)device->user_data;

    /* not nead to set pin configuration for ml302 device power on */
    if (ml302->power_pin == -1)
    {
        return;
    }
    if (ml302->power_en_pin != -1)
    {
        rt_pin_write(ml302->power_en_pin, PIN_LOW);
        rt_thread_mdelay(2000);
        rt_pin_write(ml302->power_en_pin, PIN_HIGH);
        rt_thread_mdelay(1000);
    }
    rt_pin_write(ml302->power_pin, PIN_LOW);
    rt_thread_mdelay(2000);
    rt_pin_write(ml302->power_pin, PIN_HIGH);
}

static void ml302_power_off(struct at_device* device)
{
    struct at_device_ml302* ml302 = RT_NULL;

    ml302 = (struct at_device_ml302*)device->user_data;

    /* not nead to set pin configuration for ml302 device power on */
    if (ml302->power_pin == -1)
    {
        return;
    }
    if (ml302->power_en_pin != -1)
    {
        rt_pin_write(ml302->power_en_pin, PIN_LOW);
    }
    else
    {
        rt_pin_write(ml302->power_pin, PIN_LOW);
        rt_thread_mdelay(2000);
        rt_pin_write(ml302->power_pin, PIN_HIGH);
    }
}

/* =============================  sim76xx network interface operations ============================= */

/* set ml302 network interface device status and address information */
static int ml302_netdev_set_info(struct netdev* netdev)
{
#define ml302_IEMI_RESP_SIZE 64
#define ml302_IPADDR_RESP_SIZE 96
#define ml302_DNS_RESP_SIZE 96
#define ml302_INFO_RESP_TIMO rt_tick_from_millisecond(300)

    int result = RT_EOK;
    ip_addr_t addr;
    at_response_t resp = RT_NULL;
    struct at_device* device = RT_NULL;

    if (netdev == RT_NULL)
    {
        LOG_E("input network interface device is NULL.");
        return -RT_ERROR;
    }

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return -RT_ERROR;
    }

    /* set network interface device status */
    netdev_low_level_set_status(netdev, RT_TRUE);
    netdev_low_level_set_link_status(netdev, RT_TRUE);
    netdev_low_level_set_dhcp_status(netdev, RT_TRUE);

    resp = at_create_resp(ml302_IEMI_RESP_SIZE, 4, ml302_INFO_RESP_TIMO);
    if (resp == RT_NULL)
    {
        LOG_E("ml302 device(%s) set IP address failed, no memory for response object.", device->name);
        result = -RT_ENOMEM;
        goto __exit;
    }

    /* set network interface device hardware address(IMEI) */
    {
#define ml302_NETDEV_HWADDR_LEN 8
#define ml302_IEMI_LEN 15

        char imei[ml302_IEMI_LEN + 1] = {0};
        int i = 0, j = 0;

        /* send "AT+CGSN=1" commond to get device IMEI */
        if (at_obj_exec_cmd(device->client, resp, "AT+CGSN=1") < 0)
        {
            result = -RT_ERROR;
            goto __exit;
        }

        if (at_resp_parse_line_args_by_kw(resp, "+CGSN:", "+CGSN: %s", imei) <= 0)
        {
            LOG_E("%s device prase \"AT+CGSN=1\" cmd error.", device->name);
            result = -RT_ERROR;
            goto __exit;
        }
        LOG_I("ml302 device(%s) IMEI: %s", device->name, imei);

        netdev->hwaddr_len = ml302_NETDEV_HWADDR_LEN;
        /* get hardware address by IEMI */
        for (i = 0, j = 0; i < ml302_NETDEV_HWADDR_LEN && j < ml302_IEMI_LEN; i++, j += 2)
        {
            if (j != ml302_IEMI_LEN - 1)
            {
                netdev->hwaddr[i] = (imei[j] - '0') * 10 + (imei[j + 1] - '0');
            }
            else
            {
                netdev->hwaddr[i] = (imei[j] - '0');
            }
        }
    }

    /* set network interface device IP address */
    {
#define IP_ADDR_SIZE_MAX 16
        char ipaddr[IP_ADDR_SIZE_MAX] = {0};

        at_resp_set_info(resp, ml302_IPADDR_RESP_SIZE, 0, ml302_INFO_RESP_TIMO);

        /* send "AT+CGPADDR" commond to get IP address */
        if (at_obj_exec_cmd(device->client, resp, "AT+CGPADDR=1") < 0)
        {
            result = -RT_ERROR;
            goto __exit;
        }

        if (at_resp_parse_line_args_by_kw(resp, "+CGPADDR:", "+CGPADDR: %*[^\"]\"%[^\"]", ipaddr) <= 0)
        {
            LOG_E("ml302 device(%s) prase \"AT+CGPADDR\" commands resposne data error!", device->name);
            result = -RT_ERROR;
            goto __exit;
        }

        LOG_I("ml302 device(%s) IP address: %s", device->name, ipaddr);

        /* set network interface address information */
        inet_aton(ipaddr, &addr);
        netdev_low_level_set_ipaddr(netdev, &addr);
    }

    /* set network interface device dns server */
    {
#define DNS_ADDR_SIZE_MAX 16
        char dns_server1[DNS_ADDR_SIZE_MAX] = {0}, dns_server2[DNS_ADDR_SIZE_MAX] = {0};

        at_resp_set_info(resp, ml302_DNS_RESP_SIZE, 0, ml302_INFO_RESP_TIMO);

        /* send "AT+MDNSCFG?" commond to get DNS servers address */
        if (at_obj_exec_cmd(device->client, resp, "AT+MDNSCFG?") < 0)
        {
            result = -RT_ERROR;
            goto __exit;
        }

        if (at_resp_parse_line_args_by_kw(resp, "+MDNSCFG:", "+MDNSCFG:%*[^:]:%[^ ]", dns_server1) <= 0 ||
            at_resp_parse_line_args_by_kw(resp, "+MDNSCFG:", "+MDNSCFG:%*[^:]:%*[^:]:%[^\r]", dns_server2) <= 0)
        {
            LOG_E("Prase \"AT+MDNSCFG?\" commands resposne data error!");
            result = -RT_ERROR;
            goto __exit;
        }

        LOG_I("ml302 device(%s) primary DNS server address: %s", device->name, dns_server1);
        LOG_I("ml302 device(%s) secondary DNS server address: %s", device->name, dns_server2);

        inet_aton(dns_server1, &addr);
        netdev_low_level_set_dns_server(netdev, 0, &addr);

        inet_aton(dns_server2, &addr);
        netdev_low_level_set_dns_server(netdev, 1, &addr);
    }

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_reboot(struct at_device* device);

static void check_link_status_entry(void* parameter)
{
#define ml302_LINK_STATUS_OK 1
#define ml302_LINK_RESP_SIZE 64
#define ml302_LINK_RESP_TIMO (3 * RT_TICK_PER_SECOND)
#define ml302_LINK_DELAY_TIME (30 * RT_TICK_PER_SECOND)

    at_response_t resp = RT_NULL;
    int result_code, link_status;
    struct at_device* device = RT_NULL;
    struct at_device_ml302* ml302 = RT_NULL;

    char parsed_data[10] = {0};
    struct netdev* netdev = (struct netdev*)parameter;

    LOG_D("statrt ml302 device(%s) link status check \n", netdev->name);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return;
    }
    ml302 = (struct at_device_ml302*)device->user_data;
    resp = at_create_resp(ml302_LINK_RESP_SIZE, 0, ml302_LINK_RESP_TIMO);
    if (resp == RT_NULL)
    {
        LOG_E("ml302 device(%s) set check link status failed, no memory for response object.", device->name);
        return;
    }

    while (1)
    {
        /* send "AT+CGREG?" commond  to check netweork interface device link status */
        if (at_obj_exec_cmd(device->client, resp, "AT+CGREG?") < 0)
        {
            rt_thread_mdelay(ml302_LINK_DELAY_TIME);
            LOG_E("ml302 device(%s) send cgreg failed", device->name);
            continue;
        }

        link_status = -1;
        at_resp_parse_line_args_by_kw(resp, "+CGREG:", "+CGREG: %d,%d", &result_code, &link_status);

        /* check the network interface device link status  */
        if ((ml302_LINK_STATUS_OK == link_status) != netdev_is_link_up(netdev))
        {
            netdev_low_level_set_link_status(netdev, (ml302_LINK_STATUS_OK == link_status));
        }

        if ((ml302->power_status_pin == -1) || (rt_pin_read(ml302->power_status_pin) == PIN_HIGH)) //check the module_status , if moduble_status is Low, user can do your logic here
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CSQ") == 0)
            {
                at_resp_parse_line_args_by_kw(resp, "+CSQ:", "+CSQ: %s", &parsed_data);
                if (strncmp(parsed_data, "99,99", sizeof(parsed_data)))
                {
                    LOG_D("ml302 device(%s) signal strength: %s", device->name, parsed_data);
                }
            }
        }
        else
        {
            //LTE down
            LOG_E("the lte pin is low");
            ml302_reboot(device);
        }

        rt_thread_mdelay(ml302_LINK_DELAY_TIME);
    }
}

static int ml302_netdev_check_link_status(struct netdev* netdev)
{

#define ml302_LINK_THREAD_TICK 20
#define ml302_LINK_THREAD_STACK_SIZE 1536
#define ml302_LINK_THREAD_PRIORITY (RT_THREAD_PRIORITY_MAX - 22)

    rt_thread_t tid;
    char tname[RT_NAME_MAX] = {0};

    if (netdev == RT_NULL)
    {
        LOG_E("input network interface device is NULL.\n");
        return -RT_ERROR;
    }

    rt_snprintf(tname, RT_NAME_MAX, "%s_link", netdev->name);

    tid = rt_thread_create(tname, check_link_status_entry, (void*)netdev,
                           ml302_LINK_THREAD_STACK_SIZE, ml302_LINK_THREAD_PRIORITY, ml302_LINK_THREAD_TICK);
    if (tid)
    {
        rt_thread_startup(tid);
    }

    return RT_EOK;
}

static int ml302_net_init(struct at_device* device);

static int ml302_netdev_set_up(struct netdev* netdev)
{
    struct at_device* device = RT_NULL;

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return -RT_ERROR;
    }

    if (device->is_init == RT_FALSE)
    {
        ml302_net_init(device);
        device->is_init = RT_TRUE;

        netdev_low_level_set_status(netdev, RT_TRUE);
        LOG_D("the network interface device(%s) set up status.", netdev->name);
    }

    return RT_EOK;
}

static int ml302_netdev_set_down(struct netdev* netdev)
{
    struct at_device* device = RT_NULL;

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return -RT_ERROR;
    }

    if (device->is_init == RT_TRUE)
    {
        ml302_power_off(device);
        device->is_init = RT_FALSE;

        netdev_low_level_set_status(netdev, RT_FALSE);
        LOG_D("the network interface device(%s) set down status.", netdev->name);
    }

    return RT_EOK;
}

static int ml302_netdev_set_dns_server(struct netdev* netdev, uint8_t dns_num, ip_addr_t* dns_server)
{
#define ml302_DNS_RESP_LEN 8
#define ml302_DNS_RESP_TIMEO rt_tick_from_millisecond(300)

    int result = RT_EOK;
    at_response_t resp = RT_NULL;
    struct at_device* device = RT_NULL;

    RT_ASSERT(netdev);
    RT_ASSERT(dns_server);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return -RT_ERROR;
    }

    resp = at_create_resp(ml302_DNS_RESP_LEN, 0, ml302_DNS_RESP_TIMEO);
    if (resp == RT_NULL)
    {
        LOG_D("ml302 set dns server failed, no memory for response object.");
        result = -RT_ENOMEM;
        goto __exit;
    }

    /* send "AT+MDNSCFG=<pri_dns>[,<sec_dns>]" commond to set dns servers */
    if (at_obj_exec_cmd(device->client, resp, "AT+MDNSCFG=\"%s\"", inet_ntoa(*dns_server)) < 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

    netdev_low_level_set_dns_server(netdev, dns_num, dns_server);

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_ping_domain_resolve(struct at_device* device, const char* name, char ip[16])
{
    int result = RT_EOK;
    char recv_ip[16] = {0};
    at_response_t resp = RT_NULL;

    /* The maximum response time is 14 seconds, affected by network status */
    resp = at_create_resp(256, 4, 14 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for ml302 device(%s) response structure.", device->name);
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+MDNSGIP=\"%s\"", name) < 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

    /* parse the third line of response data, get the IP address */
    if (at_resp_parse_line_args_by_kw(resp, "+MDNSGIP:", "+MDNSGIP: %*[^,],%*[^,],\"%[^\"]", recv_ip) < 0)
    {
        rt_thread_mdelay(100);
        /* resolve failed, maybe receive an URC CRLF */
    }

    if (rt_strlen(recv_ip) < 8)
    {
        rt_thread_mdelay(100);
        /* resolve failed, maybe receive an URC CRLF */
    }
    else
    {
        rt_strncpy(ip, recv_ip, 15);
        ip[15] = '\0';
    }

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

#ifdef NETDEV_USING_PING
static int ml302_netdev_ping(struct netdev* netdev, const char* host,
                             size_t data_len, uint32_t timeout, struct netdev_ping_resp* ping_resp)
{
#define ml302_PING_RESP_SIZE 256
#define ml302_PING_IP_SIZE 16
#define ml302_PING_TIMEO (10 * RT_TICK_PER_SECOND)

#define ml302_PING_ERR_TIME 600
#define ml302_PING_ERR_TTL 255

    int result = RT_EOK;
    int time, ttl, i, err_code = 0;
    char ip_addr[ml302_PING_IP_SIZE] = {0};
    at_response_t resp = RT_NULL;
    struct at_device* device = RT_NULL;

    RT_ASSERT(netdev);
    RT_ASSERT(host);
    RT_ASSERT(ping_resp);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get ml302 device by netdev name(%s) failed.", netdev->name);
        return -RT_ERROR;
    }

    for (i = 0; i < rt_strlen(host) && !isalpha((int)(host[i])); i++)
    {
        ;
    }

    if (i < strlen(host))
    {
        /* check domain name is usable */
        if (ml302_ping_domain_resolve(device, host, ip_addr) < 0)
        {
            return -RT_ERROR;
        }
        rt_memset(ip_addr, 0x00, ml302_PING_IP_SIZE);
    }

    resp = at_create_resp(ml302_PING_RESP_SIZE, 4, ml302_PING_TIMEO);
    if (resp == RT_NULL)
    {
        LOG_E("ml302 device(%s) set dns server failed, no memory for response object.", device->name);
        result = -RT_ERROR;
        goto __exit;
    }

    /* domain name prase error options */
    if (at_resp_parse_line_args_by_kw(resp, "+MDNSGIP:", "+MDNSGIP: 0,%d", &err_code) > 0)
    {
        /* 3 - network error, 8 - dns common error */
        if (err_code == 3 || err_code == 8)
        {
            result = -RT_ERROR;
            goto __exit;
        }
    }

    if (data_len < 36)
    {
        data_len = 36;
    }
    if (data_len > 1500)
    {
        data_len = 1500;
    }

    /* send "AT+MPING=<IP addr>[,<timeout>[,<retryNum>[,<dataLen>]]]" commond to send ping request */
    if (at_obj_exec_cmd(device->client, resp, "AT+MPING=\"%s\",%d,1,%d",
                        host, ml302_PING_TIMEO / (RT_TICK_PER_SECOND / 10), data_len) < 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

    if (at_resp_parse_line_args_by_kw(resp, "0", "0 %[^:]:%*[^=]=%*[^=]= %d(ms)%*[^=]= %d\r\n",
                                      ip_addr, &time, &ttl) <= 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

    /* the ping request timeout expires, the response time settting to 600 and ttl setting to 255 */
    if (time == ml302_PING_ERR_TIME && ttl == ml302_PING_ERR_TTL)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    inet_aton(ip_addr, &(ping_resp->ip_addr));
    ping_resp->data_len = data_len;
    /* reply time, in units of 100 ms */
    ping_resp->ticks = time;
    ping_resp->ttl = ttl;

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}
#endif /* NETDEV_USING_PING */

#ifdef NETDEV_USING_NETSTAT
void ml302_netdev_netstat(struct netdev* netdev)
{
#define ML302_NETSTAT_RESP_SIZE         320
#define ML302_NETSTAT_SOCK_SIZE         4
#define ML302_NETSTAT_TYPE_SIZE         4
#define ML302_NETSTAT_IPADDR_SIZE       17
#define ML302_NETSTAT_PORT_SIZE       10
#define ML302_NETSTAT_STATE_SIZE       25
#define ML302_NETSTAT_EXPRESSION        "+MIPSTATE:%[^,],\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",%[^,],%*d,%*d"

    at_response_t resp = RT_NULL;
    struct at_device* device = RT_NULL;
    int i;
    char* sock = RT_NULL;
    char* type = RT_NULL;
    char* port = RT_NULL;
    char* ipaddr = RT_NULL;
    char* state = RT_NULL;

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
    if (device == RT_NULL)
    {
        LOG_E("get device(%s) failed.", netdev->name);
        return;
    }

    sock = (char*) rt_calloc(1, ML302_NETSTAT_SOCK_SIZE);
    type = (char*) rt_calloc(1, ML302_NETSTAT_TYPE_SIZE);
    ipaddr = (char*) rt_calloc(1, ML302_NETSTAT_IPADDR_SIZE);
    port = (char*) rt_calloc(1, ML302_NETSTAT_PORT_SIZE);
    state = (char*) rt_calloc(1, ML302_NETSTAT_STATE_SIZE);
    if ((sock && type && ipaddr && port && state) == RT_NULL)
    {
        LOG_E("no memory for ipaddr create.");
        goto __exit;
    }

    resp = at_create_resp(ML302_NETSTAT_RESP_SIZE, 0, 5 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.", device->name);
        goto __exit;
    }

    /* send network connection information commond "AT+MIPSTATE" and wait response */
    if (at_obj_exec_cmd(device->client, resp, "AT+MIPSTATE") < 0)
    {
        goto __exit;
    }

    for (i = 1; i <= resp->line_counts; i++)
    {
        rt_memset(sock, 0x00, ML302_NETSTAT_SOCK_SIZE);
        rt_memset(type, 0x00, ML302_NETSTAT_TYPE_SIZE);
        rt_memset(ipaddr, 0x00, ML302_NETSTAT_IPADDR_SIZE);
        rt_memset(port, 0x00, ML302_NETSTAT_PORT_SIZE);
        rt_memset(state, 0x00, ML302_NETSTAT_STATE_SIZE);
        if (strstr(at_resp_get_line(resp, i), "+MIPSTATE"))
        {
            /* parse the third line of response data, get the network connection information */
            if (at_resp_parse_line_args(resp, i, ML302_NETSTAT_EXPRESSION, sock, type, ipaddr, port, state) <= 0)
            {
                goto __exit;
            }
            else
            {
                LOG_RAW("sock:%s, type:%s, addr:%s, port:%s, state:%s\n", sock, type, ipaddr, port, state);
            }
        }
    }

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    if (sock)
    {
        rt_free(sock);
    }
    if (type)
    {
        rt_free(type);
    }
    if (ipaddr)
    {
        rt_free(ipaddr);
    }
    if (port)
    {
        rt_free(port);
    }
    if (state)
    {
        rt_free(state);
    }
}
#endif /* NETDEV_USING_NETSTAT */

const struct netdev_ops ml302_netdev_ops =
{
    ml302_netdev_set_up,
    ml302_netdev_set_down,

    RT_NULL, /* not support set ip, netmask, gatway address */
    ml302_netdev_set_dns_server,
    RT_NULL, /* not support set DHCP status */

#ifdef NETDEV_USING_PING
    ml302_netdev_ping,
#endif
#ifdef NETDEV_USING_NETSTAT
    ml302_netdev_netstat,
#endif
};

static struct netdev* ml302_netdev_add(const char* netdev_name)
{
#define ml302_NETDEV_MTU 1500
    struct netdev* netdev = RT_NULL;

    RT_ASSERT(netdev_name);

    netdev = netdev_get_by_name(netdev_name);
    if (netdev != RT_NULL)
    {
        return (netdev);
    }

    netdev = (struct netdev*)rt_calloc(1, sizeof(struct netdev));
    if (netdev == RT_NULL)
    {
        LOG_E("no memory for ml302 device(%s) netdev structure.", netdev_name);
        return RT_NULL;
    }

    netdev->mtu = ml302_NETDEV_MTU;
    netdev->ops = &ml302_netdev_ops;

#ifdef SAL_USING_AT
    extern int sal_at_netdev_set_pf_info(struct netdev * netdev);
    /* set the network interface socket/netdb operations */
    sal_at_netdev_set_pf_info(netdev);
#endif

    netdev_register(netdev, netdev_name, RT_NULL);

    return netdev;
}

/* =============================  sim76xx device operations ============================= */

#define AT_SEND_CMD(client, resp, resp_line, timeout, cmd)                                      \
    do                                                                                          \
    {                                                                                           \
        (resp) = at_resp_set_info((resp), 128, (resp_line), rt_tick_from_millisecond(timeout)); \
        if (at_obj_exec_cmd((client), (resp), (cmd)) < 0)                                       \
        {                                                                                       \
            result = -RT_ERROR;                                                                 \
            goto __exit;                                                                        \
        }                                                                                       \
    } while (0)

/* init for ml302 */
static void ml302_init_thread_entry(void* parameter)
{
#define INIT_RETRY 5
#define CPIN_RETRY 10
#define CSQ_RETRY 10
#define CREG_RETRY 10
#define CGREG_RETRY 30
#define CGATT_RETRY 10

    int i, retry_num = INIT_RETRY;

    char parsed_data[10] = {0};
    rt_err_t result = RT_EOK;
    at_response_t resp = RT_NULL;
    struct at_device* device = (struct at_device*)parameter;
    struct at_client* client = device->client;

    resp = at_create_resp(128, 0, rt_tick_from_millisecond(300));
    if (resp == RT_NULL)
    {
        LOG_E("no memory for ml302 device(%s) response structure.", device->name);
        return;
    }

    while (retry_num--)
    {
        rt_memset(parsed_data, 0, sizeof(parsed_data));
        rt_thread_mdelay(1000);
        ml302_power_on(device);
        rt_thread_mdelay(5000); //check the ml302 hardware manual, when we use the pow_key to start ml302, it takes about 20s,so we put 25s here to ensure starting air720 normally.

        LOG_I("start initializing the ml302 device(%s)", device->name);
        /* wait ml302 startup finish */
        if (at_client_obj_wait_connect(client, ML302_WAIT_CONNECT_TIME))
        {
            result = -RT_ETIMEOUT;
            goto __exit;
        }

        /* disable echo */
        AT_SEND_CMD(client, resp, 0, 300, "ATE0");
        /* get module version */
        AT_SEND_CMD(client, resp, 0, 300, "ATI");
        /* show module version */
        for (i = 0; i < (int)resp->line_counts - 1; i++)
        {
            LOG_I("%s", at_resp_get_line(resp, i + 1));
        }
        /* check SIM card */
        for (i = 0; i < CPIN_RETRY; i++)
        {
            AT_SEND_CMD(client, resp, 2, 5 * RT_TICK_PER_SECOND, "AT+CPIN?");

            if (at_resp_get_line_by_kw(resp, "READY"))
            {
                LOG_I("ml302 device(%s) SIM card detection success.", device->name);
                break;
            }
            rt_thread_mdelay(500);
        }
        if (i == CPIN_RETRY)
        {
            LOG_E("ml302 device(%s) SIM card detection failed.", device->name);
            result = -RT_ERROR;
            goto __exit;
        }
        /* waiting for dirty data to be digested */
        rt_thread_mdelay(100);

        /* check the GSM network is registered */
        for (i = 0; i < CREG_RETRY; i++)
        {
            AT_SEND_CMD(client, resp, 0, 300, "AT+CREG?");
            at_resp_parse_line_args_by_kw(resp, "+CREG:", "+CREG: %s", &parsed_data);
            if (!strncmp(parsed_data, "0,1", sizeof(parsed_data)) ||
                !strncmp(parsed_data, "0,5", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) GSM network is registered(%s),", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CREG_RETRY)
        {
            LOG_E("ml302 device(%s) GSM network is register failed(%s).", device->name, parsed_data);
            result = -RT_ERROR;
            goto __exit;
        }
        /* check the GPRS network is registered */
        for (i = 0; i < CGREG_RETRY; i++)
        {
            AT_SEND_CMD(client, resp, 0, 300, "AT+CGREG?");
            at_resp_parse_line_args_by_kw(resp, "+CGREG:", "+CGREG: %s", &parsed_data);
            if (!strncmp(parsed_data, "0,1", sizeof(parsed_data)) ||
                !strncmp(parsed_data, "0,5", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) GPRS network is registered(%s).", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CGREG_RETRY)
        {
            LOG_E("ml302 device(%s) GPRS network is register failed(%s).", device->name, parsed_data);
            result = -RT_ERROR;
            goto __exit;
        }

        /* check signal strength */
        for (i = 0; i < CSQ_RETRY; i++)
        {
            AT_SEND_CMD(client, resp, 0, 300, "AT+CSQ");
            at_resp_parse_line_args_by_kw(resp, "+CSQ:", "+CSQ: %s", &parsed_data);
            if (strncmp(parsed_data, "99,99", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) signal strength: %s", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CSQ_RETRY)
        {
            LOG_E("ml302 device(%s) signal strength check failed (%s)", device->name, parsed_data);
            result = -RT_ERROR;
            goto __exit;
        }

        for (i = 0; i < CGATT_RETRY; i++)
        {
            AT_SEND_CMD(client, resp, 0, 300, "AT+CGATT?");
            at_resp_parse_line_args_by_kw(resp, "+CGATT:", "+CGATT: %s", &parsed_data);
            if (strncmp(parsed_data, "1", sizeof(parsed_data)) == 0)
            {
                LOG_I("ml302 device(%s) attach GPRS", device->name);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CGATT_RETRY)
        {
            LOG_E("ml302 device(%s) can't attach GPRS ", device->name);
            result = -RT_ERROR;
            goto __exit;
        }

        AT_SEND_CMD(client, resp, 0, 300, "AT+FOTAMODE=1");

        AT_SEND_CMD(client, resp, 0, 300, "AT+CFUN=1");

        AT_SEND_CMD(client, resp, 0, 300, "AT+CSCLK=0");

        AT_SEND_CMD(client, resp, 0, 300, "AT+CTZU=1");

        AT_SEND_CMD(client, resp, 0, 300, "AT+MIPMODE=0");

        AT_SEND_CMD(client, resp, 0, 300, "AT+CGDCONT=1,\"IP\",\"CMIOT\"");

        AT_SEND_CMD(client, resp, 0, 5 * 1000, "AT+CGACT=1,1");

        AT_SEND_CMD(client, resp, 2, 300, "AT+CGPADDR=1");

        if (at_resp_get_line_by_kw(resp, "+CGPADDR: 1,\"0.0.0.0\"") != RT_NULL)
        {
            LOG_E("ml302 device(%s) get the local address failed.", device->name);
            result = -RT_ERROR;
            goto __exit;
        }
        result = RT_EOK;

__exit:
        if (result == RT_EOK)
        {
            break;
        }
        else
        {
            /* power off the ml302 device */
            ml302_power_off(device);
            rt_thread_mdelay(1000);

            LOG_I("ml302 device(%s) initialize retry...", device->name);
        }
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    if (result == RT_EOK)
    {
        /* set network interface device status and address information */
        ml302_netdev_set_info(device->netdev);

        if (rt_thread_find(device->netdev->name) == RT_NULL)
        {
            ml302_netdev_check_link_status(device->netdev);
        }

        LOG_I("ml302 device(%s) network initialize success!", device->name);
    }
    else
    {
        LOG_E("ml302 device(%s) network initialize failed(%d)!", device->name, result);
    }
}

static int ml302_net_init(struct at_device* device)
{
#ifdef AT_DEVICE_ML302_INIT_ASYN
    rt_thread_t tid;
    tid = rt_thread_create("ml302_net_init", ml302_init_thread_entry, (void*)device,
                           ML302_THREAD_STACK_SIZE, ML302_THREAD_PRIORITY, 20);
    if (tid)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("create ml302 device(%s) initialization thread failed.", device->name);
        return -RT_ERROR;
    }
#else
    ml302_init_thread_entry(device);
#endif /* AT_DEVICE_ml302_INIT_ASYN */

    return RT_EOK;
}

static void urc_func(struct at_client* client, const char* data, rt_size_t size)
{
    RT_ASSERT(data);

    LOG_I("URC data : %.*s", size, data);
}

/* ml302 device URC table for the device control */
static const struct at_urc urc_table[] =
{
    {"RDY", "\r\n", urc_func},
    {"CLOSED", "\r\n", urc_func},
    {"SMS READY", "\r\n", urc_func},
    {"UART Boot Completed", "\r\n", urc_func},
};

static int ml302_init(struct at_device* device)
{
    struct at_device_ml302* ml302 = (struct at_device_ml302*)device->user_data;

    /* initialize AT client */
    at_client_init(ml302->client_name, ml302->recv_line_num);

    device->client = at_client_get(ml302->client_name);
    if (device->client == RT_NULL)
    {
        LOG_E("ml302 device(%s) initialize failed, get AT client(%s) failed.", ml302->device_name, ml302->client_name);
        return -RT_ERROR;
    }

    /* register URC data execution function  */
    at_obj_set_urc_table(device->client, urc_table, sizeof(urc_table) / sizeof(urc_table[0]));

#ifdef AT_USING_SOCKET
    ml302_socket_init(device);
#endif

    /* add ml302 device to the netdev list */
    device->netdev = ml302_netdev_add(ml302->device_name);
    if (device->netdev == RT_NULL)
    {
        LOG_E("ml302 device(%s) initialize failed, get network interface device failed.", ml302->device_name);
        return -RT_ERROR;
    }

    /* initialize ml302 pin configuration */
    if (ml302->power_en_pin != -1)
    {
        rt_pin_mode(ml302->power_en_pin, PIN_MODE_OUTPUT);
    }
    if (ml302->power_pin != -1)
    {
        rt_pin_mode(ml302->power_pin, PIN_MODE_OUTPUT);
    }
    if (ml302->power_status_pin != -1)
    {
        rt_pin_mode(ml302->power_status_pin, PIN_MODE_INPUT);
    }

    /* initialize ml302 device network */
    return ml302_netdev_set_up(device->netdev);
}

static int ml302_deinit(struct at_device* device)
{
    return ml302_netdev_set_down(device->netdev);
}

static int ml302_reboot(struct at_device* device)
{
    ml302_power_off(device);
    rt_thread_mdelay(2000);
    ml302_power_on(device);
    device->is_init = RT_FALSE;
    ml302_net_init(device);
    device->is_init = RT_TRUE;
    return RT_EOK;
}

static int ml302_reset(struct at_device* device)
{
    int result = RT_EOK;
    struct at_client* client = device->client;

    /* send "AT+RST" commonds to mw31 device */
    result = at_obj_exec_cmd(client, RT_NULL, "AT+MREBOOT");
    rt_thread_mdelay(1000);

    /* waiting 10 seconds for mw31 device reset */
    device->is_init = RT_FALSE;
    if (at_client_obj_wait_connect(client, ML302_WAIT_CONNECT_TIME))
    {
        return -RT_ETIMEOUT;
    }

    /* initialize ml302 device network */
    ml302_net_init(device);

    device->is_init = RT_TRUE;

    return result;
}

static int ml302_get_signal(struct at_device* device, unsigned char* rssi)
{
    int result = RT_EOK;
    struct at_response* resp = RT_NULL;
    int rssi_val = 99;

    if (rssi == RT_NULL)
    {
        LOG_E("input rssi error.");
        return -RT_ERROR;
    }

    resp = at_create_resp(128, 0, 2 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CSQ") == 0)
    {
        at_resp_parse_line_args_by_kw(resp, "+CSQ:", "+CSQ: %d,%*d", &rssi_val);
        *rssi = rssi_val & 0xff;
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_get_module_info(struct at_device* device, struct at_device_ml302_module_info* module_info)
{
    int result = RT_EOK;
    struct at_response* resp = RT_NULL;

    if (module_info == RT_NULL)
    {
        LOG_E("input module_info error.");
        return -RT_ERROR;
    }

    resp = at_create_resp(256, 0, 2 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CGMM") == 0)
    {
        at_resp_parse_line_args(resp, 2, "%[^\r]\r", module_info->model);
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CGMR") == 0)
    {
        at_resp_parse_line_args(resp, 2, "%[^\r]\r", module_info->firmware);
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CGSN=1") == 0)
    {
        at_resp_parse_line_args_by_kw(resp, "+CGSN:", "+CGSN: %[^\r]\r", module_info->imei);
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CIMI") == 0)
    {
        at_resp_parse_line_args(resp, 2, "%[^\r]\r", module_info->imsi);
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+ICCID") == 0)
    {
        at_resp_parse_line_args_by_kw(resp, "+ICCID:", "+ICCID: %[^\r]\r", module_info->iccid);
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_net_conn(struct at_device* device, char* ap_name)
{
    int i;
    char parsed_data[10] = {0};
    struct at_client* client = device->client;
    int result = RT_EOK;
    struct at_response* resp = RT_NULL;

    if (ap_name == RT_NULL)
    {
        LOG_E("input ap_name error.");
        return -RT_ERROR;
    }

    resp = at_create_resp(128, 0, rt_tick_from_millisecond(300));
    if (resp == RT_NULL)
    {
        LOG_E("no memory for ml302 device(%s) response structure.", device->name);
        return -RT_ERROR;
    }

    do
    {
        rt_memset(parsed_data, 0, sizeof(parsed_data));

        LOG_I("start initializing the ml302 device(%s)", device->name);
        /* wait ml302 startup finish */
        if (at_client_obj_wait_connect(client, ML302_WAIT_CONNECT_TIME))
        {
            result = -RT_ETIMEOUT;
            break;
        }

        /* disable echo */
        if (at_obj_exec_cmd(device->client, resp, "ATE0") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }
        /* get module version */
        if (at_obj_exec_cmd(device->client, resp, "ATI") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }
        /* show module version */
        for (i = 0; i < (int)resp->line_counts - 1; i++)
        {
            LOG_I("%s", at_resp_get_line(resp, i + 1));
        }
        /* check SIM card */
        for (i = 0; i < CPIN_RETRY; i++)
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CPIN?") < 0)
            {
                result = -RT_ETIMEOUT;
                break;
            }

            if (at_resp_get_line_by_kw(resp, "READY"))
            {
                LOG_I("ml302 device(%s) SIM card detection success.", device->name);
                break;
            }
            rt_thread_mdelay(500);
        }
        if (i == CPIN_RETRY)
        {
            LOG_E("ml302 device(%s) SIM card detection failed.", device->name);
            result = -RT_ERROR;
            break;
        }
        /* waiting for dirty data to be digested */
        rt_thread_mdelay(100);

        /* check the GSM network is registered */
        for (i = 0; i < CREG_RETRY; i++)
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CREG?") < 0)
            {
                result = -RT_ETIMEOUT;
                break;
            }
            at_resp_parse_line_args_by_kw(resp, "+CREG:", "+CREG: %s", &parsed_data);
            if (!strncmp(parsed_data, "0,1", sizeof(parsed_data)) ||
                !strncmp(parsed_data, "0,5", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) GSM network is registered(%s),", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CREG_RETRY)
        {
            LOG_E("ml302 device(%s) GSM network is register failed(%s).", device->name, parsed_data);
            result = -RT_ERROR;
            break;
        }
        /* check the GPRS network is registered */
        for (i = 0; i < CGREG_RETRY; i++)
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CGREG?") < 0)
            {
                result = -RT_ETIMEOUT;
                break;
            }
            at_resp_parse_line_args_by_kw(resp, "+CGREG:", "+CGREG: %s", &parsed_data);
            if (!strncmp(parsed_data, "0,1", sizeof(parsed_data)) ||
                !strncmp(parsed_data, "0,5", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) GPRS network is registered(%s).", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CGREG_RETRY)
        {
            LOG_E("ml302 device(%s) GPRS network is register failed(%s).", device->name, parsed_data);
            result = -RT_ERROR;
            break;
        }

        /* check signal strength */
        for (i = 0; i < CSQ_RETRY; i++)
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CSQ") < 0)
            {
                result = -RT_ETIMEOUT;
                break;
            }
            at_resp_parse_line_args_by_kw(resp, "+CSQ:", "+CSQ: %s", &parsed_data);
            if (strncmp(parsed_data, "99,99", sizeof(parsed_data)))
            {
                LOG_I("ml302 device(%s) signal strength: %s", device->name, parsed_data);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CSQ_RETRY)
        {
            LOG_E("ml302 device(%s) signal strength check failed (%s)", device->name, parsed_data);
            result = -RT_ERROR;
            break;
        }

        for (i = 0; i < CGATT_RETRY; i++)
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+CGATT?") < 0)
            {
                result = -RT_ETIMEOUT;
                break;
            }
            at_resp_parse_line_args_by_kw(resp, "+CGATT:", "+CGATT: %s", &parsed_data);
            if (strncmp(parsed_data, "1", sizeof(parsed_data)) == 0)
            {
                LOG_I("ml302 device(%s) attach GPRS", device->name);
                break;
            }
            rt_thread_mdelay(1000);
        }
        if (i == CGATT_RETRY)
        {
            LOG_E("ml302 device(%s) can't attach GPRS ", device->name);
            result = -RT_ERROR;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+FOTAMODE=1") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CFUN=1") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CSCLK=0") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CTZU=1") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+MIPMODE=0") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CGDCONT=1,\"IP\",\"%s\"", ap_name) < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CGACT=1,1") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_obj_exec_cmd(device->client, resp, "AT+CGPADDR=1") < 0)
        {
            result = -RT_ETIMEOUT;
            break;
        }

        if (at_resp_get_line_by_kw(resp, "+CGPADDR: 1,\"0.0.0.0\"") != RT_NULL)
        {
            LOG_E("ml302 device(%s) get the local address failed.", device->name);
            result = -RT_ERROR;
            break;
        }
        result = RT_EOK;
    } while (0);

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_net_disconn(struct at_device* device)
{
    int result = RT_EOK;
    struct at_response* resp = RT_NULL;

    resp = at_create_resp(128, 0, 2 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CGACT=0,1") < 0)
    {
        result = RT_ERROR;
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_get_time(struct at_device* device, time_t* time)
{
    int result = RT_EOK;
    struct at_response* resp = RT_NULL;
    int year = 0;
    int month = 0;
    int date = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    int timenum = 0;
    struct tm time_now;
    time_t get_time;

    if (time == RT_NULL)
    {
        LOG_E("input time error.");
        return -RT_ERROR;
    }

    resp = at_create_resp(128, 0, 2 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+CCLK?") == 0)
    {
        at_resp_parse_line_args_by_kw(resp, "+CCLK:", "+CCLK: \"%d/%d/%d,%d:%d:%d+%d\"", &year, &month, &date, &hour, &min, &sec, &timenum);

        time_now.tm_year = year + 2000 - 1900;
        time_now.tm_mon = month - 1;
        time_now.tm_mday = date;
        time_now.tm_hour = hour;
        time_now.tm_min = min;
        time_now.tm_sec = sec;
        get_time = mktime(&time_now);
        get_time -= (timenum / 4) * 60 * 60;

        if (time != RT_NULL)
        {
            *time = get_time;
        }
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

static int ml302_control(struct at_device* device, int cmd, void* arg)
{
    int result = -RT_ERROR;

    RT_ASSERT(device);

    switch (cmd)
    {
        case AT_DEVICE_CTRL_LOW_POWER:
        case AT_DEVICE_CTRL_SLEEP:
        case AT_DEVICE_CTRL_WAKEUP:
        case AT_DEVICE_CTRL_SET_WIFI_INFO:
        case AT_DEVICE_CTRL_GET_GPS:
            LOG_W("ml302 not support the control command(%d).", cmd);
            break;
        case AT_DEVICE_CTRL_GET_VER:
            result = ml302_get_module_info(device, (struct at_device_ml302_module_info*)arg);
            break;
        case AT_DEVICE_CTRL_POWER_ON:
            ml302_power_on(device);
            result = RT_EOK;
            break;
        case AT_DEVICE_CTRL_POWER_OFF:
            ml302_power_off(device);
            result = RT_EOK;
            break;
        case AT_DEVICE_CTRL_NET_CONN:
            result = ml302_net_conn(device, (char*)arg);
            break;
        case AT_DEVICE_CTRL_NET_DISCONN:
            result = ml302_net_disconn(device);
            break;
        case AT_DEVICE_CTRL_GET_SIGNAL:
            result = ml302_get_signal(device, (unsigned char*)arg);
            break;
        case AT_DEVICE_CTRL_RESET:
            result = ml302_reset(device);
            break;
        case AT_DEVICE_CTRL_GET_TIME:
            result = ml302_get_time(device, (time_t*)arg);
            break;
        //case AT_DEVICE_CTRL_REBOOT:
        //    result = ml302_reboot(device);
        //    break;
        default:
            LOG_E("input error control command(%d).", cmd);
            break;
    }

    return result;
}

const struct at_device_ops ml302_device_ops =
{
    ml302_init,
    ml302_deinit,
    ml302_control,
};

static int ml302_device_class_register(void)
{
    struct at_device_class* class = RT_NULL;

    class = (struct at_device_class*)rt_calloc(1, sizeof(struct at_device_class));
    if (class == RT_NULL)
    {
        LOG_E("no memory for ml302 device class create.");
        return -RT_ENOMEM;
    }

    /* fill ml302 device class object */
#ifdef AT_USING_SOCKET
    ml302_socket_class_register(class);
#endif
    class->device_ops = &ml302_device_ops;

    return at_device_class_register(class, AT_DEVICE_CLASS_ML302);
}
INIT_DEVICE_EXPORT(ml302_device_class_register);

#endif /* AT_DEVICE_USING_ml302 */
