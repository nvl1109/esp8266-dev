/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "smartconfig.h"
#include "driver/uart.h"

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
  switch(status) {
    case SC_STATUS_WAIT:
    os_printf("SC_STATUS_WAIT\n");
    break;

    case SC_STATUS_FIND_CHANNEL:
    os_printf("SC_STATUS_FIND_CHANNEL\n");
    break;

    case SC_STATUS_GETTING_SSID_PSWD:
    os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
    break;

    case SC_STATUS_LINK:
    os_printf("SC_STATUS_LINK\n");
    struct station_config *sta_conf = pdata;
    wifi_station_set_config(sta_conf);
    wifi_station_disconnect();
    wifi_station_connect();
    break;
    
    case SC_STATUS_LINK_OVER:
    os_printf("SC_STATUS_LINK_OVER\n");
    smartconfig_stop();
    break;
  }
}

void user_init(void)
{
  uart_init(BIT_RATE_115200, BIT_RATE_115200);

  os_printf("SDK version:%s\n", system_get_sdk_version());

  wifi_set_opmode(STATION_MODE);
  smartconfig_start(smartconfig_done);
}
