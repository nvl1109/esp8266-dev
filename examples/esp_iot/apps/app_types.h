/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __APP_TYPES__H__
#define __APP_TYPES__H__

#define MAX_APP_PER_PROTOCOL    (5)

typedef enum EProtocolType {
    PROTOCOL_INVALID = 0,
    PROTOCOL_MQTT,      /*!< MQTT protocol */
    PROTOCOL_RESTFUL,   /*!< RESTful protocol */
    PROTOCOL_HTTP,      /*!< HTTP POST/GET protocol */
    PROTOCOL_LAST
} protocol_type_t;

typedef enum EProtocolStatus {
    PROTO_STATUS_UNINIT = 0,
    PROTO_STATUS_INIT,
    PROTO_STATUS_CONNNECTED,
    PROTO_STATUS_SENDING,
    PROTO_STATUS_LAST
} protocol_status_t;

typedef enum EAppType {
    APP_INVALID = 0,
    APP_LAMP,       /*!< lamp application */
    APP_SWITCH,     /*!< Switch application */
    APP_DHT,        /*!< DHT11 sensor application */
    APP_SWITCHLAMP, /*!< Swith + Lamp application */
    APP_LAST
} app_type_t;

typedef enum EAppStatus {
    APP_STATUS_OK = 0,
    APP_STATUS_INVALID_PARAM,
    APP_STATUS_PROTOCOL_NULL,
    APP_STATUS_NOT_CONNECTED,
    APP_STATUS_LIST_FULL,
    APP_STATUS_ERR
} app_status_t;

typedef struct SProtocolGeneric {
    protocol_type_t type;
} protocol_generic_t;

typedef struct SAppGeneric {
    app_type_t type;
    void * protocol;
} app_generic_t;

typedef struct SAppStruct {
    app_type_t type;
    void * app;
} app_t;

typedef struct ProtocolStruct
{
    protocol_type_t type;                       /*!< Type of protocol */
    void * pprotocol;                           /*!< Pointer to real protocol struct */
    protocol_status_t status;                   /*!< Current protocol status */
    void * apps[MAX_APP_PER_PROTOCOL];          /*!< Array of apps that use this protocol */
    uint8_t app_count;                          /*!< Current apps count use this protocol */
} protocol_t;

typedef void (*protocol_callback)(protocol_type_t *protocol);

#endif
