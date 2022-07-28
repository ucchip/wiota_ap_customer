#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <stdlib.h>
#include "manager_task.h"
#include "manager_wiota_respons.h"
#include "manager_net_messager.h"
#include "manager_queue.h"
#include "net_passthrough.h"
#include "manager_module.h"
#include "manager_logic.h"
#include "manager_logic_cmd.h"
#include "manager_wiota_frame.h"
#include "manager_wiota_attribute.h"
#include "wiota_mqtt.h"

#define NETCARD_NAME "ml305"

#define PUB_ATTR "v1/devices/me/attributes"
#define PUB_STATE "v1/devices/me/telemetry"
#define PUB_IOTE_ATTR "v1/gateway/attributes"
#define PUB_IOTE_STATE "v1/gateway/telemetry"

#define TOPIC_PLATFORM_RESPONSE "v1/devices/me/rpc/response/"
#define TOPIC_REQUEST "v1/devices/me/rpc/request/"

static void *net_passthrough_handle;
static uint32_t s_nRequestId = 1;

int manager_create_passthrough_queue(void)
{
    // create wiota app manager queue.
    net_passthrough_handle = manager_create_queue("net_passthrough", 4, 16, UC_SYSTEM_IPC_FLAG_PRIO);
    if (net_passthrough_handle == RT_NULL)
    {
        rt_kprintf("manager_create_queue error\n");
        return 1;
    }

    return 0;
}

int to_network_data(int cmd, void *data)
{
    int res = 0;
    t_app_passthrough_message *message = rt_malloc(sizeof(t_app_passthrough_message));
    if (RT_NULL == message)
    {
        rt_kprintf("wiota_recv_wiota_data malloc error\n");
        return -12;
    }
    message->cmd = cmd;
    message->data = data;

    res = manager_send_queue(net_passthrough_handle, message, 4);
    if (RT_EOK != res)
    {
        rt_kprintf("send net_passthrough_handle queue is error.res %d\n", res);
        rt_free(message);
        return res;
    }

    return res;
}

char *dec2str(int num, char *str)
{
    if (RT_NULL == str)
    {
        return RT_NULL;
    }
    int i = 0;
    char *p = str;
    if ('\0' != p[0])
    {
        while (*++p != '\0')
            ;
    }
    if (num < 0) // 如果num为负数，将num变正
    {
        num = -num;
        p[i++] = '-';
    }
    do
    {
        p[i++] = num % 10 + 48; // 取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0'
        num /= 10;              // 去掉最低位
    } while (num);              // num不为0继续循环
    p[i] = '\0';

    int j = 0;
    if (p[0] == '-') // 如果有负号，负号不用调整
    {
        j = 1; // 从第二位开始调整
        ++i;   // 由于有负号，所以交换的对称轴也要后移1位
    }
    // 对称交换
    for (; j < (i >> 1); j++)
    {
        // 对称交换两端的值 其实就是省下中间变量交换a+b的值：a=a+b;b=a-b;a=a-b;
        p[j] = p[j] + p[i - 1 - j];
        p[i - 1 - j] = p[j] - p[i - 1 - j];
        p[j] = p[j] - p[i - 1 - j];
    }
    return str;
}

const char *find_last_slash(const char *strIn, int nLen)
{
    if ((RT_NULL == strIn) || ('\0' == strIn[0]) || (nLen <= 0))
    {
        return RT_NULL;
    }
    do
    {
        if (nLen > 0)
        {
            nLen--;
        }
        else
        {
            break;
        }
    } while ('/' != strIn[nLen]);
    return nLen ? &strIn[nLen + 1] : RT_NULL;
}

