
#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <socket/netdb.h>
#include <arpa/inet.h>
#include <netdev.h>

#include "wiota_mqtt.h"

// #define THREAD_ORIORITY   RT_MAIN_THREAD_PRIORITY
// #define THREAD_STACK_SIZE 512
// #define THREAD_TIMESLICE  5

#define MAX_BUFFER_SIZE 2048

static MQTTCtx mqttCtx;
static struct netdev *netdev = RT_NULL;
static connect_state uc_mqtt_state = STATE_DISCONNECT;

int mqttclient_conn(MQTTCtx *mqttCtx, MqttMsgCb msg_cb)
{
    int rc = MQTT_CODE_SUCCESS;

    PRINTF("MQTT Client: QoS %d, Use TLS %d", mqttCtx->qos,
           mqttCtx->use_tls);

    /* Initialize Network */
    rc = MqttClientNet_Init(&mqttCtx->net, mqttCtx);
    PRINTF("MQTT Net Init: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    /* setup tx/rx buffers */
    mqttCtx->tx_buf = (byte *)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);
    mqttCtx->rx_buf = (byte *)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);

    /* Initialize MqttClient structure */
    rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net,
                         msg_cb,
                         mqttCtx->tx_buf, MAX_BUFFER_SIZE,
                         mqttCtx->rx_buf, MAX_BUFFER_SIZE,
                         mqttCtx->cmd_timeout_ms);

    PRINTF("MQTT Init: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }
    /* The client.ctx will be stored in the cert callback ctx during
       MqttSocket_Connect for use by mqtt_tls_verify_cb */
    mqttCtx->client.ctx = mqttCtx;
    /* Connect to broker */
    rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host,
                               mqttCtx->port,
                               DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, mqtt_tls_cb);

    PRINTF("MQTT Socket Connect: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    /* Build connect packet */
    XMEMSET(&mqttCtx->connect, 0, sizeof(MqttConnect));
    mqttCtx->connect.keep_alive_sec = mqttCtx->keep_alive_sec;
    mqttCtx->connect.clean_session = mqttCtx->clean_session;
    mqttCtx->connect.client_id = mqttCtx->client_id;

    /* Last will and testament sent by broker to subscribers
        of topic when broker connection is lost */
    XMEMSET(&mqttCtx->lwt_msg, 0, sizeof(mqttCtx->lwt_msg));
    mqttCtx->connect.lwt_msg = &mqttCtx->lwt_msg;
    mqttCtx->connect.enable_lwt = mqttCtx->enable_lwt;
    if (mqttCtx->enable_lwt)
    {
        /* Send client id in LWT payload */
        mqttCtx->lwt_msg.qos = mqttCtx->qos;
        mqttCtx->lwt_msg.retain = 0;
        mqttCtx->lwt_msg.topic_name = WOLFMQTT_TOPIC_NAME "lwttopic";
        mqttCtx->lwt_msg.buffer = (byte *)mqttCtx->client_id;
        mqttCtx->lwt_msg.total_len = (word16)XSTRLEN(mqttCtx->client_id);
    }
    /* Optional authentication */
    mqttCtx->connect.username = mqttCtx->username;
    mqttCtx->connect.password = mqttCtx->password;
#ifdef WOLFMQTT_V5
    mqttCtx->client.packet_sz_max = mqttCtx->max_packet_size;
    mqttCtx->client.enable_eauth = mqttCtx->enable_eauth;
#endif

    /* Send Connect and wait for Connect Ack */
    rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect);

    PRINTF("MQTT Connect: Proto (%s), %s (%d)",
           MqttClient_GetProtocolVersionString(&mqttCtx->client),
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    /* Validate Connect Ack info */
    PRINTF("MQTT Connect Ack: Return Code %u, Session Present %d",
           mqttCtx->connect.ack.return_code,
           (mqttCtx->connect.ack.flags &
            MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT)
               ? 1
               : 0);

    return rc;
}

