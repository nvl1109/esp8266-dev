#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#include "telnet_server.h"
#include "debug.h"
#include "config.h"

static struct espconn *serverConn;
static esp_tcp *serverTcp;

//Connection pool
serverConnData *connData;
static char *txbuffer;
static char *rxbuffer;
static uint16_t rxIndex = 0;

//send all data in conn->txbuffer
//returns result from espconn_sent if data in buffer or ESPCONN_OK (0)
//only internal used by espbuffsent, serverSentCb
static sint8  ICACHE_FLASH_ATTR sendtxbuffer(serverConnData *conn) {
    sint8 result = ESPCONN_OK;
    if (connData->txbufferlen != 0)  {
        connData->readytosend = false;
        result= espconn_sent(connData->conn, (uint8_t*)connData->txbuffer, connData->txbufferlen);
        connData->txbufferlen = 0;
        if (result != ESPCONN_OK)
            os_printf("sendtxbuffer: espconn_sent error %d on conn %p\n", result, conn);
    }
    return result;
}

//send formatted output to transmit buffer and call sendtxbuffer, if ready (previous espconn_sent is completed)
sint8 ICACHE_FLASH_ATTR  espbuffsentprintf(serverConnData *conn, const char *format, ...) {
    uint16 len;
    va_list al;
    va_start(al, format);
    len = ets_vsnprintf(connData->txbuffer + connData->txbufferlen, MAX_TXBUFFER - connData->txbufferlen - 1, format, al);
    va_end(al);
    if (len <0)  {
        os_printf("espbuffsentprintf: txbuffer full on conn %p\n", conn);
        return len;
    }
    connData->txbufferlen += len;
    if (connData->readytosend)
        return sendtxbuffer(connData);
    return ESPCONN_OK;
}


//send string through espbuffsent
sint8 ICACHE_FLASH_ATTR espbuffsentstring(serverConnData *conn, const char *data) {
    return espbuffsent(connData, data, strlen(data));
}

//use espbuffsent instead of espconn_sent
//It solve problem: the next espconn_sent must after espconn_sent_callback of the pre-packet.
//Add data to the send buffer and send if previous send was completed it call sendtxbuffer and  espconn_sent
//Returns ESPCONN_OK (0) for success, -128 if buffer is full or error from  espconn_sent
sint8 ICACHE_FLASH_ATTR espbuffsent(serverConnData *conn, const char *data, uint16 len) {
    if (connData->txbufferlen + len > MAX_TXBUFFER) {
        os_printf("\r\nespbuffsent: txbuffer full txbufferlen=%d, len=%d, max=%d", connData->txbufferlen, len, MAX_TXBUFFER);
        return -128;
    }
    os_memcpy(connData->txbuffer + connData->txbufferlen, data, len);
    connData->txbufferlen += len;
    if (connData->readytosend)
        return sendtxbuffer(connData);
    return ESPCONN_OK;
}

//callback after the data are sent
static void ICACHE_FLASH_ATTR serverSentCb(void *arg) {
    connData->readytosend = true;
    sendtxbuffer(connData); // send possible new data in txbuffer
}

static void ICACHE_FLASH_ATTR serverRecvCb(void *arg, char *data, unsigned short len) {
    int i;
    char *p, *e;

    // Store received data into rxbuffer util new line detected
    for (i=0; i<len; ++i) {
        if ((data[i] == '\r') || (data[i] == '\n')) {
            if (rxIndex >= 4 && rxbuffer[0] == '+' && rxbuffer[1] == '+' && rxbuffer[2] == '+' && rxbuffer[3] =='A' && rxbuffer[4] == 'T') {
                config_parse(connData, rxbuffer, rxIndex + 1);
                DBG_MSG("Parsed rxbuffer %d: %s\r\n", rxIndex, rxbuffer);
            } else {
                DBG_MSG("NOT A CONFIG COMMAND!!!\r\n");
            }
            memset(rxbuffer, 0, MAX_RXBUFFER);
            rxIndex = 0;
            break;
        }
        if (rxIndex >= MAX_RXBUFFER) {
            // RX buffer full
            os_printf("\r\nERR: Rx buffer full.");
            espbuffsentstring(connData, "Rx buffer full\r\n");
        }
        rxbuffer[rxIndex++] = data[i];
    }
}