void *parse_mqtt_payload(cJSON *payload, int request_number)
{
    if (RT_NULL == payload)
    {
        return RT_NULL;
    }
    const cJSON *param = cJSON_GetObjectItemCaseSensitive(payload, "params");
    if (RT_NULL == param)
    {
        rt_kprintf("parse_mqtt_payload no param.\n");
        return RT_NULL;
    }
    // get "dest_address" : [111, 222, ...]
    unsigned int dest_len = 0;
    unsigned int *dest_address = RT_NULL;
    const cJSON *dest_addr_arr = cJSON_GetObjectItemCaseSensitive(param, "dest_address");
    if (cJSON_IsArray(dest_addr_arr) &&
        (dest_len = cJSON_GetArraySize(dest_addr_arr)))
    {
        const cJSON *dest_addr;
        int i = 0;
        dest_address = (unsigned int *)rt_calloc(sizeof(unsigned int), dest_len);
        if (RT_NULL != dest_address)
        {
            cJSON_ArrayForEach(dest_addr, dest_addr_arr)
            {
                if (cJSON_IsNumber(dest_addr))
                {
                    dest_address[i] = (unsigned int)dest_addr->valuedouble;
                    i++;
                    if (i >= dest_len)
                    {
                        break;
                    }
                }
            }
        }
    }
    else if (cJSON_IsNumber(dest_addr_arr))
    {
        dest_len = 1;
        dest_address = (unsigned int *)rt_calloc(sizeof(unsigned int), dest_len);
        dest_address[0] = (unsigned int)dest_addr_arr->valuedouble;
    }

    // get "cmd" : 111
    int cmd = 0;
    const cJSON *cmd_json = cJSON_GetObjectItemCaseSensitive(param, "cmd");
    if (cJSON_IsNumber(cmd_json))
    {
        cmd = cmd_json->valueint;
        rt_kprintf("parse_mqtt_payload cmd = %d\n", cmd);
    }
    // get "data" : {}/[] and so on
    unsigned char *json_data = RT_NULL;
    const cJSON *data_j = cJSON_GetObjectItemCaseSensitive(param, "data");
    char *json_print = cJSON_Print(data_j);

    if (RT_NULL != json_print)
    {
        rt_kprintf("get the data %s\n", json_print);
        json_data = (unsigned char *)json_print;
    }
    t_from_network_message *wiota_logic_info = rt_malloc(sizeof(t_from_network_message));
    MEMORY_ASSERT(wiota_logic_info);
    wiota_logic_info->dest_len = dest_len * sizeof(unsigned int);
    wiota_logic_info->dest_address = dest_address;
    wiota_logic_info->cmd = cmd;
    wiota_logic_info->request_number = request_number;
    wiota_logic_info->json_data = (unsigned char *)json_data;
    return (void *)wiota_logic_info;
}

static int uc_mqtt_msg_process(MqttClient *client, MqttMessage *msg,
                               byte msg_new, byte msg_done)
{
    char *topic = RT_NULL;
    char *payload = RT_NULL;
    void *data = RT_NULL;
    int nCmd = 0;
    word32 nTopicLen = 0;
    word32 nPayloadLen = 0;
    MQTTCtx *mqttCtx = (MQTTCtx *)client->ctx;
    int nId = -1;
    // rt_kprintf("11111\n");

    (void)mqttCtx;

    if (msg_new)
    {
        // rt_kprintf("22222\n");
        /* Determine min size to dump */
        nTopicLen = msg->topic_name_len;
        if (nTopicLen > MQTT_TOPIC_MAX_LENGTH)
        {
            nTopicLen = MQTT_TOPIC_MAX_LENGTH;
        }
        topic = (char *)rt_malloc(nTopicLen + 1);
        RT_ASSERT(topic);
        XMEMCPY(topic, msg->topic_name, nTopicLen);
        topic[nTopicLen] = '\0'; /* Make sure its null terminated */
        const char *strId = find_last_slash(topic, nTopicLen);
        if (RT_NULL != strId)
        {
            nId = atoi(strId);
        }

        /* Print incoming message */
        PRINTF("MQTT Message: Topic %s, Qos %d, Len %u",
               topic, msg->qos, msg->total_len);
    }

    //rt_kprintf("333333\n");
    /* Print message payload */
    nPayloadLen = msg->buffer_len;
    if (nPayloadLen > MQTT_PAYLOAD_MAX_LENGTH)
    {
        nPayloadLen = MQTT_PAYLOAD_MAX_LENGTH;
    }
    payload = (char *)rt_malloc(nPayloadLen + 1);
    RT_ASSERT(payload);
    XMEMCPY(payload, msg->buffer, nPayloadLen);
    payload[nPayloadLen] = '\0'; /* Make sure its null terminated */
    PRINTF("Payload (%d - %d): %s",
           msg->buffer_pos, msg->buffer_pos + nPayloadLen, payload);
    do
    {
        if (!msg_done)
        {
            break;
        }
        t_app_mqtt_request *req = (t_app_mqtt_request *)rt_malloc(sizeof(t_app_mqtt_request));
        req->nId = nId;
        req->payload = (char *)malloc(nPayloadLen + 1);
        RT_ASSERT(req->payload);
        rt_memcpy(req->payload, payload, nPayloadLen);
        req->payload[nPayloadLen] = '\0';
        data = (void *)req;
        if (!rt_strncmp(topic, TOPIC_REQUEST, rt_strlen(TOPIC_REQUEST)))
        {
            nCmd = MANAGER_LOGIC_MQTT_REQUEST;
        }
        else if (!rt_strncmp(topic, TOPIC_PLATFORM_RESPONSE, rt_strlen(TOPIC_PLATFORM_RESPONSE)))
        {
            nCmd = MANAGER_LOGIC_MQTT_RESPONSE;
        }
        if (0 != to_network_data(nCmd, data))
        {
            rt_free(req->payload);
            rt_free(req);
        }
        PRINTF("MQTT Message: Done");
    } while (0);

    if (RT_NULL != topic)
    {
        rt_free(topic);
        topic = RT_NULL;
    }
    if (RT_NULL != payload)
    {
        rt_free(payload);
        payload = RT_NULL;
    }
    // rt_kprintf("4444444\n");

    /* Return negative to terminate publish processing */
    return MQTT_CODE_SUCCESS;
}

