/**
 * Created on Jul 27 2015 by LinhNV (nvl1109@gmail.com)
 */
#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "debug.h"
#include "user_config.h"
#include "config.h"
#include "protocol_mqtt.h"

static protocol_callback mqtt_connectedcb = NULL;
static protocol_callback mqtt_disconnectedcb = NULL;
static protocol_callback mqtt_sentcb = NULL;
static protocol_callback mqtt_receivedcb = NULL;

protocol_mqtt_t * ICACHE_FLASH_ATTR mqtt_create_client(char * name, cJSON * config) {
    protocol_mqtt_t * result = NULL;
    cJSON * jtmp = NULL;
    char * pstr = NULL;
    MQTT_Client * mqtt_client = NULL;

    DBG_MSG("name: %s, config: %p\r\n", name, config);
    // Validate params
    if (config == NULL) goto done;
    if (name == NULL) goto done;

    result = (protocol_mqtt_t *)os_malloc(sizeof(protocol_mqtt_t));
    DBG_MSG("result: %p\r\n", result);
    if (result == NULL) goto err1;
    result->type = PROTOCOL_MQTT;
    result->clientid = name;

    // Server
    jtmp = cJSON_GetObjectItem(config, PMQTT_CFG_SERVER_STR);
    DBG_MSG("%s-jtmp:%p\r\n", PMQTT_CFG_SERVER_STR, jtmp);
    if (jtmp == NULL) goto err1;
    pstr = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
    if (pstr == NULL) goto err1;
    os_memset(pstr, 0, os_strlen(jtmp->valuestring) + 1);
    os_memcpy(pstr, jtmp->valuestring, os_strlen(jtmp->valuestring));
    result->server = pstr;
    DBG_MSG("result->server: %s\r\n", result->server);

    // port
    jtmp = cJSON_GetObjectItem(config, PMQTT_CFG_PORT_STR);
    DBG_MSG("%s-jtmp:%p\r\n", PMQTT_CFG_PORT_STR, jtmp);
    if (jtmp == NULL)
        result->port = PMQTT_DEFAULT_PORT;
    else
        result->port = jtmp->valueint;
    DBG_MSG("result->port: %d\r\n", result->port);

    // user
    jtmp = cJSON_GetObjectItem(config, PMQTT_CFG_USER_STR);
    DBG_MSG("%s-jtmp:%p\r\n", PMQTT_CFG_USER_STR, jtmp);
    if (jtmp != NULL) {
        pstr = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
        if (pstr == NULL) goto err1;
        os_memset(pstr, 0, os_strlen(jtmp->valuestring) + 1);
        os_memcpy(pstr, jtmp->valuestring, os_strlen(jtmp->valuestring));
    } else {
        pstr = (char *)os_malloc(2);
        if (pstr == NULL) goto err1;
        os_memset(pstr, 0, 2);
    }
    result->user = pstr;
    DBG_MSG("result->user: %s\r\n", result->user);

    // pass
    jtmp = cJSON_GetObjectItem(config, PMQTT_CFG_PASS_STR);
    DBG_MSG("%s-jtmp:%p\r\n", PMQTT_CFG_PASS_STR, jtmp);
    if (jtmp != NULL) {
        pstr = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
        if (pstr == NULL) goto err1;
        os_memset(pstr, 0, os_strlen(jtmp->valuestring) + 1);
        os_memcpy(pstr, jtmp->valuestring, os_strlen(jtmp->valuestring));
    } else {
        pstr = (char *)os_malloc(2);
        if (pstr == NULL) goto err1;
        os_memset(pstr, 0, 2);
    }
    result->pass = pstr;
    DBG_MSG("result->pass: %s\r\n", result->pass);

    // timeout
    jtmp = cJSON_GetObjectItem(config, PMQTT_CFG_TIMEOUT_STR);
    DBG_MSG("%s-jtmp:%p\r\n", PMQTT_CFG_TIMEOUT_STR, jtmp);
    if (jtmp == NULL)
        result->timeout = PQMTT_DEFAULT_TIMEOUT;
    else
        result->timeout = jtmp->valueint;
    DBG_MSG("result->timeout: %d\r\n", result->timeout);

    // Ok, information enough, now create MQTT client
    mqtt_client = (MQTT_Client *)os_malloc(sizeof(MQTT_Client));
    DBG_MSG("mqtt_client:%p\r\n", mqtt_client);
    if (mqtt_client == NULL) goto err1;
    os_memset(mqtt_client, 0, sizeof(MQTT_Client));
    MQTT_InitConnection(mqtt_client, result->server, result->port, 0);
    MQTT_InitClient(mqtt_client, result->clientid, result->user, result->pass, result->timeout, 1);
    MQTT_InitLWT(mqtt_client, "/lwt", "offline", 0, 0);
    result->client = mqtt_client;

    // Everything is ok, return
    goto done;
err1:
    DBG_MSG("ERROR OCCUR!!!\r\n");
    if ((result) && (result->server)) os_free(result->server);
    if ((result) && (result->user)) os_free(result->user);
    if ((result) && (result->pass)) os_free(result->pass);
    if ((result) && (result->client)) os_free(result->client);
    if (result) os_free(result);
    if (mqtt_client) os_free(mqtt_client);
    result = NULL;

done:
    return result;
}

