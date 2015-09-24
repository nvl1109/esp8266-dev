#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#if DEBUG_ON
#ifndef INFO
#define INFO os_printf
#endif
#ifndef DBG_MSG
#define DBG_MSG(fmt, args...) os_printf("%s:%d:%s(): " fmt,__FILE__, __LINE__, __FUNCTION__, ##args)
#endif
#else
#define INFO
#define DBG_MSG
#endif

#define WIFI_DEFAULT_SMARTAFTER     (30000)
#define WIFI_DEFAULT_STOPNETAFTER   (50000)
#define WIFI_BOARD_RST_AFTER        (30000)

#define DEVICE_DEFAULT_CONFIG   "{"                                           \
                                "    \"apps\": \"lamp1 switch1 switchlamp1\","\
                                "    \"lamp1\": {"                            \
                                "        \"app\": \"lamp\","                  \
                                "        \"protocol\": \"mqtt\","             \
                                "        \"timeout\": 120,"                   \
                                "        \"server\": \"192.168.1.198\"," \
                                "        \"port\": 1883,"                     \
                                "        \"url\": \"linh/home/devices/livingroom/floorlamp1/#\","  \
                                "        \"pin\": 4"                          \
                                "    },"                                      \
                                "    \"switch1\": {"                          \
                                "        \"app\": \"switch\","                \
                                "        \"protocol\": \"mqtt\","             \
                                "        \"timeout\": 120,"                   \
                                "        \"server\": \"192.168.1.198\"," \
                                "        \"port\": 1883,"                     \
                                "        \"url\": \"linh/home/devices/livingroom/floorlamp1/#\","  \
                                "        \"surl\": \"linh/home/devices/livingroom/floorlamp1/value/set\"," \
                                "        \"pin\": 14"                         \
                                "    },"                                      \
                                "    \"switchlamp1\": {"                      \
                                "        \"app\": \"switchlamp\","            \
                                "        \"protocol\": \"mqtt\","             \
                                "        \"timeout\": 120,"                   \
                                "        \"server\": \"192.168.1.198\"," \
                                "        \"port\": 1883,"                     \
                                "        \"url\": \"linh/home/devices/livingroom/green_lamp/value/#\","  \
                                "        \"surl\": \"linh/home/devices/livingroom/green_lamp/value/set\"," \
                                "        \"swpin\": 13,"                      \
                                "        \"lpin\": 5"                         \
                                "    },"                                      \
                                "    \"wifi_ssid\":\"Vinh 1102\","     \
                                "    \"wifi_pass\":\"224466882\""              \
                                "}"
#define DEVICE_TELNET_PORT      (2323)

#define CONFIG_IN_FLASH         (0)

#define CLIENT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST           "test.mosquitto.org" //or "mqtt.yourdomain.com"
#define MQTT_PORT           1883
#define MQTT_BUF_SIZE       1024
#define MQTT_KEEPALIVE      120  /*second*/

#define MQTT_CLIENT_ID      "DVES_%08X"
#define MQTT_USER           ""
#define MQTT_PASS           ""

#define MQTT_RECONNECT_TIMEOUT  5   /*second*/

#define DEFAULT_SECURITY    0
#define QUEUE_BUFFER_SIZE               2048

#define PROTOCOL_NAMEv31    /*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311         /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/
#endif
