/**
 * Author LinhNV (nvl1109@gmail.com)
 */
#ifndef __PROTOCOL_REST__H__
#define __PROTOCOL_REST__H__

#include "app_types.h"
#include "config.h"
#include "espconn.h"

typedef struct SProtocolRest {
    protocol_type_t type;       /**!< Type of protocol - RESTful */
    char * server;              /**!< Server url */
    uint32_t port;              /**!< Server port number */
    char * user;                /**!< User name */
    char * pass;                /**!< Password */
    char * url;                 /**!< REST url */
    struct espconn conn;        /**!< TCP Connection */
} protocol_rest_t;

protocol_rest_t * ICACHE_FLASH_ATTR rest_create_client(cJSON * config);

#endif /* __PROTOCOL_REST__H__ */
