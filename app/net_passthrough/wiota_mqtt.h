#ifndef WIOTA_MQTT_H
#define WIOTA_MQTT_H

#include <stdint.h>
#include "port/mqttctx.h"
#include "port/mqttnet.h"
#include "wolfmqtt/mqtt_client.h"

typedef enum {
    STATE_DISCONNECT,
    STATE_CONNECT,
}connect_state;

#if defined (__cplusplus)
extern "C" {
#endif

int uc_find_netcard(char *card_name);

connect_state uc_mqtt_get_state(void);

void uc_mqtt_connect(MqttMsgCb msg_cb);
void uc_mqtt_disconnect(void);
int uc_mqtt_subscribe(char *topic, uint8_t nQos);
int uc_mqtt_unsubscribe(char *topic);
int uc_mqtt_publish(char     *topic,
                    uint8_t  *data, 
                    uint16_t nLength, 
                    uint8_t  nQos, 
                    uint8_t  nRetain, 
                    uint8_t  nDuplicate);
void uc_mqtt_wait_msg(void *pPara);

#if defined (__cplusplus)
}
#endif

#endif // #ifndef WIOTA_MQTT_H