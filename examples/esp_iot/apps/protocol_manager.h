/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __PROTOCOL_MANAGER__H__
#define __PROTOCOL_MANAGER__H__

#include "cJSON.h"
#include "app_types.h"
#include "config.h"

#define MAX_NUMBER_OF_PROTOCOL  (5)

extern protocol_t protocol_list[MAX_NUMBER_OF_PROTOCOL];
extern uint8_t protocol_idx;

protocol_t * ICACHE_FLASH_ATTR protocol_create(protocol_type_t type, char * name, cJSON * config);
void ICACHE_FLASH_ATTR protocol_destroy(protocol_t * pproto);

/**
 * @brief Find first matches protocol
 */
protocol_t * ICACHE_FLASH_ATTR protocol_find(protocol_type_t type, char * search_str);
protocol_t * ICACHE_FLASH_ATTR protocol_find_by_mqtt_client(void * client);
bool ICACHE_FLASH_ATTR protocol_connect(protocol_t * pproto);
bool ICACHE_FLASH_ATTR protocol_disconnect(protocol_t * pproto);
bool ICACHE_FLASH_ATTR protocol_send(protocol_t *protocol, char *url, char * data);
bool ICACHE_FLASH_ATTR protocol_add_app(protocol_t * pproto, void * app);
void * ICACHE_FLASH_ATTR protocol_find_app(protocol_t * pproto, char * appid);
bool ICACHE_FLASH_ATTR protocol_register_receive(protocol_t *pproto, void *app);

#endif
