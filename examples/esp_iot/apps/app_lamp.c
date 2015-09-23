#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "debug.h"
#include "user_config.h"
#include "config.h"
#include "app_types.h"
#include "app_lamp.h"
#include "gpio_mod.h"
#include "gpio.h"
#include "protocol_manager.h"

static app_status_t ICACHE_FLASH_ATTR lamp_recv (void *lamp, char * data);
static app_status_t ICACHE_FLASH_ATTR lamp_online (void *lamp);
static app_status_t ICACHE_FLASH_ATTR lamp_offline (void *lamp);

app_status_t ICACHE_FLASH_ATTR lamp_create(char * id, cJSON * config, app_lamp_t **lamp) {
    cJSON * jtmp = NULL;
    app_lamp_t *app = NULL;
    app_status_t result = APP_STATUS_ERR;
    char *ctmp = NULL;

    DBG_MSG("id: %s, config: %p\r\n", id, config);
    if ((id == NULL) || (config == NULL)) return APP_STATUS_INVALID_PARAM;

    app = os_malloc(sizeof(app_lamp_t));
    DBG_MSG("app: %p\r\n", app);
    if (app == NULL) goto done;
    os_bzero(app, 0, sizeof(app_lamp_t));

    app->id = id;

    jtmp = cJSON_GetObjectItem(config, ALAMP_CFG_URL_STR);
    DBG_MSG("jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) goto err1;
    ctmp = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
    os_memset(ctmp, 0, os_strlen(jtmp->valuestring) + 1);
    os_memcpy(ctmp, jtmp->valuestring, os_strlen(jtmp->valuestring));
    app->url = ctmp;
    DBG_MSG("app->url: %s\r\n", app->url);

    jtmp = cJSON_GetObjectItem(config, ALAMP_CFG_PIN_STR);
    DBG_MSG("jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) goto err1;
    app->pin = jtmp->valueint;
    DBG_MSG("app->pin: %d\r\n", app->pin);

    app->type = APP_LAMP;
    app->recv = lamp_recv;
    app->online = lamp_online;
    app->offline = lamp_offline;

    // Init GPIO pin
    gpio_function(app->pin);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(app->pin), 0);

    // If everything ok, goto done
    result = APP_STATUS_OK;
    *lamp = app;
    goto done;
err1:
    DBG_MSG("Error occured with status %d\r\n", result);
    if ((app != NULL) && (app->id != NULL)) os_free(app->id);
    if ((app != NULL) && (app->url != NULL)) os_free(app->url);
    if (app != NULL) os_free(app);
done:
    return result;
}

app_status_t ICACHE_FLASH_ATTR lamp_remove(app_lamp_t *lamp) {
    DBG_MSG("lamp: %p\r\n", lamp);
    if (lamp == NULL) return APP_STATUS_INVALID_PARAM;

    if ((lamp != NULL) && (lamp->id != NULL)) os_free(lamp->id);
    if ((lamp != NULL) && (lamp->url != NULL)) os_free(lamp->url);
    if (lamp != NULL) os_free(lamp);

    return APP_STATUS_OK;
}

static app_status_t ICACHE_FLASH_ATTR lamp_recv (void *app, char * data) {
    app_lamp_t *lamp = (app_lamp_t *)((app_t *)app)->app;
    DBG_MSG("lamp %s: %p, data: %s\r\n", lamp->id, app, data);
    if ((strcmp(data, "true") == 0) || (strcmp(data, "1")==0)) {
        GPIO_OUTPUT_SET(GPIO_ID_PIN(lamp->pin), 1);
    } else {
        GPIO_OUTPUT_SET(GPIO_ID_PIN(lamp->pin), 0);
    }
    return APP_STATUS_OK;
}
static app_status_t ICACHE_FLASH_ATTR lamp_online (void *app) {
    bool res = false;
    app_lamp_t *lamp = (app_lamp_t *)((app_t *)app)->app;
    DBG_MSG("lamp %s: %p\r\n", lamp->id, app);
    res = protocol_register_receive((protocol_t *)lamp->protocol, app);
    DBG_MSG("receive registered, res: %d\r\n", res);
    DBG_MSG("lamp->url: [%s]\r\n", lamp->url);
    DBG_MSG("lamp->pin: [%d]\r\n", lamp->pin);
    return APP_STATUS_OK;
}
static app_status_t ICACHE_FLASH_ATTR lamp_offline (void *app) {
    app_lamp_t *lamp = (app_lamp_t *)((app_t *)app)->app;
    DBG_MSG("lamp %s: %p\r\n", lamp->id, app);
    return APP_STATUS_OK;
}