static void ICACHE_FLASH_ATTR serverReconCb(void *arg, sint8 err) {
    serverConnData *conn = arg;
    if (conn==NULL) return;
    //Yeah... No idea what to do here. ToDo: figure something out.
}

static void ICACHE_FLASH_ATTR serverDisconCb(void *arg) {
    if (connData->conn!=NULL) {
        if (connData->conn->state==ESPCONN_NONE || connData->conn->state==ESPCONN_CLOSE) {
            connData->conn=NULL;
        }
        config_status = CONFIG_STATUS_WIFI_CONNECTED;
    }
}

static void ICACHE_FLASH_ATTR serverConnectCb(void *arg) {
    struct espconn *conn = arg;

    connData->conn=conn;
    connData->txbufferlen = 0;
    connData->readytosend = true;
    DBG_MSG("Client connected!!! txbufferlen=%d\r\n", connData->txbufferlen);

    espconn_regist_recvcb(conn, serverRecvCb);
    espconn_regist_reconcb(conn, serverReconCb);
    espconn_regist_disconcb(conn, serverDisconCb);
    espconn_regist_sentcb(conn, serverSentCb);
    config_status = CONFIG_STATUS_TELNET_CONNNECTED;
}

void ICACHE_FLASH_ATTR serverInit(int port) {
    // Alloc mem for structs
    serverConn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    serverTcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    connData = (serverConnData *)os_zalloc(sizeof(serverConnData));
    txbuffer = (char *)os_zalloc(MAX_TXBUFFER);
    rxbuffer = (char *)os_zalloc(MAX_RXBUFFER);
    DBG_MSG("mem alloc: serverConn: %p, serverTcp: %p, connData: %p, txbuffer: %p, rxbuffer: %p\r\n", serverConn, serverTcp, connData, txbuffer, rxbuffer);
    connData->conn = NULL;
    connData->txbuffer = txbuffer;
    connData->txbufferlen = 0;
    connData->readytosend = true;

    serverConn->type=ESPCONN_TCP;
    serverConn->state=ESPCONN_NONE;
    serverTcp->local_port=port;
    serverConn->proto.tcp=serverTcp;
}

void ICACHE_FLASH_ATTR serverDisconnect(void) {
    espconn_disconnect(serverConn);
    config_status = CONFIG_STATUS_WIFI_CONNECTED;
}

void ICACHE_FLASH_ATTR serverConnect(void) {
    sint8 res;
    res = espconn_regist_connectcb(serverConn, serverConnectCb);
    DBG_MSG("espconn_regist_connectcb, res: %d\r\n", res);
    res = espconn_accept(serverConn);
    DBG_MSG("espconn_accept, res: %d\r\n", res);
    res = espconn_regist_time(serverConn, SERVER_TIMEOUT, 0);
    DBG_MSG("espconn_regist_time, res: %d\r\n", res);
}

void ICACHE_FLASH_ATTR serverDeInit(void) {
    espconn_disconnect(serverConn);
    DBG_MSG("espconn_disconnect\r\n");
    espconn_delete(serverConn);
    DBG_MSG("espconn_delete\r\n");
    if (serverConn != NULL) os_free(serverConn);
    DBG_MSG("os_free(serverConn)\r\n");
    if (serverTcp != NULL) os_free(serverTcp);
    DBG_MSG("os_free(serverTcp)\r\n");
    if (connData != NULL) os_free(connData);
    DBG_MSG("os_free(connData)\r\n");
    if (txbuffer != NULL) os_free(txbuffer);
    DBG_MSG("os_free(txbuffer)\r\n");
    if (rxbuffer != NULL) os_free(rxbuffer);
    DBG_MSG("os_free(rxbuffer)\r\n");
}
