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
#include "protocol_rest.h"
#include "protocol_manager.h"
#include "app_manager.h"
#include "mqtt.h"

protocol_t protocol_list[MAX_NUMBER_OF_PROTOCOL];
uint8_t protocol_idx = 0;

/********PROTOTYPES*************/
static void ICACHE_FLASH_ATTR protocol_connected_cb(protocol_t *protocol);
static void ICACHE_FLASH_ATTR protocol_disconnected_cb(protocol_t *protocol);
static void ICACHE_FLASH_ATTR protocol_received_cb(protocol_t *protocol, char *url, char *data);
static void ICACHE_FLASH_ATTR protocol_sent_cb(protocol_t *protocol);
/******MQTT*********/
static void ICACHE_FLASH_ATTR mqtt_connected_cb(uint32_t *args);
static void ICACHE_FLASH_ATTR mqtt_disconnected_cb(uint32_t *args);
static void ICACHE_FLASH_ATTR mqtt_sent_cb(uint32_t *args);
static void ICACHE_FLASH_ATTR mqtt_received_cb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t length);
/******END***********/

protocol_t * ICACHE_FLASH_ATTR protocol_create(protocol_type_t type, char * name, cJSON * config) {
    protocol_t * result = NULL;

    DBG_MSG("type: %d, name: %s, config: %p\r\n", type, name, config);
    if (protocol_idx == 0) {
        DBG_MSG("Zero the protocol list %p:%d\r\n", protocol_list, sizeof(protocol_list));
        os_memset(protocol_list, 0, sizeof(protocol_list));
    }

    if (protocol_idx >= (MAX_NUMBER_OF_PROTOCOL - 1)) return NULL;
    if ((type <= PROTOCOL_INVALID) || (type >= PROTOCOL_LAST)) return NULL;
    if (config == NULL) return NULL;
    if (name == NULL) return NULL;

    switch(type) {
        protocol_mqtt_t *pmqtt_proto = NULL;
        case PROTOCOL_MQTT:
        pmqtt_proto = mqtt_create_client(name, config);
        DBG_MSG("pmqtt_proto: %p\r\n", pmqtt_proto);
        if (pmqtt_proto == NULL) goto err1;
        DBG_MSG("protocol_idx: %d\r\n", protocol_idx);
        protocol_list[protocol_idx].type = PROTOCOL_MQTT;
        protocol_list[protocol_idx].pprotocol = (void *)pmqtt_proto;
        protocol_list[protocol_idx].app_count = 0;
        os_memset(protocol_list[protocol_idx].apps, 0, sizeof(protocol_list[protocol_idx].apps));
        result = &protocol_list[protocol_idx];
        protocol_idx++;
        break;

        default:
        goto err1;
        break;
    }

    // Everything is ok, return
    goto done;

err1:
    DBG_MSG("Error occur!!!");
    result = NULL;

done:
    return result;
}

void ICACHE_FLASH_ATTR protocol_destroy(protocol_t * pproto) {
    // TODO: implement later
}

protocol_t * ICACHE_FLASH_ATTR protocol_find(protocol_type_t type, char * search_str) {
    uint8_t i;

    DBG_MSG("type: %d, search_str: %s\r\n", type, search_str);
    DBG_MSG("Current protocol_idx=%d, %p\r\n", protocol_idx, protocol_list);
    if (protocol_idx == 0) return NULL;

    for (i=0; i<protocol_idx; ++i) {
        // Find matches protocol
        if (protocol_list[i].type != type) continue;

        if (type == PROTOCOL_MQTT) {
            // Search MQTT
            if (mqtt_match_client((protocol_mqtt_t *)protocol_list[i].pprotocol, search_str))
                return &protocol_list[i];
        } else if (type == PROTOCOL_RESTFUL) {
            // Search by rest
        } else if (type == PROTOCOL_HTTP) {
            // Search by http
        }
    }
    return NULL;
}

bool ICACHE_FLASH_ATTR protocol_send(protocol_t *protocol, char *url, char * data) {
    DBG_MSG("protocol: %p, url: %s, data: %s\r\n", protocol, url, data);

    if (protocol->type == PROTOCOL_MQTT) {
        protocol_mqtt_t *mqtt = (protocol_mqtt_t *)protocol->pprotocol;
        MQTT_Client * mqtt_client = mqtt->client;
        DBG_MSG("protocol->status: %d\r\n", protocol->status);
        protocol->status = PROTO_STATUS_SENDING;

        return MQTT_Publish(mqtt_client, url, data, os_strlen(data), 0, 0);
    }

    return false;
}