int mqttclient_disconn(MQTTCtx *mqttCtx)
{
    int rc = MQTT_CODE_SUCCESS;

    /* Disconnect */
    rc = MqttClient_Disconnect_ex(&mqttCtx->client,
                                  &mqttCtx->disconnect);

    PRINTF("MQTT Disconnect: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    rc = MqttClient_NetDisconnect(&mqttCtx->client);

    PRINTF("MQTT Socket Disconnect: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);

    //exit:

    /* Free resources */
    if (mqttCtx->tx_buf)
    {
        WOLFMQTT_FREE(mqttCtx->tx_buf);
    }
    if (mqttCtx->rx_buf)
    {
        WOLFMQTT_FREE(mqttCtx->rx_buf);
    }

    /* Cleanup network */
    MqttClientNet_DeInit(&mqttCtx->net);

    MqttClient_DeInit(&mqttCtx->client);

    return rc;
}

static uint32_t mqttclient_get_interval_tick(uint32_t last_tick)
{
    uint32_t cur_tick = 0;
    uint32_t interval = 0;

    cur_tick = rt_tick_get();

    interval = (cur_tick >= last_tick) ? (cur_tick - last_tick) : (0xffffffff - last_tick + cur_tick + 1);

    return interval;
}

int mqttclient_waitmessage(MQTTCtx *mqttCtx, int timeout_ms)
{
    int rc = MQTT_CODE_SUCCESS;

    /* Try and read packet */
    rc = MqttClient_WaitMessage(&mqttCtx->client, timeout_ms);
    //PRINTF("MqttClient_WaitMessage rc = %d", rc);

    /* check return code */
    if (rc == MQTT_CODE_ERROR_TIMEOUT)
    {
        /* Keep Alive */
        //PRINTF("Keep-alive timeout, sending ping");
        rc = MQTT_CODE_SUCCESS;
    }
    else if (rc != MQTT_CODE_SUCCESS)
    {
        /* There was an error */
        PRINTF("MQTT Message Wait: %s (%d)",
               MqttClient_ReturnCodeToString(rc), rc);
    }

    if (rc == MQTT_CODE_SUCCESS)
    {
        static uint32_t last_tick = 0;
        static uint8_t start_flag = 1;
        uint32_t interval_tick = (mqttCtx->keep_alive_sec * 1000) / 2;

        if (start_flag == 1)
        {
            last_tick = rt_tick_get();
            start_flag = 0;
        }
        if (mqttclient_get_interval_tick(last_tick) > interval_tick)
        {
            int rc_ping = MQTT_CODE_SUCCESS;
            last_tick = rt_tick_get();
            rc_ping = MqttClient_Ping_ex(&mqttCtx->client, &mqttCtx->ping);
            if (rc_ping == MQTT_CODE_SUCCESS)
            {
                rt_kprintf("Send Ping Keep Alive Succ!!\r\n");
            }
            else
            {
                PRINTF("MQTT Ping Keep Alive Error: %s (%d)",
                       MqttClient_ReturnCodeToString(rc), rc);
            }
        }
    }

    // debug MQTT reconnect
    // static int nTest = 0;
    // nTest++;
    // if ((nTest & 15) == 15)
    // {
    //    uc_mqtt_publish("v1/devices/me/rpc/request/123", (uint8_t *)"{}", rt_strlen("{}"), 0, 0, 0);
    // }

    return rc;
}

static int mqttclient_subscribe(MQTTCtx *mqttCtx, char *topic_name, uint8_t qos)
{
    int rc = MQTT_CODE_SUCCESS;
    int i = 0;

    if ((mqttCtx == RT_NULL) || (topic_name == RT_NULL))
    {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    XMEMSET(&mqttCtx->subscribe, 0, sizeof(MqttSubscribe));
    mqttCtx->topics[i].topic_filter = topic_name;
    mqttCtx->topics[i].qos = qos;

    /* Subscribe Topic */
    mqttCtx->subscribe.packet_id = mqtt_get_packetid();
    mqttCtx->subscribe.topic_count = 1;
    //sizeof(mqttCtx->topics) / sizeof(MqttTopic);
    mqttCtx->subscribe.topics = mqttCtx->topics;

    rc = MqttClient_Subscribe(&mqttCtx->client, &mqttCtx->subscribe);

    PRINTF("MQTT Subscribe: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    /* show subscribe results */
    for (i = 0; i < mqttCtx->subscribe.topic_count; i++)
    {
        MqttTopic *topic = &mqttCtx->subscribe.topics[i];
        PRINTF("  Topic %s, Qos %u, Return Code %u",
               topic->topic_filter,
               topic->qos, topic->return_code);
    }

    return rc;
}

static int mqttclient_unsubscribe(MQTTCtx *mqttCtx, char *topic_name)
{
    int rc = MQTT_CODE_SUCCESS;
    int i = 0;

    if ((mqttCtx == RT_NULL) || (topic_name == RT_NULL))
    {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    XMEMSET(&mqttCtx->unsubscribe, 0, sizeof(MqttUnsubscribe));
    mqttCtx->topics[i].topic_filter = topic_name;

    /* UnSubscribe Topic */
    mqttCtx->unsubscribe.packet_id = mqtt_get_packetid();
    mqttCtx->unsubscribe.topic_count = 1;
    //sizeof(mqttCtx->topics) / sizeof(MqttTopic);
    mqttCtx->unsubscribe.topics = mqttCtx->topics;

    rc = MqttClient_Unsubscribe(&mqttCtx->client, &mqttCtx->unsubscribe);

    PRINTF("MQTT Unsubscribe: %s (%d)",
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS)
    {
        return rc;
    }

    /* show subscribe results */
    for (i = 0; i < mqttCtx->unsubscribe.topic_count; i++)
    {
        MqttTopic *topic = &mqttCtx->unsubscribe.topics[i];
        PRINTF("  Topic %s, Qos %u, Return Code %u",
               topic->topic_filter,
               topic->qos, topic->return_code);
    }

    return rc;
}

static int mqttclient_publish(MQTTCtx *mqttCtx, const char *topic_name, uint8_t *data, uint16_t length, uint8_t qos, uint8_t retain, uint8_t duplicate)
{
    int rc = MQTT_CODE_SUCCESS;

    if ((mqttCtx == RT_NULL) || (topic_name == RT_NULL) || (data == RT_NULL) || (length == 0))
    {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Publish Topic */
    XMEMSET(&mqttCtx->publish, 0, sizeof(MqttPublish));
    mqttCtx->publish.retain = retain;
    mqttCtx->publish.qos = qos;
    mqttCtx->publish.duplicate = duplicate;
    mqttCtx->publish.topic_name = topic_name;
    mqttCtx->publish.packet_id = mqtt_get_packetid();

    mqttCtx->publish.buffer = data;
    mqttCtx->publish.total_len = length;

    do
    {
        rc = MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);
    } while (rc == MQTT_CODE_PUB_CONTINUE);

    PRINTF("MQTT Publish: Topic %s, %s (%d)", mqttCtx->publish.topic_name, MqttClient_ReturnCodeToString(rc), rc);

    return rc;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int uc_find_netcard(char *card_name)
{
    if ((RT_NULL == card_name) || '\0' == card_name[0])
    {
        return -1;
    }
    rt_kprintf("uc_find_netcard name is %s\r\n", card_name);

    rt_thread_mdelay(2000);
    netdev = netdev_get_by_name(card_name);
    if (netdev == RT_NULL)
    {
        rt_kprintf("get network interface device(%s) failed.\n", card_name);
        return -2;
    }
    while (!netdev_is_link_up(netdev))
    {
        rt_thread_mdelay(100);
    }
    return 0;
}

connect_state uc_mqtt_get_state(void)
{
    return uc_mqtt_state;
}

void uc_mqtt_connect(MqttMsgCb msg_cb)
{
    uc_mqtt_state = STATE_DISCONNECT;

    if (RT_NULL == msg_cb)
    {
        return;
    }
    /* init defaults */
    mqtt_init_ctx(&mqttCtx);
    // mqttCtx.app_name = "mqttclient";
    // mqttCtx.client_id = "wolfTest-123";
    // mqttCtx.host = "broker-cn.emqx.io";
    // mqttCtx.port = 1883;
    // mqttCtx.keep_alive_sec = 60;
    // mqttCtx.username = "wolfmqtt";
    // mqttCtx.password = "123456";
    // mqttCtx.cmd_timeout_ms = 30000;
    // mqttCtx.max_packet_size = 2048;

    // chengdu 000b9f3fd63e 0005kn5gd64g
    mqttCtx.app_name = "mqttclient";
    mqttCtx.client_id = "000b9f3fd63e";
    mqttCtx.host = "117.172.29.2";
    mqttCtx.port = 31880;
    mqttCtx.keep_alive_sec = 60;
    mqttCtx.username = "000b9f3fd63e";
    mqttCtx.password = "000b9f3fd63e";
    mqttCtx.cmd_timeout_ms = 30000;
    mqttCtx.max_packet_size = 2048;

    while (mqttclient_conn(&mqttCtx, msg_cb) != MQTT_CODE_SUCCESS)
    {
        mqttclient_disconn(&mqttCtx);
        rt_thread_mdelay(2000);
    }
    uc_mqtt_state = STATE_CONNECT;
}

void uc_mqtt_disconnect(void)
{
    rt_kprintf("uc_mqtt_disconnect()\n");
    if (uc_mqtt_state == STATE_DISCONNECT)
        return;
    uc_mqtt_state = STATE_DISCONNECT;
    mqttclient_disconn(&mqttCtx);
}

int uc_mqtt_subscribe(char *topic, uint8_t nQos)
{
    int nRet = -1;
    if ((STATE_DISCONNECT == uc_mqtt_state) || (RT_NULL == topic) || ('\0' == topic[0]) || (nQos > 2))
    {
        return nRet;
    }
    nRet = mqttclient_subscribe(&mqttCtx, topic, nQos);
    return nRet;
}

int uc_mqtt_unsubscribe(char *topic)
{
    int nRet = -1;
    if ((STATE_DISCONNECT == uc_mqtt_state) || (RT_NULL == topic) || ('\0' == topic[0]))
    {
        return nRet;
    }
    nRet = mqttclient_unsubscribe(&mqttCtx, topic);
    return nRet;
}

int uc_mqtt_publish(char *topic,
                    uint8_t *data,
                    uint16_t nLength,
                    uint8_t nQos,
                    uint8_t nRetain,
                    uint8_t nDuplicate)
{
    int nRet = -1;
    if ((STATE_DISCONNECT == uc_mqtt_state) ||
        (RT_NULL == topic) || ('\0' == topic[0]) ||
        (RT_NULL == data) ||
        (0 == nLength) ||
        (nQos > 2))
    {
        return nRet;
    }
    rt_kprintf("publish payload: %s\n", (char *)data);
    nRet = mqttclient_publish(&mqttCtx, topic, data, nLength, nQos, nRetain, nDuplicate);
    return nRet;
}

// int uc_mqtt_wait_msg(void *pPara)
// {
//     int nTimeoutMs = (int)pPara;
//     int nRet = -1;
//     if (nTimeoutMs > (mqttCtx.keep_alive_sec * 1000))
//     {
//         return nRet;
//     }
//     nRet = mqttclient_waitmessage(&mqttCtx, nTimeoutMs);
//     return nRet;
// }

void uc_mqtt_wait_msg(void *pPara)
{
    (void)pPara;
    int nRet = MQTT_CODE_SUCCESS;
    while (MQTT_CODE_SUCCESS == nRet)
    {
        nRet = mqttclient_waitmessage(&mqttCtx, 1000);
    }
    if (nRet != MQTT_CODE_SUCCESS)
    {
        uc_mqtt_disconnect();
    }
}

#endif