void passthrough_manager_task(void *pPara)
{
    t_app_passthrough_message *page;
    void *app_mqtt_wait_handle = RT_NULL;
    int nQueueRes = 0;

    while (uc_find_netcard(NETCARD_NAME))
        ;

    rt_kprintf("find out the netcard\n");

    while (1)
    {
        rt_kprintf("ready connect the mqtt server\n");
        uc_mqtt_connect(uc_mqtt_msg_process);

        rt_kprintf("connect the mqtt server success\n");

        // subscribe or publish
        int nRet = 0;
        // nRet = uc_mqtt_subscribe("mqtt_uc/sub1", 0);
        // rt_kprintf("subscribe state = %d\n", nRet);
        nRet = uc_mqtt_subscribe(TOPIC_REQUEST "+", 0);
        rt_kprintf("subscribe request state = %d\n", nRet);
        // nRet = uc_mqtt_subscribe(TOPIC_PLATFORM_RESPONSE"+", 0);
        // rt_kprintf("subscribe platform response state = %d\n", nRet);

        if (0 != manager_thread_create_task(&app_mqtt_wait_handle, "app_mqtt_wait_handle", uc_mqtt_wait_msg, RT_NULL, 512, 3, 5))
        {
            rt_kprintf("custom_manager_task create error\n");
            return;
        }
        rt_thread_startup((rt_thread_t)app_mqtt_wait_handle);

        while (uc_mqtt_get_state() == STATE_CONNECT)
        {
            nQueueRes = manager_recv_queue(net_passthrough_handle, (void *)&page, 500);
            if ((QUEUE_EOK != nQueueRes) || (RT_NULL == page))
            {
                continue;
            }
            rt_kprintf("passthrough_manager_task page->cmd [0x%x ( %d )]\n", page->cmd, page->cmd);
            switch (page->cmd)
            {
            case APP_CMD_IOTE_PROPERTY_REPORT:
            {
                cmd_iote_property_report(page);
                break;
            }
            case APP_CMD_AP_PROPERTY_REPORT:
            {
                cmd_ap_property_report(page);
                break;
            }
            case APP_CMD_IOTE_STATE_REPORT:
            {
                cmd_iote_state_report(page);
                break;
            }
            case APP_CMD_IOTE_PAIR_REQUEST:
            case APP_CMD_UPDATE_VERSION_REQUEST:
            case APP_CMD_IOTE_MUTICAST_REQUEST:
            {
                /*
                        typedef struct app_passthrough_message
                        {
                            int cmd;
                            void *data;------------------------------> typedef struct app_logic_to_net
                                                                                                    {
                                                                                                        unsigned int src_addr;
                                                                                                        unsigned int data_len;
                                                                                                        void *data; ---------------------> malloc data
                                                                                                    } t_app_logic_to_net;
                        } t_app_passthrough_message;
                    */
                cmd_iote_request(page);
                break;
            }
            case APP_CMD_IOTE_REQUEST_NEW_CONFIG:
            case WIOTA_AP_REQUEST_RESERVE_ADDR:
            {
                cmd_ap_request_reserve_addr(page);
                break;
            }
            case MANAGER_LOGIC_MQTT_REQUEST:
            {
                cmd_mqtt_request_or_response(page);
                break;
            }
            case MANAGER_LOGIC_MQTT_RESPONSE:
            {
                cmd_mqtt_request_or_response(page);
                break;
            }
            case MANAGER_LOGIC_MQTT_EXE_RESPONSE:
            {
                /*
                    typedef struct app_passthrough_message
                    {
                        int cmd;
                        void *data;--------------------->    typedef struct app_logic_to_net
                                                                                    {
                                                                                        unsigned int src_addr;
                                                                                        unsigned int data_len;
                                                                                        void *data; -----------------> typedef struct net_messager_response_result
                                                                                                                                            {
                                                                                                                                                unsigned int response_number;
                                                                                                                                                int result;
                                                                                                                                            } t_net_messager_response;
                                                                                    } t_app_logic_to_net;
                    } t_app_passthrough_message;
                    */
                cm_mqtt_exe_response(page);
            }
            break;
            }

            if (RT_NULL != page)
            {
                // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
                if (RT_NULL != page->data)
                {
                    //    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
                    rt_free(page->data);
                    page->data = RT_NULL;
                }
                // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
                rt_free(page);
                // rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
            }
        }

        // manager_thread_del(app_mqtt_wait_handle);
        rt_free(((rt_thread_t)app_mqtt_wait_handle)->stack_addr);
        rt_free(app_mqtt_wait_handle);
        app_mqtt_wait_handle = RT_NULL;

        uc_mqtt_disconnect();
        rt_kprintf("disconnect, ready to next...\n");
        rt_thread_mdelay(2000);
    }
}

