/*
 * config.c
 *
 *  Created on: Jul 25, 2015
 *      Author: LinhNV (nvl1109@gmail.com)
 */
#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "debug.h"
#include "user_config.h"
#include "flash_config.h"
#include "config.h"
#include "telnet_server.h"

LOCAL os_timer_t connectTimer;
LOCAL uint32_t stopTelnetAfter;
cJSON * jconfig_root = NULL;
char * config_string = NULL;
uint8_t config_status = CONFIG_STATUS_NOTHING;

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    cJSON * jssid = NULL;
    cJSON * jpass = NULL;
    cJSON * jtmp = NULL;
    uint8 phone_ip[4] = {0};
    struct station_config *sta_conf = pdata;

    DBG_MSG("\r\n");
    switch(status) {

        case SC_STATUS_WAIT:
            DBG_MSG("SC_STATUS_WAIT\r\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            DBG_MSG("SC_STATUS_FIND_CHANNEL\r\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            DBG_MSG("SC_STATUS_GETTING_SSID_PSWD\r\n");
            break;
        case SC_STATUS_LINK:
            DBG_MSG("SC_STATUS_LINK\r\n");
            // Store config
            if (jconfig_root == NULL) {
                jconfig_root = cJSON_Parse(config_string);
            }
            if (jconfig_root) {
                DBG_MSG("Now json content:\r\n%s\r\n", cJSON_Print(jconfig_root));
                jssid = cJSON_GetObjectItem(jconfig_root,"wifi_ssid");
                if (jssid) {
                    jtmp = cJSON_DetachItemFromObject(jconfig_root, "wifi_ssid");
                    DBG_MSG("detached %p\r\n", jtmp);
                    cJSON_Delete(jssid);
                    DBG_MSG("Delete ssid object %p\r\n", jssid);
                }
                jpass = cJSON_GetObjectItem(jconfig_root,"wifi_pass");
                if (jpass) {
                    jtmp = cJSON_DetachItemFromObject(jconfig_root, "wifi_pass");
                    DBG_MSG("detached %p\r\n", jtmp);
                    cJSON_Delete(jpass);
                    DBG_MSG("Delete pass object %p\r\n", jpass);
                }
                DBG_MSG("Now json content:\r\n%s\r\n", cJSON_Print(jconfig_root));
                cJSON_AddItemToObject(jconfig_root, "wifi_ssid", cJSON_CreateString(sta_conf->ssid));
                cJSON_AddItemToObject(jconfig_root, "wifi_pass", cJSON_CreateString(sta_conf->password));
                DBG_MSG("Updated config with content: \r\n%s\r\n", cJSON_Print(jconfig_root));
                CFG_save(cJSON_PrintUnformatted(jconfig_root));
            }
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            DBG_MSG("SC_STATUS_LINK_OVER\r\n");
            os_memcpy(phone_ip, (uint8*)pdata, 4);
            INFO("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);

            smartconfig_stop();
            wifi_station_set_auto_connect(1);
            break;
    }
}
void ICACHE_FLASH_ATTR stop_telnet_server(void *arg) {
    os_timer_disarm(&connectTimer);

    if (config_status != CONFIG_STATUS_TELNET_CONNNECTED) {
        DBG_MSG("Stop telnet server NOW!!!\r\n");
        serverDeInit();
    }
}
void ICACHE_FLASH_ATTR restart_board_timer_cb(void *arg) {
    DBG_MSG("RESTART NOWWWWW!!!!\r\n");
    system_restart();
}
void ICACHE_FLASH_ATTR timer_check_connection(void *arg) {
    uint8_t status = wifi_station_get_connect_status();
    DBG_MSG("status = %d\r\n", status);
    os_timer_disarm(&connectTimer);
    // Check wifi status
    if (status != STATION_GOT_IP) {
        DBG_MSG("Start smart config\r\n");
        // Start the smart config
        wifi_set_opmode(STATION_MODE);
        wifi_station_disconnect();
        os_timer_setfn(&connectTimer, (os_timer_func_t *)restart_board_timer_cb, NULL);
        os_timer_arm(&connectTimer, WIFI_BOARD_RST_AFTER, 0);
        smartconfig_start(smartconfig_done, 0);
    }
}

void ICACHE_FLASH_ATTR CFG_parse_config(char *str) {
    char * val = NULL;
    char * tmp = NULL;
    cJSON * lamp = NULL;
    if (jconfig_root == NULL) {
        jconfig_root = cJSON_Parse(str);
    }
    DBG_MSG("json root pointer %p\r\n", jconfig_root);
}

void ICACHE_FLASH_ATTR CFG_init_config(uint32_t startSmartAfter, uint32_t stopNetConfigAfter) {
    DBG_MSG("init with %d:%d\r\n", startSmartAfter, stopNetConfigAfter);
    DBG_MSG("config_string was read with %p\r\n", config_string);
    if (config_string != NULL) {
        DBG_MSG("config length = [%d]\r\n", os_strlen(config_string));
        DBG_MSG("config content: \r\n%s\r\n\r\n", config_string);
        CFG_parse_config(config_string);
    }
    stopTelnetAfter = stopNetConfigAfter;
    serverInit(DEVICE_TELNET_PORT);
    os_timer_disarm(&connectTimer);
    os_timer_setfn(&connectTimer, (os_timer_func_t *)timer_check_connection, NULL);
    os_timer_arm(&connectTimer, startSmartAfter, 0);
}

void ICACHE_FLASH_ATTR CFG_start_config(void) {
    serverConnect();
    os_timer_disarm(&connectTimer);
    os_timer_setfn(&connectTimer, (os_timer_func_t *)stop_telnet_server, NULL);
    os_timer_arm(&connectTimer, stopTelnetAfter, 0);
}

void ICACHE_FLASH_ATTR CFG_cancel_stop(void) {
    os_timer_disarm(&connectTimer);
}
