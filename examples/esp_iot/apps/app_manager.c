#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "debug.h"
#include "user_config.h"
#include "config.h"
#include "app_manager.h"
#include "protocol_manager.h"
#include "cJSON.h"

app_t app_list[MAX_NUMBER_OF_APP];
uint8_t app_idx = 0;

app_status_t ICACHE_FLASH_ATTR app_create(app_t **app, void * protocol, char *id, cJSON * config) {
    app_t * papp = NULL;
    app_status_t res = APP_STATUS_ERR;
    cJSON * jtmp = NULL;

    DBG_MSG("id: %s, protocol: %p, config: %p\r\n", id, protocol, config);
    if (app_idx == 0) {
        DBG_MSG("Zero the app list %p:%d\r\n", app_list, sizeof(app_list));
        os_memset(app_list, 0, sizeof(app_list));
    }
    jtmp = cJSON_GetObjectItem(config, APP_CFG_TYPE_STR);
    if (jtmp == NULL) goto err1;
    DBG_MSG("Type %s\r\n", jtmp->valuestring);

    if (app_idx >= (MAX_NUMBER_OF_APP - 1)) return APP_STATUS_LIST_FULL;
    if (config == NULL) return APP_STATUS_INVALID_PARAM;
    if (id == NULL) return APP_STATUS_INVALID_PARAM;
    if (protocol == NULL) return APP_STATUS_INVALID_PARAM;

    papp = &app_list[app_idx];
    DBG_MSG("app_idx: %d, papp: %p\r\n", app_idx, papp);

    if (strcmp("lamp", jtmp->valuestring) == 0) {
        DBG_MSG("Create lamp app\r\n");
        app_lamp_t * lamp = NULL;
        os_bzero(papp, sizeof(app_t));
        res = lamp_create(id, config, &lamp);
        DBG_MSG("lamp_create with res: %d, lamp: %p\r\n", res, lamp);
        if (res != APP_STATUS_OK) goto err1;
        papp->app = (void *)lamp;
        papp->type = APP_LAMP;
    } else if (strcmp("switch", jtmp->valuestring) == 0) {
        DBG_MSG("Create switch app\r\n");
        app_switch_t * sw = NULL;
        os_bzero(papp, sizeof(app_t));
        res = switch_create(id, config, &sw);
        DBG_MSG("switch_create with res: %d, switch: %p\r\n", res, sw);
        if (res != APP_STATUS_OK) goto err1;
        papp->app = (void *)sw;
        papp->type = APP_SWITCH;
    } else if (strcmp("switchlamp", jtmp->valuestring) == 0) {
        DBG_MSG("Create switchlamp app\r\n");
        app_switchlamp_t * sw = NULL;
        os_bzero(papp, sizeof(app_t));
        res = switchlamp_create(id, config, &sw);
        DBG_MSG("switchlamp_create with res: %d, switchlamp: %p\r\n", res, sw);
        if (res != APP_STATUS_OK) goto err1;
        papp->app = (void *)sw;
        papp->type = APP_SWITCHLAMP;
    } else {
        return APP_STATUS_INVALID_PARAM;
    }
    *app = papp;

    // Call app created callback
    switch(papp->type) {
        case APP_LAMP:
        break;
        case APP_SWITCH:
        break;
        case APP_SWITCHLAMP:
            switchlamp_aftercreate(papp);
        break;
        default:
        break;
    }

    goto done;
err1:
    return res;
done:
    app_idx ++;
    return APP_STATUS_OK;
}

app_status_t ICACHE_FLASH_ATTR app_send(app_t * app, char * data) {
    char * url = NULL;

    DBG_MSG("app: %p, data:%s\r\n", app, data);
    if ((app == NULL) || (data == NULL)) return APP_STATUS_INVALID_PARAM;

    // Check protocol
    DBG_MSG("app->app: %p\r\n", app->app);
    if (app->app == NULL) return APP_STATUS_ERR;
    DBG_MSG("app->app->protocol: %p\r\n", ((app_generic_t *)app->app)->protocol);
    if (((app_generic_t *)app->app)->protocol == NULL) return APP_STATUS_PROTOCOL_NULL;
    DBG_MSG("app->app->protocol->status: %p\r\n", ((protocol_t *)((app_generic_t *)app->app)->protocol)->status);
    if (((protocol_t *)((app_generic_t *)app->app)->protocol)->status != PROTO_STATUS_CONNNECTED) return APP_STATUS_NOT_CONNECTED;

    // Send data
    switch(app->type) {
        case APP_LAMP:
        url = ((app_lamp_t *)app->app)->url;
        break;
        case APP_SWITCH:
        url = ((app_switch_t *)app->app)->url;
        break;
        case APP_SWITCHLAMP:
        url = ((app_switchlamp_t *)app->app)->surl;
        break;
    }
    DBG_MSG("url: %s\r\n", url);
    if (!protocol_send((protocol_t *)((app_generic_t *)app->app)->protocol, url, data)) return APP_STATUS_ERR;

    return APP_STATUS_OK;
}

app_status_t ICACHE_FLASH_ATTR app_match(app_t *app, char *id) {
    DBG_MSG("app: %p, id: %s\r\n", app, id);

    if ((app == NULL) || (id == NULL) || (os_strlen(id) == 0)) return APP_STATUS_INVALID_PARAM;
    DBG_MSG("app->app: %p\r\n", app->app);
    if (app->app == NULL) return APP_STATUS_ERR;
    if (app->type == APP_LAMP) {
        if (strcmp(((app_lamp_t *)app->app)->id, id) == 0) return APP_STATUS_OK;
    } else if (app->type == APP_SWITCH) {
        if (strcmp(((app_switch_t *)app->app)->id, id) == 0) return APP_STATUS_OK;
    } else if (app->type == APP_SWITCHLAMP) {
        if (strcmp(((app_switchlamp_t *)app->app)->id, id) == 0) return APP_STATUS_OK;
    }

    return APP_STATUS_ERR;
}

app_status_t ICACHE_FLASH_ATTR app_get_id(app_t *app, char **id) {
    DBG_MSG("app: %p\r\n", app);

    if (app == NULL) return APP_STATUS_INVALID_PARAM;
    DBG_MSG("app->app: %p\r\n", app->app);
    if (app->app == NULL) return APP_STATUS_ERR;
    if (app->type == APP_LAMP) {
        *id = ((app_lamp_t *)app->app)->id;
        return APP_STATUS_OK;
    } else if (app->type == APP_SWITCH) {
        *id = ((app_switch_t *)app->app)->id;
        return APP_STATUS_OK;
    } else if (app->type == APP_SWITCHLAMP) {
        *id = ((app_switchlamp_t *)app->app)->id;
        return APP_STATUS_OK;
    }

    return APP_STATUS_ERR;
}
