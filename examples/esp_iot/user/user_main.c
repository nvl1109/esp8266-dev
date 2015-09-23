/* main.c -- MQTT client example
*
* Copyright (c) 2015, Linh Nguyen <nvl1109@gmail.com>
* All rights reserved.
*
*/
#include "ets_sys.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "flash_config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "config.h"
#include "protocol_manager.h"
#include "app_manager.h"
#include "telnet_server.h"

void ICACHE_FLASH_ATTR wifiConnectCb(System_Event_t *evt)
{
    uint8_t i;
    bool res = false;
    DBG_MSG("Wifi event: %d\r\n", evt->event);
    if(evt->event == EVENT_STAMODE_GOT_IP){
        // Start telnet server for user config
        config_status = CONFIG_STATUS_WIFI_CONNECTED;
        CFG_start_config();
        // Connect network
        for (i=0; i<protocol_idx; ++i) {
            res = protocol_connect(&protocol_list[i]);
            DBG_MSG("protocol_connect res: %d, index: %d\r\n", res, i);
        }
    } else {
        // Disconnect network
        if (config_status != CONFIG_STATUS_NOTHING) {
            for (i=0; i<protocol_idx; ++i) {
                res = protocol_disconnect(&protocol_list[i]);
                DBG_MSG("protocol_disconnect res: %d, index: %d\r\n", res, i);
            }
            serverDisconnect();
            config_status = CONFIG_STATUS_NOTHING;
        }
    }
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
}
void ICACHE_FLASH_ATTR create_protocol_app(cJSON * jconfig) {
    cJSON *jitem = NULL;
    protocol_t * pproto = NULL;
    app_status_t app_status = APP_STATUS_OK;
    app_t * papp = NULL;
    char *ptmp = NULL;
    bool res = false;

    DBG_MSG("jconfig: %p\r\n", jconfig);
    if (jconfig == NULL) return;

    // Check item is device
    jitem = cJSON_GetObjectItem(jconfig, "protocol");
    DBG_MSG("jitem: %p\r\n", jitem);
    if (jitem == NULL) return;
    ptmp = cJSON_PrintUnformatted(jconfig);
    DBG_MSG("device item:%s\r\n", ptmp);
    DBG_MSG("jitem value: %s\r\n", jitem->valuestring);
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
    if (strcmp(jitem->valuestring, "mqtt") == 0) {
        DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
        pproto = protocol_find(PROTOCOL_MQTT, ptmp);
        if (NULL == pproto)
            pproto = protocol_create(PROTOCOL_MQTT, jconfig->string, jconfig);
    }
    DBG_MSG("pproto: %p\r\n", pproto);
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
    app_status = app_create(&papp, (void *)pproto, jconfig->string, jconfig);
    DBG_MSG("create app, result: %d, papp: %p, app_idx: %d\r\n", app_status, papp, app_idx);
    if (app_status == APP_STATUS_OK) {
        res = protocol_add_app(pproto, papp);
        DBG_MSG("add app into protocol, res: %d\r\n", res);
    }
    os_free(ptmp);
}
void ICACHE_FLASH_ATTR parse_config_for_app(void) {
    uint8_t i, pre=0;
    cJSON * jtmp = NULL;
    char atmp[50];

    DBG_MSG("jconfig_root: %p\r\n", jconfig_root);
    if (jconfig_root == NULL) return;

    // Check "apps" element
    jtmp = cJSON_GetObjectItem(jconfig_root, "apps");
    DBG_MSG("apps jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) return;
    for (i=0; i<os_strlen(jtmp->valuestring); ++i) {
        if (jtmp->valuestring[i] != ' ') continue;
        if ((i - pre) <= 1) {
            pre ++;
            continue;
        }
        os_bzero(atmp, sizeof(atmp));
        os_memcpy(atmp, jtmp->valuestring + pre, i - pre);
        DBG_MSG("app tmp: %s.\r\n", atmp);
        create_protocol_app(cJSON_GetObjectItem(jconfig_root, atmp));
        pre = i + 1;
    }
    if (pre < (os_strlen(jtmp->valuestring) - 1)) {
        // Have last element
        os_bzero(atmp, sizeof(atmp));
        os_memcpy(atmp, jtmp->valuestring + pre, os_strlen(jtmp->valuestring) - pre);
        DBG_MSG("last app: %s.\r\n", atmp);
        create_protocol_app(cJSON_GetObjectItem(jconfig_root, atmp));
    }
}

void user_rf_pre_init(void)
{
}

void user_init(void)
{
    uart_div_modify(0, UART_CLK_FREQ/74880);
    os_delay_us(100000);
    DBG_MSG("SDK version:%s\r\n", system_get_sdk_version());
    // Initialize the GPIO subsystem.
    gpio_init();

#if CONFIG_IN_FLASH
    CFG_save(DEVICE_DEFAULT_CONFIG);
    config_string = CFG_read();
    DBG_MSG("Use config from flash, config_string: %p\r\n", config_string);
#else
    config_string = (char *)os_zalloc(CFG_CONFIG_SIZE);
    DBG_MSG("config_string: %p\r\n", config_string);
    os_memcpy(config_string, DEVICE_DEFAULT_CONFIG, os_strlen(DEVICE_DEFAULT_CONFIG));
    DBG_MSG("Config len: %d\r\n", os_strlen(config_string));
#endif
    WIFI_Connect(wifiConnectCb);
    CFG_init_config(WIFI_DEFAULT_SMARTAFTER, WIFI_DEFAULT_STOPNETAFTER);

    INFO("\r\nSystem started ...\r\n");
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
    parse_config_for_app();
    DBG_MSG("Free heap: %d\r\n", system_get_free_heap_size());
}
