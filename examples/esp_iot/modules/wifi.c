/*
 * wifi.c
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */
#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "debug.h"
#include "user_config.h"
#include "flash_config.h"
#include "smartconfig.h"
#include "config.h"

void ICACHE_FLASH_ATTR WIFI_Connect(wifi_event_handler_cb_t cb)
{
    struct station_config stationConf;
    INFO("WIFI_INIT\r\n");
    wifi_set_opmode(STATION_MODE);

    wifi_set_event_handler_cb(cb);

    // Config led to inidicate wifi status
    wifi_status_led_install(4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);

    // Check Flash saved configuration
    wifi_station_get_config_default(&stationConf);
    DBG_MSG("Default config [%s]:[%s]\r\n", stationConf.ssid, stationConf.password);
    if (strlen(stationConf.ssid) == 0) {
        os_memset(&stationConf, 0, sizeof(struct station_config));

        if (jconfig_root == NULL) {
            jconfig_root = cJSON_Parse(config_string);
        }
        DBG_MSG("jconfig_root: %p\r\n", jconfig_root);
        if (jconfig_root != NULL) {
            cJSON * jssid = NULL;
            cJSON * jpass = NULL;

            jssid = cJSON_GetObjectItem(jconfig_root,"wifi_ssid");
            jpass = cJSON_GetObjectItem(jconfig_root,"wifi_pass");
            if ((jssid != NULL) && (jpass != NULL)) {
                os_sprintf(stationConf.ssid, "%s", jssid->valuestring);
                os_sprintf(stationConf.password, "%s", jpass->valuestring);
                DBG_MSG("connect wifi from config [%s]:[%s]\r\n", jssid->valuestring, jpass->valuestring);
                wifi_station_set_config(&stationConf);
            } else {
                DBG_MSG("wifi info not found from config [%p]:[%p]\r\n", jssid, jpass);
            }
        }
    }
#if 0
    os_sprintf(stationConf.ssid, "abdef");
    os_sprintf(stationConf.password, "abcdef");
    wifi_station_set_config(&stationConf);
#endif

    //Set sta settings
    wifi_station_set_config_current(&stationConf);
    wifi_station_set_auto_connect(TRUE);
    wifi_station_set_reconnect_policy(1);
    wifi_station_connect();
}

