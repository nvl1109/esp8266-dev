#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "debug.h"
#include "user_config.h"
#include "config.h"
#include "app_types.h"
#include "app_switch.h"
#include "protocol_manager.h"
#include "gpio_mod.h"

static app_status_t ICACHE_FLASH_ATTR switch_recv (void *sw, char * data);
static app_status_t ICACHE_FLASH_ATTR switch_online (void *sw);
static app_status_t ICACHE_FLASH_ATTR switch_offline (void *sw);

app_status_t ICACHE_FLASH_ATTR switch_create(char * id, cJSON * config, app_switch_t **sw) {
    cJSON * jtmp = NULL;
    app_switch_t *app = NULL;
    app_status_t result = APP_STATUS_ERR;
    char *ctmp = NULL;

    DBG_MSG("id: %s, config: %p\r\n", id, config);
    if ((id == NULL) || (config == NULL)) return APP_STATUS_INVALID_PARAM;

    app = os_malloc(sizeof(app_switch_t));
    DBG_MSG("app: %p\r\n", app);
    if (app == NULL) goto done;
    os_bzero(app, 0, sizeof(app_switch_t));

    app->id = id;

    jtmp = cJSON_GetObjectItem(config, ASWITCH_CFG_URL_STR);
    DBG_MSG("jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) goto err1;
    ctmp = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
    os_memset(ctmp, 0, os_strlen(jtmp->valuestring) + 1);
    os_memcpy(ctmp, jtmp->valuestring, os_strlen(jtmp->valuestring));
    app->url = ctmp;
    DBG_MSG("app->url: %s\r\n", app->url);

    jtmp = cJSON_GetObjectItem(config, ASWITCH_CFG_SENDURL_STR);
    DBG_MSG("jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) {
        app->surl = app->url;
    } else {
        ctmp = (char *)os_malloc(os_strlen(jtmp->valuestring) + 1);
        os_memset(ctmp, 0, os_strlen(jtmp->valuestring) + 1);
        os_memcpy(ctmp, jtmp->valuestring, os_strlen(jtmp->valuestring));
        app->surl = ctmp;
    }
    DBG_MSG("app->surl: %s\r\n", app->surl);

    jtmp = cJSON_GetObjectItem(config, ASWTICH_CFG_PIN_STR);
    DBG_MSG("jtmp: %p\r\n", jtmp);
    if (jtmp == NULL) goto err1;
    app->pin = jtmp->valueint;
    DBG_MSG("app->pin: %d\r\n", app->pin);

    app->type = APP_SWITCH;
    app->curval = false;
    app->recv = switch_recv;
    app->online = switch_online;
    app->offline = switch_offline;

    gpio_interrupt_start();

    // If everything ok, goto done
    result = APP_STATUS_OK;
    *sw = app;
    goto done;
err1:
    DBG_MSG("Error occured with status %d\r\n", result);
    if ((app != NULL) && (app->id != NULL)) os_free(app->id);
    if ((app != NULL) && (app->url != NULL)) os_free(app->url);
    if (app != NULL) os_free(app);
done:
    return result;
}

app_status_t ICACHE_FLASH_ATTR switch_remove(app_switch_t *sw) {
    if (sw == NULL) return APP_STATUS_INVALID_PARAM;

    if ((sw != NULL) && (sw->id != NULL)) os_free(sw->id);
    if ((sw != NULL) && (sw->url != NULL)) os_free(sw->url);
    if (sw != NULL) os_free(sw);

    return APP_STATUS_OK;
}

static app_status_t ICACHE_FLASH_ATTR switch_recv (void *app, char * data) {
    app_t *tapp = (app_t *)app;
    app_switch_t *sw = NULL;
    DBG_MSG("switch: %p, data: %s\r\n", app, data);
    if (app == NULL) return;
    if (data == NULL) return;
    DBG_MSG("app->app: %p\r\n", tapp->app);
    if (tapp->app == NULL) return;
    sw = (app_switch_t *)tapp->app;
    if ((strcmp(data, "true") == 0) || (strcmp(data, "1")==0)) {
        sw->curval = true;
    } else {
        sw->curval = false;
    }
    DBG_MSG("sw->curval: %d\r\n", sw->curval);

    return APP_STATUS_OK;
}
static void ICACHE_FLASH_ATTR switch_interrupt_cb(void *app) {
    app_t *tapp = (app_t *)app;
    app_switch_t *sw = NULL;
    protocol_t *protocol = NULL;
    bool res = false;
    char senddata[5];

    DBG_MSG("app %p callback\r\n", app);
    if (app == NULL) return;
    DBG_MSG("app->app: %p\r\n", tapp->app);
    if (tapp->app == NULL) return;
    sw = (app_switch_t *)tapp->app;
    DBG_MSG("sw->protocol: %p\r\n", sw->protocol);
    if (sw->protocol == NULL) return;
    protocol = (protocol_t *)sw->protocol;
    if (sw->curval) res = false;
    else res = true;
    os_bzero(senddata, sizeof(senddata));
    os_sprintf(senddata, "%d", res);
    DBG_MSG("res: %d, senddata: %s.\r\n", res, senddata);
    res = protocol_send(protocol, sw->surl, senddata);
    DBG_MSG("data sent, result %d\r\n", res);
}
static app_status_t ICACHE_FLASH_ATTR switch_online (void *app) {
    bool res = false;
    app_switch_t *sw = NULL;
    DBG_MSG("switch: %p\r\n", app);
    sw = (app_switch_t *)((app_t *)app)->app;
    res = protocol_register_receive((protocol_t *)sw->protocol, app);
    gpio_interrupt_install(sw->pin, GPIO_PIN_INTR_POSEDGE, (gpio_interrupt_callback)&switch_interrupt_cb, app);
    DBG_MSG("Interrupt installed for pin %d\r\n", sw->pin);
    return APP_STATUS_OK;
}
static app_status_t ICACHE_FLASH_ATTR switch_offline (void *app) {
    DBG_MSG("switch: %p\r\n", app);
    return APP_STATUS_OK;
}
