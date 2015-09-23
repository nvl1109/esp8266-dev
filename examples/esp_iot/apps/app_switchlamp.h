/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __APP_SWITCHLAMP__H__
#define __APP_SWITCHLAMP__H__

#include "app_types.h"
#include "config.h"

#define ASWITCHLAMP_CFG_RECVURL_STR     "url"
#define ASWITCHLAMP_CFG_SENDURL_STR     "surl"
#define ASWTICHLAMP_CFG_SWPIN_STR       "swpin"
#define ASWTICHLAMP_CFG_LPIN_STR        "lpin"

typedef struct SAppSwitchLamp {
    app_type_t type;                        /*!< Type of app */
    void * protocol;                        /*!< Pointer to protocol */
    protocol_type_t protocol_type;          /*!< App protocol */
    char * url;                             /*!< App url */
    char * surl;                            /*!< App send url */
    char * id;                              /*!< App ID */
    uint8_t swpin;                          /*!< Switch GPIO pin number */
    uint8_t lpin;                           /*!< Lamp GPIO pin number */
    bool curval;                            /*!< Current value of switch */
    app_status_t (*recv) (void *app, char * data);
    app_status_t (*online) (void *app);
    app_status_t (*offline) (void *app);
} app_switchlamp_t;

app_status_t ICACHE_FLASH_ATTR switchlamp_create(char * id, cJSON * config, app_switchlamp_t **);
app_status_t ICACHE_FLASH_ATTR switchlamp_aftercreate(void *app);
app_status_t ICACHE_FLASH_ATTR switchlamp_remove(app_switchlamp_t *sw);

#endif /* __APP_SWITCHLAMP__H__ */
