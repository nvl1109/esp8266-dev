/* config.h
*
* Copyright (c) 2015, LinhNV <nvl1109 at gmail dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef USR_NET_CONFIG_H_
#define USR_NET_CONFIG_H_
#include "os_type.h"
#include "smartconfig.h"
#include "mem.h"
#include "cJSON.h"

#define CONFIG_STATUS_NOTHING           (0)
#define CONFIG_STATUS_WIFI_CONNECTED    (1)
#define CONFIG_STATUS_TELNET_CONNNECTED (2)

extern char * config_string;
extern cJSON * jconfig_root;
extern uint8_t config_status;

extern void *pvPortMalloc(size_t xWantedSize);
extern void *pvPortZalloc(size_t);
extern void pvPortFree(void *ptr);
extern void vPortFree(void *ptr);
extern void *vPortMalloc(size_t xWantedSize);

/**
 * @brief Intialize net configuration
 *
 * After startSmartAfter(ms), if wifi still do not connected, Wifi smart config will start.
 * Then user can use smartphone to set/update wifi AP info (ssid, password) for connection.
 * After wifi connected, a telnet server will be started. It allows user set/update the config
 * of device.
 *
 * @param uint32_t startSmartAfter Number of milisecond that smart config will be started after.
 * @param uint32_t stopNetConfigAfter Number of milisecond that net config will be stopped after.
 *
 * @return None
 */
void ICACHE_FLASH_ATTR CFG_init_config(uint32_t startSmartAfter, uint32_t stopNetConfigAfter);

/**
 * @brief Start the telnet configuration
 *
 * Start the telnet server for configuration. This telnet server will be stop after
 * stopNetConfigAfter that configured by CFG_init_config().
 *
 */
void ICACHE_FLASH_ATTR CFG_start_config(void);

void ICACHE_FLASH_ATTR CFG_cancel_stop(void);

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata);

#endif /* USR_NET_CONFIG_H_ */
