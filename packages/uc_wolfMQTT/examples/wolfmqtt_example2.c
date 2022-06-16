
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <socket/netdb.h>
#include <arpa/inet.h>
#include <netdev.h>

#include "port/mqttctx.h"
#include "port/mqttnet.h"
#include "wolfmqtt/mqtt_client.h"


#define THREAD_ORIORITY   RT_MAIN_THREAD_PRIORITY
#define THREAD_STACK_SIZE 512
#define THREAD_TIMESLICE  5


#define MAX_BUFFER_SIZE 2048

static int mqttclient_message_cb(MqttClient* client, MqttMessage* msg,
                                 byte msg_new, byte msg_done)
{
    byte buf[PRINT_BUFFER_SIZE + 1];
    word32 len;
    MQTTCtx* mqttCtx = (MQTTCtx*)client->ctx;

    (void)mqttCtx;

    if (msg_new)
    {
        /* Determine min size to dump */
        len = msg->topic_name_len;
        if (len > PRINT_BUFFER_SIZE)
        {
            len = PRINT_BUFFER_SIZE;
        }
        XMEMCPY(buf, msg->topic_name, len);
        buf[len] = '\0'; /* Make sure its null terminated */

        /* Print incoming message */
        PRINTF("MQTT Message: Topic %s, Qos %d, Len %u",
               buf, msg->qos, msg->total_len);
    }

    /* Print message payload */
    len = msg->buffer_len;
    if (len > PRINT_BUFFER_SIZE)
    {
        len = PRINT_BUFFER_SIZE;
    }
    XMEMCPY(buf, msg->buffer, len);
    buf[len] = '\0'; /* Make sure its null terminated */
    PRINTF("Payload (%d - %d): %s",
           msg->buffer_pos, msg->buffer_pos + len, buf);

    if (msg_done)
    {
        PRINTF("MQTT Message: Done");
    }

    /* Return negative to terminate publish processing */
    return MQTT_CODE_SUCCESS;
}

int mqttclient_conn(MQTTCtx* mqttCtx)
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
    mqttCtx->tx_buf = (byte*)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);
    mqttCtx->rx_buf = (byte*)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);

    /* Initialize MqttClient structure */
    rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net,
                         mqttclient_message_cb,
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
        mqttCtx->lwt_msg.topic_name = WOLFMQTT_TOPIC_NAME"lwttopic";
        mqttCtx->lwt_msg.buffer = (byte*)mqttCtx->client_id;
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
           (mqttCtx->connect.ack.flags&
            MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT) ?
           1 : 0
          );

    return rc;
}

int mqttclient_disconn(MQTTCtx* mqttCtx)
{
    int rc = MQTT_CODE_SUCCESS;

    //disconn:
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

int mqttclient_waitmessage(MQTTCtx* mqttCtx, int timeout_ms)
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
            if (rc_ping != MQTT_CODE_SUCCESS)
            {
                PRINTF("MQTT Ping Keep Alive Error: %s (%d)",
                       MqttClient_ReturnCodeToString(rc), rc);
                //break;
            }
            else
            {
                rt_kprintf("Send Ping Keep Alive Succ!!\r\n");
            }
        }
    }

    return rc;
}

int mqttclient_subscribe(MQTTCtx* mqttCtx, char* topic_name, uint8_t qos)
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
        MqttTopic* topic = &mqttCtx->subscribe.topics[i];
        PRINTF("  Topic %s, Qos %u, Return Code %u",
               topic->topic_filter,
               topic->qos, topic->return_code);
    }

    return rc;
}

int mqttclient_unsubscribe(MQTTCtx* mqttCtx, char* topic_name)
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
        MqttTopic* topic = &mqttCtx->unsubscribe.topics[i];
        PRINTF("  Topic %s, Qos %u, Return Code %u",
               topic->topic_filter,
               topic->qos, topic->return_code);
    }

    return rc;
}

int mqttclient_publish(MQTTCtx* mqttCtx, const char* topic_name, uint8_t* data, uint16_t length, uint8_t qos, uint8_t retain, uint8_t duplicate)
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

static void mqtt_task(void* parameter)
{
    struct netdev* netdev = RT_NULL;

    rt_kprintf("mqtt_task start\r\n");

    rt_thread_mdelay(2000);
    netdev = netdev_get_by_name("ml302");
    if (netdev == RT_NULL)
    {
        rt_kprintf("get network interface device(%s) failed.\n", "ml302");
        return;
    }
    while (1)
    {
        /* 通过名称获取 netdev 网卡对象 */
        if (!netdev_is_link_up(netdev))
        {
            //rt_kprintf("get network interface device(%s) failed.\n", "ml302");
            //return -RT_ERROR;
            rt_thread_mdelay(100);
        }
        else
        {
            break;
        }
    }

    int rc;
    MQTTCtx mqttCtx;
    int conn_state = 0;

    /* init defaults */
    mqtt_init_ctx(&mqttCtx);
    mqttCtx.app_name = "mqttclient";
    mqttCtx.client_id = "wolfTest-123";
    mqttCtx.host = "broker-cn.emqx.io";
    mqttCtx.port = 1883;
    mqttCtx.keep_alive_sec = 60;
    mqttCtx.username = "wolfmqtt";
    mqttCtx.password = "123456";
    mqttCtx.cmd_timeout_ms = 30000;
    mqttCtx.max_packet_size = 2048;

    while (1)
    {
        while (conn_state == 0)
        {
            if (mqttclient_conn(&mqttCtx) == MQTT_CODE_SUCCESS)
            {
                rt_kprintf("mqttclient_conn Succ!!!\r\n");
                mqttclient_subscribe(&mqttCtx, "wolfMQTT/down", 0);
                mqttclient_subscribe(&mqttCtx, "wolfMQTT/down1", 0);
                mqttclient_publish(&mqttCtx, "wolfMQTT/up", (uint8_t*)"I am coming!", (uint16_t)rt_strlen("I am coming!"), 0, 0, 0);
                //mqttclient_unsubscribe(&mqttCtx, "wolfMQTT/down1");
                conn_state = 1;
                break;
            }
            mqttclient_disconn(&mqttCtx);
            rt_thread_mdelay(10000);
        }

        rc = mqttclient_waitmessage(&mqttCtx, 10);
        if (rc != MQTT_CODE_SUCCESS)
        {
            conn_state = 0;
            mqttclient_disconn(&mqttCtx);
        }
    }

    //mqtt_free_ctx(&mqttCtx);
}

int main(void)
{
    rt_thread_t tid = RT_NULL;

    rt_kprintf("hello world!!\r\n");

    tid = rt_thread_create("mqtt_task",
                           mqtt_task,
                           RT_NULL,
                           THREAD_STACK_SIZE * 8,
                           THREAD_ORIORITY,
                           THREAD_TIMESLICE);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
}
