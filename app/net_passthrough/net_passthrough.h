#ifndef NET_PASSTHROUGH_H_
#define NET_PASSTHROUGH_H_

#include "cJSON.h"
#include <sys/time.h>

#define MQTT_TOPIC_MAX_LENGTH (128)
#define MQTT_PAYLOAD_MAX_LENGTH (2 * 1024)

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
}timeval_custom;

char *dec2str(int num, char *str);
const char *find_last_slash(const char *strIn, int nLen);

int manager_create_passthrough_queue(void);
int to_network_data(int cmd, void *data);

void passthrough_manager_task(void *pPara);

#endif // #ifndef NET_PASSTHROUGH_H_
