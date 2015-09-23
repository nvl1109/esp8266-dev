/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __APP_SWITCH__H__
#define __APP_SWITCH__H__

#include "app_types.h"
#include "config.h"

#define ASWITCH_CFG_URL_STR     "url"
#define ASWITCH_CFG_SENDURL_STR "surl"
#define ASWTICH_CFG_PIN_STR     "pin"

typedef struct SAppSwitch {
    app_type_t type;                        /*!< Type of app */
    void * protocol;                        /*!< Pointer to protocol */
    protocol_type_t protocol_type;          /*!< App protocol */
    char * url;                             /*!< App url */
    char * surl;                            /*!< App send url */
    char * id;                              /*!< App ID */
    uint8_t pin;                            /*!< Switch GPIO pin number */
    bool curval;                            /*!< Current value of switch */
    app_status_t (*recv) (void *app, char * data);
    app_status_t (*online) (void *app);
    app_status_t (*offline) (void *app);
} app_switch_t;

app_status_t ICACHE_FLASH_ATTR switch_create(char * id, cJSON * config, app_switch_t **);
app_status_t ICACHE_FLASH_ATTR switch_remove(app_switch_t *sw);

#endif /* __APP_SWITCH__H__ */
