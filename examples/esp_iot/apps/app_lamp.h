/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __APP_LAMP__H__
#define __APP_LAMP__H__

#include "app_types.h"
#include "config.h"

#define ALAMP_CFG_URL_STR   "url"
#define ALAMP_CFG_PIN_STR   "pin"

typedef struct SAppLamp {
    app_type_t type;                        /**!< Type of app */
    void * protocol;                        /**!< Pointer to protocol */
    protocol_type_t protocol_type;          /**!< App protocol */
    char * url;                             /**!< App url */
    char * id;                              /**!< App ID */
    uint8_t pin;                            /**!< Lamp GPIO pin number */
    app_status_t (*recv) (void *app, char * data);
    app_status_t (*online) (void *app);
    app_status_t (*offline) (void *app);
} app_lamp_t;

app_status_t ICACHE_FLASH_ATTR lamp_create(char * id, cJSON * config, app_lamp_t **);
app_status_t ICACHE_FLASH_ATTR lamp_remove(app_lamp_t *lamp);

#endif /* __APP_LAMP__H__ */