// the following are command processing fuctions

void cmd_iote_property_report(t_app_passthrough_message *page)
{
    t_app_logic_to_net *cmd_data = (t_app_logic_to_net *)page->data;
    char *payload = rt_calloc(1, cmd_data->data_len * 4);
    MEMORY_ASSERT(payload);

    rt_memcpy(payload, "{\"sub_client_", rt_strlen("{\"sub_client_"));
    dec2str(cmd_data->src_addr, payload);
    rt_memcpy(payload + rt_strlen(payload), "\":", rt_strlen("\":"));
    rt_memcpy(payload + rt_strlen(payload), cmd_data->data, rt_strlen((char *)cmd_data->data));
    rt_memcpy(payload + rt_strlen(payload), "}", rt_strlen("}"));
    uc_mqtt_publish(PUB_IOTE_ATTR, (uint8_t *)payload, rt_strlen(payload), 0, 0, 0);

    rt_free(payload);
    free(cmd_data->data);
}

void cmd_ap_property_report(t_app_passthrough_message *page)
{
    cJSON *json_data = page->data;
    char *payload = cJSON_Print(json_data);
    if (RT_NULL != payload)
    {
        uc_mqtt_publish(PUB_ATTR, (uint8_t *)payload, rt_strlen(payload), 0, 0, 0);
        cJSON_free(payload);
    }
    cJSON_Delete(page->data);
    page->data = RT_NULL;
}

void cmd_iote_state_report(t_app_passthrough_message *page)
{
    t_app_logic_to_net *cmd_data = (t_app_logic_to_net *)page->data;

    //timeval_custom tv;
    //gettimeofday(&tv.tv, RT_NULL);
    // rt_device_t dev = rt_device_find(NETCARD_NAME);
    // rt_device_control(dev, AT_DEVICE_CTRL_GET_TIME, &tv.tv)

    cJSON *json_payload = cJSON_CreateObject();
    char sub_client[30] = {"sub_client_\0"};
    dec2str(cmd_data->src_addr, sub_client);
    cJSON *json_sub_client = cJSON_CreateArray();
    cJSON_AddItemToObject(json_payload, sub_client, json_sub_client);

    cJSON *json_sub_client_child_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_sub_client_child_obj, "ts", 50 /*tv.u64*/);

    //cJSON_AddStringToObject(json_sub_client_child_obj, "values", cJSON_Parse(cmd_data->data)/*cmd_data->data*/);

    cJSON_AddItemToObject(json_sub_client_child_obj, "values", cJSON_Parse(cmd_data->data));

    cJSON_AddItemToArray(json_sub_client, json_sub_client_child_obj);

    char *payload = cJSON_Print(json_payload);

    // rt_snprintf(payload, cmd_data->data_len * 3, "{\"sub_client_%d\":[{\"ts\":1121564444,\"values\":%s}]}",
    // cmd_data->src_addr, (char *)cmd_data->data);
    if (RT_NULL != payload)
    {
        uc_mqtt_publish(PUB_IOTE_STATE, (uint8_t *)payload, rt_strlen(payload), 0, 0, 0);
        cJSON_free(payload);
    }
    cJSON_Delete(json_payload);
    rt_free(cmd_data->data);
    cmd_data->data = RT_NULL;
}

