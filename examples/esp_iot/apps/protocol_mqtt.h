/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __PROTOCOL_MQTT__H__
#define __PROTOCOL_MQTT__H__

#include "app_types.h"
#include "config.h"
#include "mqtt.h"

#define PMQTT_CFG_SERVER_STR    "server"
#define PMQTT_CFG_PORT_STR      "port"
#define PMQTT_CFG_USER_STR      "user"
#define PMQTT_CFG_PASS_STR      "pass"
#define PMQTT_CFG_TIMEOUT_STR   "timeout"

#define PMQTT_DEFAULT_PORT      (1883)
#define PQMTT_DEFAULT_TIMEOUT   (120)

typedef struct SProtocolMqtt {
    protocol_type_t type;       /**!< Type of protocol - MQTT */
    char * clientid;            /**!< Client ID */
    char * server;              /**!< Server url */
    uint32_t port;              /**!< Server port number */
    char * user;                /**!< User name */
    char * pass;                /**!< Password */
    uint32_t timeout;           /**!< Timeout value */
    MQTT_Client * client;       /**!< Pointer to MQTT client */
} protocol_mqtt_t;

protocol_mqtt_t * ICACHE_FLASH_ATTR mqtt_create_client(char * name, cJSON * config);
void ICACHE_FLASH_ATTR mqtt_client_free(protocol_mqtt_t * pproto);
bool ICACHE_FLASH_ATTR mqtt_match_client(protocol_mqtt_t * pproto, char * search_str);
void ICACHE_FLASH_ATTR mqtt_register_callbacks(protocol_callback connectedcb,
                                               protocol_callback disconnectedcb,
                                               protocol_callback receivedcb,
                                               protocol_callback sentcb);

#endif
