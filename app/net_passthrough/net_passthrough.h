#ifndef NET_PASSTHROUGH_H_
#define NET_PASSTHROUGH_H_

#include "cJSON.h"
#include <sys/time.h>

#define MQTT_TOPIC_MAX_LENGTH (64)    /* (128) */
#define MQTT_PAYLOAD_MAX_LENGTH (512) /* (2 * 1024) */

typedef struct app_passthrough_message
{
    int cmd;
    void *data;
} t_app_passthrough_message;

typedef struct app_mqtt_request
{
    int nId;
    char *payload;
} t_app_mqtt_request;

typedef struct _timeval_custom
{
    union
    {
        struct timeval tv;
        unsigned long long u64;
    };
} timeval_custom;

char *dec2str(int num, char *str);
const char *find_last_slash(const char *strIn, int nLen);

int manager_create_passthrough_queue(void);
int to_network_data(int cmd, void *data);

void passthrough_manager_task(void *pPara);

// the following are command processing fuctions

void cmd_iote_property_report(t_app_passthrough_message *page);

void cmd_ap_property_report(t_app_passthrough_message *page);

void cmd_iote_state_report(t_app_passthrough_message *page);

void cmd_iote_request(t_app_passthrough_message *page);

void cmd_ap_request_reserve_addr(t_app_passthrough_message *page);

void cmd_mqtt_request_or_response(t_app_passthrough_message *page);

void cm_mqtt_exe_response(t_app_passthrough_message *page);

#endif // #ifndef NET_PASSTHROUGH_H_
