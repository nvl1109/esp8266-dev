/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __APP_MANAGER__H__
#define __APP_MANAGER__H__

#include "app_types.h"
#include "config.h"
#include "app_lamp.h"
#include "app_switch.h"
#include "app_switchlamp.h"

#define MAX_NUMBER_OF_APP           (10)

#define APP_CFG_TYPE_STR            "app"

extern app_t app_list[MAX_NUMBER_OF_APP];
extern uint8_t app_idx;

app_status_t ICACHE_FLASH_ATTR app_create(app_t **app, void * protocol, char *id, cJSON * config);
app_status_t ICACHE_FLASH_ATTR app_send(app_t * app, char * data);
app_status_t ICACHE_FLASH_ATTR app_match(app_t *app, char *id);
app_status_t ICACHE_FLASH_ATTR app_get_id(app_t *app, char **id);

#endif /* __APP_MANAGER__H__ */