void cmd_iote_request(t_app_passthrough_message *page)
{
    cJSON *json_data = RT_NULL;
    cJSON *json_payload = RT_NULL;
    char *json_string = RT_NULL;
    // char *json_tmp = RT_NULL;

    t_app_logic_to_net *cmd_data = (t_app_logic_to_net *)page->data;
    // json_string = (char *)rt_calloc(1, MQTT_PAYLOAD_MAX_LENGTH);
    char *topic = (char *)rt_calloc(1, MQTT_TOPIC_MAX_LENGTH);
    MEMORY_ASSERT(topic);

    json_payload = cJSON_CreateObject();
    cJSON_AddStringToObject(json_payload, "method", "open");

    json_data = cJSON_Parse(cmd_data->data);
    MEMORY_ASSERT(json_data);
    cJSON_AddNumberToObject(json_data, "cmd", page->cmd);
    cJSON_AddNumberToObject(json_data, "src_address", cmd_data->src_addr);

    cJSON_AddItemToObject(json_payload, "params", json_data);
    json_string = cJSON_Print(json_payload);
    MEMORY_ASSERT(json_string);

    // json_tmp = cJSON_Print(json_data);
    // MEMORY_ASSERT(json_tmp);
    // rt_sprintf(json_string, "{\"method\":\"open\",\"params\":%s}", json_tmp);

    rt_memcpy(topic, TOPIC_REQUEST, rt_strlen(TOPIC_REQUEST));
    dec2str(s_nRequestId++, topic);
    uc_mqtt_publish(topic, (uint8_t *)json_string, rt_strlen(json_string), 0, 0, 0);

    cJSON_Delete(json_payload);
    rt_free(json_string);

    rt_free(topic);
    rt_free(cmd_data->data);
    cmd_data->data = RT_NULL;
}

void cmd_ap_request_reserve_addr(t_app_passthrough_message *page)
{
    char *strPayload;
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *json_data = page->data;

    char *topic = (char *)rt_calloc(1, MQTT_TOPIC_MAX_LENGTH);
    RT_ASSERT(topic);
    rt_memcpy(topic, TOPIC_REQUEST, rt_strlen(TOPIC_REQUEST));
    dec2str(s_nRequestId++, topic);

    cJSON_AddStringToObject(root, "method", "test");

    cJSON_AddNumberToObject(params, "cmd", page->cmd); // WIOTA_AP_REQUEST_RESERVE_ADDR
    cJSON_AddItemToObject(params, "data", json_data);

    cJSON_AddItemToObject(root, "params", params);
    strPayload = cJSON_Print(root);
    if (NULL != strPayload)
    {
        // push data
        uc_mqtt_publish(topic, (uint8_t *)strPayload, rt_strlen(strPayload), 0, 0, 0);
        cJSON_free(strPayload);
    }
    cJSON_Delete(root);
    page->data = RT_NULL;
    rt_free(topic);
}

void cmd_mqtt_request_or_response(t_app_passthrough_message *page)
{
    void *wiota_logic_info;
    int request_number = 0;
    t_app_mqtt_request *req = (t_app_mqtt_request *)page->data;

    cJSON *payload = cJSON_Parse(req->payload);
    MEMORY_ASSERT(payload);
    if (MANAGER_LOGIC_MQTT_RESPONSE == page->cmd)
        request_number = 0;
    else
        request_number = req->nId;

    wiota_logic_info = parse_mqtt_payload(payload, request_number);
    if (RT_NULL != wiota_logic_info)
    {
        to_queue_logic_data(MANAGER_NETWORK_INDENTIFICATION,
                            MANAGER_LOGIC_FROM_NETWORK_MESSAGE,
                            (void *)wiota_logic_info);
    }
    if (RT_NULL != payload)
    {
        cJSON_Delete(payload);
    }
    rt_free(req->payload);
}

void cm_mqtt_exe_response(t_app_passthrough_message *page)
{
    char *topic;
    int rst = 0;
    char *res_ok = "{\"res\":\"ok\"}";
    char *res_err = "{\"res\":\"err\"}";
    t_app_logic_to_net *page_data = page->data;
    t_net_messager_response *response = page_data->data;

    if (0 != response->response_number)
    {
        topic = (char *)rt_calloc(1, MQTT_TOPIC_MAX_LENGTH);
        RT_ASSERT(topic);
        rt_memcpy(topic, TOPIC_PLATFORM_RESPONSE, rt_strlen(TOPIC_PLATFORM_RESPONSE));
        dec2str(response->response_number, topic);
        if (MANAGER_SEND_SUCCESS == response->result)
            rst = uc_mqtt_publish(topic, (uint8_t *)res_ok, rt_strlen(res_ok), 0, 0, 0);
        else
            rst = uc_mqtt_publish(topic, (uint8_t *)res_err, rt_strlen(res_err), 0, 0, 0);
        rt_kprintf("publish state = %d\n", rst);
        rt_free(topic);
    }

    if (RT_NULL != response)
    {
        rt_free(response);
        response = RT_NULL;
    }
}

#endif