void * ICACHE_FLASH_ATTR protocol_find_app(protocol_t * pproto, char * appid) {
    uint8_t i = 0;
    DBG_MSG("pproto: %p, id: %s\r\n", pproto, appid);
    if ((pproto == NULL) || (appid == NULL)) return NULL;

    DBG_MSG("pproto->app_count: %d\r\n", pproto->app_count);
    for (i=0; i<pproto->app_count; ++i) {
        app_t *app = (app_t *) pproto->apps[i];
        DBG_MSG("app: %p\r\n", app);
        if (app == NULL) continue;
        if (app_match(app, appid) == APP_STATUS_OK) return app;
    }

    return NULL;
}

bool ICACHE_FLASH_ATTR protocol_add_app(protocol_t * pproto, void * app) {
    void * tmp = NULL;
    char * id = NULL;
    app_status_t res = APP_STATUS_OK;
    DBG_MSG("pproto: %p, app: %p\r\n", pproto, app);
    if ((pproto == NULL) || (app == NULL)) return false;
    DBG_MSG("app->app: %p\r\n", (app_t *)app);
    DBG_MSG("pproto->app_count: %d\r\n", pproto->app_count);
    if (pproto->app_count >= MAX_APP_PER_PROTOCOL) return false;

    res = app_get_id((app_t *)app, &id);
    DBG_MSG("res: %d, id: %s\r\n", res, id);
    tmp = protocol_find_app(pproto, id);
    DBG_MSG("app exists, tmp: %p\r\n", tmp);
    if (tmp != NULL) return true;

    // Add
    switch (((app_t *)app)->type) {
        case APP_LAMP:
            ((app_lamp_t *)((app_t *)app)->app)->protocol = (void *)pproto;
        break;
        case APP_SWITCH:
            ((app_switch_t *)((app_t *)app)->app)->protocol = (void *)pproto;
        break;
        case APP_SWITCHLAMP:
            ((app_switchlamp_t *)((app_t *)app)->app)->protocol = (void *)pproto;
        break;
        default:
        DBG_MSG("ERR: Invalid app type");
        break;
    }
    pproto->apps[pproto->app_count] = app;
    pproto->app_count ++;
    DBG_MSG("pproto->apps: %p, pprotol->app_count: %d\r\n", pproto->apps[pproto->app_count - 1], pproto->app_count);

    return true;
}

bool ICACHE_FLASH_ATTR protocol_connect(protocol_t * pproto) {
    protocol_mqtt_t * mqtt_proto = NULL;
    DBG_MSG("pproto: %p\r\n", pproto);
    if (pproto == NULL) return false;

    switch(pproto->type) {
        case PROTOCOL_MQTT:
            mqtt_proto = (protocol_mqtt_t *)pproto->pprotocol;
            // Assign callbacks
            MQTT_OnConnected(mqtt_proto->client, mqtt_connected_cb);
            MQTT_OnDisconnected(mqtt_proto->client, mqtt_disconnected_cb);
            MQTT_OnPublished(mqtt_proto->client, mqtt_sent_cb);
            MQTT_OnData(mqtt_proto->client, mqtt_received_cb);
            MQTT_Connect(mqtt_proto->client);
            return true;
        break;
        default:
        break;
    }

    return false;
}

bool ICACHE_FLASH_ATTR protocol_disconnect(protocol_t * pproto) {
    protocol_mqtt_t * mqtt_proto = NULL;
    DBG_MSG("pproto: %p\r\n", pproto);
    if (pproto == NULL) return false;

    switch(pproto->type) {
        case PROTOCOL_MQTT:
            mqtt_proto = (protocol_mqtt_t *)pproto->pprotocol;
            // Disconnect
            MQTT_Disconnect(mqtt_proto->client);
            return true;
        break;
        default:
        break;
    }

    return false;
}

protocol_t * ICACHE_FLASH_ATTR protocol_find_by_mqtt_client(void * client) {
    uint8_t i;

    DBG_MSG("client: %p\r\n", client);
    for (i=0; i<protocol_idx; ++i) {
        protocol_mqtt_t *mqtt_proto = NULL;
        if (protocol_list[i].type != PROTOCOL_MQTT) continue;
        mqtt_proto = (protocol_mqtt_t *)protocol_list[i].pprotocol;
        if (mqtt_proto == NULL) continue;
        if (mqtt_proto->client == client) {
            DBG_MSG("Found protocol %p at %d\r\n", &protocol_list[i], i);
            return &protocol_list[i];
        }
    }
    return NULL;
}