void ICACHE_FLASH_ATTR mqtt_client_free(protocol_mqtt_t * pproto) {
    if (pproto->client) {
        DBG_MSG("Disconnect and free the MQTT client\r\n");
        MQTT_Disconnect(pproto->client);
        os_free(pproto->client);
    }
    if (pproto->pass) {
        DBG_MSG("Free MQTT password\r\n");
        os_free(pproto->pass);
    }
    if (pproto->user) {
        DBG_MSG("Free MQTT user\r\n");
        os_free(pproto->user);
    }
    if (pproto->server) {
        DBG_MSG("Free MQTT server\r\n");
        os_free(pproto->server);
    }
    if (pproto->clientid) {
        DBG_MSG("Free MQTT clientid\r\n");
        os_free(pproto->clientid);
    }
    if (pproto) {
        DBG_MSG("Free MQTT protocol struct\r\n");
        os_free(pproto);
    }
}

bool ICACHE_FLASH_ATTR mqtt_match_client(protocol_mqtt_t * pproto, char * search_str) {
    cJSON * jroot = NULL;
    cJSON * jtmp = NULL;
    bool result = true;
    uint8_t i;

    DBG_MSG("search_str:%s\r\n", search_str);
    if (search_str == NULL) return false;

    jroot = cJSON_Parse(search_str);
    DBG_MSG("jroot: %p\r\n", jroot);
    if (jroot == NULL) return false;

    for (i=0; i<cJSON_GetArraySize(jroot); ++i) {
        jtmp = cJSON_GetArrayItem(jroot, i);
        // key: jtmp->string
        // value: jtmp->valuestring
        if (strcmp(PMQTT_CFG_SERVER_STR, jtmp->string) == 0) {
            // Compare by server url
            DBG_MSG("compare %s.\r\n", PMQTT_CFG_SERVER_STR);
            if (strcmp(pproto->server, jtmp->valuestring) != 0) {
                result = false;
                break;
            }
        } else if (strcmp(PMQTT_CFG_PORT_STR, jtmp->string) == 0) {
            // Compare by port number
            DBG_MSG("compare %s.\r\n", PMQTT_CFG_PORT_STR);
            if (pproto->port != jtmp->valueint) {
                result = false;
                break;
            }
        } else if (strcmp(PMQTT_CFG_USER_STR, jtmp->string) == 0) {
            // Compare by user name
            DBG_MSG("compare %s.\r\n", PMQTT_CFG_USER_STR);
            if (strcmp(pproto->user, jtmp->valuestring) != 0) {
                result = false;
                break;
            }
        } else if (strcmp(PMQTT_CFG_PASS_STR, jtmp->string) == 0) {
            // Compare by password
            DBG_MSG("compare %s.\r\n", PMQTT_CFG_PASS_STR);
            if (strcmp(pproto->pass, jtmp->valuestring) != 0) {
                result = false;
                break;
            }
        }
    }
    DBG_MSG("result: %d\r\n", result);
    if (jroot != NULL) cJSON_Delete(jroot);
    return result;
}

void ICACHE_FLASH_ATTR mqtt_register_callbacks(protocol_callback connectedcb,
                                               protocol_callback disconnectedcb,
                                               protocol_callback receivedcb,
                                               protocol_callback sentcb) {
    mqtt_receivedcb = receivedcb;
    mqtt_connectedcb = connectedcb;
    mqtt_disconnectedcb = disconnectedcb;
    mqtt_sentcb = sentcb;
}