bool ICACHE_FLASH_ATTR protocol_register_receive(protocol_t *pproto, void *app) {
    DBG_MSG("pproto: %p, app: %p\r\n", pproto, app);
    if((pproto == NULL) || (app == NULL)) return false;

    switch (pproto->type) {
        char *url = NULL;
        case PROTOCOL_MQTT:
            switch(((app_t *)app)->type) {
                protocol_mqtt_t *mqtt_proto = (protocol_mqtt_t *)(pproto->pprotocol);
                case APP_LAMP:
                    url = ((app_lamp_t *)(((app_t *)app)->app))->url;
                break;
                case APP_SWITCH:
                    url = ((app_switch_t *)(((app_t *)app)->app))->url;
                break;
                case APP_SWITCHLAMP:
                    url = ((app_switchlamp_t *)(((app_t *)app)->app))->url;
                break;
                default:
                break;
            }
            DBG_MSG("url ptr: %p\r\n", url);
            if (url != NULL) {
                DBG_MSG("url: %s\r\n", url);
                return MQTT_Subscribe(((protocol_mqtt_t *)(pproto->pprotocol))->client, url, 0);
            }
        break;
        case PROTOCOL_RESTFUL:
        break;
        default:
        break;
    }

    return false;
}

static void ICACHE_FLASH_ATTR protocol_connected_cb(protocol_t *protocol) {
    uint8_t i;
    DBG_MSG("protocol: %p\r\n", protocol);

    if (protocol->app_count == 0) return;
    for (i=0; i<protocol->app_count; ++i) {
        app_t *app = (app_t *)protocol->apps[i];
        DBG_MSG("app: %p, type: %d\r\n", app, app->type);
        switch (app->type) {
            case APP_LAMP:
                DBG_MSG("online lamp: %p\r\n", app->app);
                ((app_lamp_t *)app->app)->online((void *)app);
            break;
            case APP_SWITCH:
                DBG_MSG("online switch: %p\r\n", app->app);
                ((app_switch_t *)app->app)->online((void *)app);
            break;
            case APP_SWITCHLAMP:
                DBG_MSG("online switchlamp: %p\r\n", app->app);
                ((app_switchlamp_t *)app->app)->online((void *)app);
            break;
            default:
            break;
        }
    }
}
static void ICACHE_FLASH_ATTR protocol_disconnected_cb(protocol_t *protocol) {
    uint8_t i;
    DBG_MSG("protocol: %p\r\n", protocol);

    if (protocol->app_count == 0) return;
    for (i=0; i<protocol->app_count; ++i) {
        app_t *app = (app_t *)protocol->apps[i];
        DBG_MSG("app: %p, type: %d\r\n", app, app->type);
        switch (app->type) {
            case APP_LAMP:
                DBG_MSG("offline lamp: %p\r\n", app->app);
                ((app_lamp_t *)app->app)->offline((void *)app);
            break;
            case APP_SWITCH:
                DBG_MSG("offline switch: %p\r\n", app->app);
                ((app_switch_t *)app->app)->offline((void *)app);
            break;
            case APP_SWITCHLAMP:
                DBG_MSG("offline switchlamp: %p\r\n", app->app);
                ((app_switchlamp_t *)app->app)->offline((void *)app);
            break;
            default:
            break;
        }
    }
}
static bool ICACHE_FLASH_ATTR protocol_filter_by_url(protocol_t *protocol, char *app_url, char *recv_url) {
    bool res = true;
    if (protocol->type == PROTOCOL_MQTT) {
        char *napp_url;
        char *nrecv_url;
        char *sharp_app;
        uint32_t app_len, recv_len, cmp_len;
        DBG_MSG("app_url[%d]: %s\r\n", os_strlen(app_url), app_url);
        DBG_MSG("recv_url[%d]: %s\r\n", os_strlen(recv_url), recv_url);
        // Check app url contains '#' character
        sharp_app = strchr(app_url, '#');
        DBG_MSG("sharp_app: %p\r\n", sharp_app);
        // If url does not contain '#', compare then return result
        if (sharp_app == NULL) {
            res = (strcmp(app_url, recv_url) == 0);
            DBG_MSG("filter result: %d\r\n", res);
            return res;
        }
        if (sharp_app != NULL) app_len = sharp_app - app_url;
        else app_len = os_strlen(app_url);
        recv_len = os_strlen(recv_url);

        if (recv_len < app_len) return false;

        cmp_len = (app_len < recv_len ? app_len:recv_len);
        DBG_MSG("app_len: %d, recv_len: %d, cmp_len: %d\r\n", app_len, recv_len, cmp_len);
        napp_url = (char *)os_zalloc(cmp_len + 1);
        nrecv_url = (char *)os_zalloc(cmp_len + 1);
        os_memcpy(napp_url, app_url, cmp_len);
        os_memcpy(nrecv_url, recv_url, cmp_len);
        napp_url[cmp_len] = 0;
        nrecv_url[cmp_len] = 0;
        DBG_MSG("napp_url: %s, nrecv_url: %s.\r\n", napp_url, nrecv_url);
        res = (strcmp(napp_url, nrecv_url) == 0);
        os_free(napp_url);
        os_free(nrecv_url);
    } else {
        res = false;
    }
    DBG_MSG("filter result: %d\r\n", res);

    return res;
}
static void ICACHE_FLASH_ATTR protocol_received_cb(protocol_t *protocol, char *url, char *data) {
    uint8_t i;
    DBG_MSG("protocol: %p, data: %s\r\n", protocol, data);

    if (protocol->app_count == 0) return;
    for (i=0; i<protocol->app_count; ++i) {
        app_t *app = (app_t *)protocol->apps[i];
        app_lamp_t *lamp = NULL;
        app_switch_t *sw = NULL;
        app_switchlamp_t *swlamp = NULL;
        DBG_MSG("app: %p, type: %d\r\n", app, app->type);
        switch (app->type) {
            case APP_LAMP:
                lamp = (app_lamp_t *)app->app;
                // Filter
                if (protocol_filter_by_url(protocol, lamp->url, url)) {
                    DBG_MSG("recv lamp: %p, url: %s\r\n", lamp, lamp->url);
                    lamp->recv((void *)app, data);
                }
            break;
            case APP_SWITCH:
                sw = (app_switch_t *)app->app;
                if (protocol_filter_by_url(protocol, sw->url, url)) {
                    DBG_MSG("recv switch: %p\r\n", sw);
                    sw->recv((void *)app, data);
                }
            break;
            case APP_SWITCHLAMP:
                swlamp = (app_switchlamp_t *)app->app;
                if (protocol_filter_by_url(protocol, swlamp->url, url)) {
                    DBG_MSG("recv switchlamp: %p\r\n", swlamp);
                    swlamp->recv((void *)app, data);
                }
            break;
            default:
            break;
        }
    }
}
static void ICACHE_FLASH_ATTR protocol_sent_cb(protocol_t *protocol) {
    DBG_MSG("protocol: %p\r\n", protocol);
}
static void ICACHE_FLASH_ATTR mqtt_connected_cb(uint32_t *args) {
    MQTT_Client* client = (MQTT_Client*)args;
    protocol_t *protocol = NULL;
    DBG_MSG("client: %p\r\n", client);
    // Find protocol by mqtt client
    protocol = protocol_find_by_mqtt_client(client);
    DBG_MSG("protocol: %p\r\n", protocol);
    if (protocol != NULL) {
        protocol->status = PROTO_STATUS_CONNNECTED;
        protocol_connected_cb(protocol);
    }
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
}
static void ICACHE_FLASH_ATTR mqtt_disconnected_cb(uint32_t *args) {
    MQTT_Client* client = (MQTT_Client*)args;
    protocol_t *protocol = NULL;
    DBG_MSG("client: %p\r\n", client);
    // Find protocol by mqtt client
    protocol = protocol_find_by_mqtt_client(client);
    DBG_MSG("protocol: %p\r\n", protocol);
    if (protocol != NULL) {
        protocol->status = PROTO_STATUS_INIT;
        protocol_disconnected_cb(protocol);
    }
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
}
static void ICACHE_FLASH_ATTR mqtt_sent_cb(uint32_t *args) {
    MQTT_Client* client = (MQTT_Client*)args;
    protocol_t *protocol = NULL;
    DBG_MSG("client: %p\r\n", client);
    // Find protocol by mqtt client
    protocol = protocol_find_by_mqtt_client(client);
    DBG_MSG("protocol: %p\r\n", protocol);
    if (protocol != NULL) {
        protocol->status = PROTO_STATUS_CONNNECTED;
        protocol_sent_cb(protocol);
    }
}
static void ICACHE_FLASH_ATTR mqtt_received_cb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t length) {
    MQTT_Client* client = (MQTT_Client*)args;
    protocol_t *protocol = NULL;
    char *data_buf = (char*)os_zalloc(length+1);
    char *url_buf = (char*)os_zalloc(topic_len+1);
    DBG_MSG("client: %p\r\n", client);
    DBG_MSG("data_buf: %p, url_buf: %p\r\n", data_buf, url_buf);
    if ((data_buf == NULL) || (url_buf == NULL)) goto err1;
    os_memcpy(data_buf, data, length);
    data_buf[length] = 0;
    os_memcpy(url_buf, topic, topic_len);
    url_buf[topic_len] = 0;
    DBG_MSG("received data: [%s] on topic %s\r\n", data_buf, url_buf);
    // Find protocol by mqtt client
    protocol = protocol_find_by_mqtt_client(client);
    DBG_MSG("protocol: %p\r\n", protocol);
    if (protocol != NULL) protocol_received_cb(protocol, url_buf, data_buf);
err1:
    if (data_buf != NULL) os_free(data_buf);
    if (url_buf != NULL) os_free(url_buf);
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
}
