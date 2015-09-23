// this is the normal build target ESP include set
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "upgrade.h"

#include "telnet_server.h"
#include "debug.h"
#include "config.h"
#include "protocol_manager.h"

#define MSG_OK "\r\n>OK\r\n"
#define MSG_ERROR "\r\n>ERROR\r\n"
#define MSG_INVALID_CMD "\r\n>UNKNOWN COMMAND\r\n"

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

#define MAX_ARGS 4
#define MSG_BUF_LEN 128

#ifdef OTAENABLED
void ICACHE_FLASH_ATTR ota_upgrade(serverConnData *conn, uint8_t argc, char *argv[]);
static void ICACHE_FLASH_ATTR handleUpgrade(uint8_t serverVersion, const char *server_ip, uint16_t port, const char *path);
#endif
void ICACHE_FLASH_ATTR config_restart(serverConnData *conn, uint8_t argc, char *argv[]);
void ICACHE_FLASH_ATTR config_help(serverConnData *conn, uint8_t argc, char *argv[]);
void ICACHE_FLASH_ATTR config_get(serverConnData *conn, uint8_t argc, char *argv[]);
void ICACHE_FLASH_ATTR config_set(serverConnData *conn, uint8_t argc, char *argv[]);
void ICACHE_FLASH_ATTR config_save(serverConnData *conn, uint8_t argc, char *argv[]);
static ICACHE_FLASH_ATTR config_get_all(serverConnData *conn);
static ICACHE_FLASH_ATTR config_get_one(serverConnData *conn, char *key);

const config_commands_t config_commands[] = {
#ifdef OTAENABLED
        { "OTA", &ota_upgrade },
#endif
        { "RST", &config_restart },
        { "help", &config_help },
        { "get", &config_get },
        { "set", &config_set },
        { "save", &config_save },
        { NULL, NULL }
    };

char ICACHE_FLASH_ATTR *my_strdup(char *str) {
    size_t len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = (char*)os_malloc((u_int)len)))
        return (NULL);
    os_memcpy(copy, str, len);
    return (copy);
}

char ICACHE_FLASH_ATTR **config_parse_args(char *buf, uint8_t *argc) {
    const char delim[] = " \t";
    char *save, *tok;
    char **argv = (char **)os_malloc(sizeof(char *) * MAX_ARGS);    // note fixed length
    os_memset(argv, 0, sizeof(char *) * MAX_ARGS);

    *argc = 0;
    for (; *buf == ' ' || *buf == '\t'; ++buf); // absorb leading spaces
    for (tok = strtok_r(buf, delim, &save); tok; tok = strtok_r(NULL, delim, &save)) {
        argv[*argc] = my_strdup(tok);
        (*argc)++;
        if (*argc == MAX_ARGS) {
            break;
        }
    }
    return argv;
}

void ICACHE_FLASH_ATTR config_parse_args_free(uint8_t argc, char *argv[]) {
    uint8_t i;
    for (i = 0; i <= argc; ++i) {
        if (argv[i])
            os_free(argv[i]);
    }
    os_free(argv);
}

void ICACHE_FLASH_ATTR config_help(serverConnData *conn, uint8_t argc, char *argv[]) {
    bool err = false;
    if (argc != 0)
        espbuffsentstring(conn, "usage: help\r\n" MSG_OK);
    else {
        espbuffsentstring(conn, "Supported command list:\r\n"
                                 "help\r\n"
                                 "  print this help\r\n"
                                 "get [config key]\r\n"
                                 "  Print value of config key or all value. Config level are splited by '/'.\r\n"
                                 "set <config key> [value]\r\n"
                                 "  Set value for config key. If value argument does not pass, the config key will be deleted.\r\n"
                                 "  value start with type indicator character. i for number, s for string.\r\n"
                                 "  Example i10, slamp.\r\n"
                                 "save\r\n"
                                 "  Save current config into FLASH for later used.\r\n"
                                 "RST\r\n"
                                 "  restart the board\r\n"
#ifdef OTAENABLED
                                 "OTA <SRV IP> <PORT> <FOLDER PATH>\r\n"
                                 "  OTA upgrade user firmware from network\r\n"
#endif
                                 MSG_OK);
    }
}
static ICACHE_FLASH_ATTR config_get_all(serverConnData *conn) {
    uint8_t i;
    cJSON * jtmp = NULL;
    char *ptmp = NULL;

    if (jconfig_root == NULL) return;

    for (i=0; i<cJSON_GetArraySize(jconfig_root); ++i) {
        jtmp = cJSON_GetArrayItem(jconfig_root, i);
        DBG_MSG("Item %d: %s\r\n", i, jtmp->string);
        ptmp = cJSON_PrintUnformatted(jtmp);
        espbuffsentprintf(conn, "%s%s\r\n", jtmp->string, ptmp);
        os_free(ptmp);
    }
}
static cJSON * ICACHE_FLASH_ATTR get_parent_by_key(cJSON *parent, char *key, char **lastchild) {
    uint8_t i, pre = 0, len;
    cJSON * jparent = parent;
    cJSON * jchild = NULL;
    char *ptmp = NULL;
    char tmpstr[50];

    len = os_strlen(key);
    for (i=0; i<len; ++i) {
        if (key[i] != '/') continue;
        os_bzero(tmpstr, sizeof(tmpstr));
        if (i <= pre) continue;
        os_memcpy(tmpstr, key + pre, i - pre);
        DBG_MSG("found / at %d, key: %s.\r\n", i, tmpstr);
        pre = i+1;
        jchild = cJSON_GetObjectItem(jparent, tmpstr);
        if (jchild == NULL) return NULL;
        DBG_MSG("found cJSON item %s.\r\n", jchild->string);
        jparent = jchild;
    }
    if (pre < (os_strlen(key) - 1)) {
        // Have last element
        os_bzero(tmpstr, sizeof(tmpstr));
        os_memcpy(tmpstr, key + pre, os_strlen(key) - pre);
        DBG_MSG("last element found key: %s.\r\n", tmpstr);
        *lastchild = key + pre;
    }
    return jparent;
}
static cJSON * ICACHE_FLASH_ATTR get_config_by_key(cJSON *parent, char *key) {
    uint8_t i, pre = 0;
    cJSON * jparent = parent;
    cJSON * jchild = NULL;
    char *ptmp = NULL;
    char tmpstr[50];

    for (i=0; i<os_strlen(key); ++i) {
        if (key[i] != '/') continue;
        os_bzero(tmpstr, sizeof(tmpstr));
        if (i <= pre) continue;
        os_memcpy(tmpstr, key + pre, i - pre);
        DBG_MSG("found / at %d, key: %s.\r\n", i, tmpstr);
        pre = i+1;
        jchild = cJSON_GetObjectItem(jparent, tmpstr);
        if (jchild == NULL) break;
        DBG_MSG("found cJSON item %s.\r\n", jchild->string);
        jparent = jchild;
    }
    if (pre < (os_strlen(key) - 1)) {
        // Have last element
        os_bzero(tmpstr, sizeof(tmpstr));
        os_memcpy(tmpstr, key + pre, os_strlen(key) - pre);
        DBG_MSG("last element found key: %s.\r\n", tmpstr);
        jchild = cJSON_GetObjectItem(jparent, tmpstr);
    }
    return jchild;
}
static ICACHE_FLASH_ATTR config_get_one(serverConnData *conn, char *key) {
    cJSON * jchild = NULL;
    char *ptmp = NULL;

    jchild = get_config_by_key(jconfig_root, key);
    if (jchild != NULL) {
        ptmp = cJSON_PrintUnformatted(jchild);
        espbuffsentprintf(conn, "%s\r\n", ptmp);
        os_free(ptmp);
    } else {
        espbuffsentprintf(conn, "Could not found config of key [%s]\r\n", key);
    }
}
void ICACHE_FLASH_ATTR config_get(serverConnData *conn, uint8_t argc, char *argv[]){
    bool err = false;
    if (argc == 0)
        config_get_all(conn);
    else if (argc != 1)
        err=true;
    else {
        config_get_one(conn, argv[1]);
    }
    if (err)
        espbuffsentstring(conn, MSG_ERROR);
    else
        espbuffsentstring(conn, MSG_OK);
}
static void * ICACHE_FLASH_ATTR convert_value(char *str) {

}
void ICACHE_FLASH_ATTR config_set(serverConnData *conn, uint8_t argc, char *argv[]){
    bool err = false;
    cJSON * config = NULL;
    cJSON * lastchild = NULL;
    char *stmp = NULL;

    DBG_MSG("config_set argc: %d\r\n", argc);
    if (argc == 0)
        espbuffsentprintf(conn, "Usage: set <key> [value]\r\nUse help for more details\r\n");
    else if (argc == 1) {
        // Delete config
        DBG_MSG("Delete config %s\r\n", argv[1]);
        config = get_parent_by_key(jconfig_root, argv[1], &stmp);
        if (config != NULL) lastchild = cJSON_GetObjectItem(config, stmp);
        DBG_MSG("config: %p, stmp: %p\r\n", config, stmp);
        if (config == NULL) {
            espbuffsentprintf(conn, "Could not found parent config with key [%s].\r\n", argv[1]);
        }else if(lastchild == NULL) {
            espbuffsentprintf(conn, "Could not found lastchild config with key [%s].\r\n", argv[1]);
        } else {
            char *tmp;
            DBG_MSG("Delete %s from %s.\r\n", lastchild->string, config->string);
            cJSON_DeleteItemFromObject(config, lastchild->string);
            tmp = cJSON_PrintUnformatted(config);
            espbuffsentstring(conn, "Deleted. Now content is:\r\n");
            espbuffsentstring(conn, tmp);
            os_free(tmp);
        }
    } else {
        // Update config value
        char value[255];
        uint8_t i;
        uint32_t idx = 0;

        os_bzero(value, sizeof(value));
        for (i=2; i<=argc; ++i) {
            DBG_MSG("argv[%d] = %s.\r\n", i, argv[i]);
            os_sprintf(value + idx, "%s ", argv[i]);
            idx += os_strlen(argv[i]) + 1;
        }
        value[idx - 1] = 0;
        DBG_MSG("Update config %s to %s.\r\n", argv[1], value);
        config = get_parent_by_key(jconfig_root, argv[1], &stmp);
        DBG_MSG("config: %p, stmp: %p\r\n", config, stmp);
        if (config == NULL) {
            espbuffsentprintf(conn, "Could not found parent config with key [%s].\r\n", argv[1]);
        }else if (stmp == NULL) {
            espbuffsentprintf(conn, "Could not found last config with key [%s].\r\n", argv[1]);
        } else {
            char *tmp;
            int i;
            cJSON *newchild = NULL;

            DBG_MSG("stmp: %s, valuetype: %c.\r\n", stmp, value[0]);
            lastchild = cJSON_GetObjectItem(config, stmp);
            DBG_MSG("lastchild: %p.\r\n", lastchild);
            switch (value[0]) {
                case 'i':
                if (os_strlen(value) < 2) {
                    espbuffsentstring(conn, "value is invalid format: i10\r\n");
                    break;
                }
                i = atoi(&value[1]);
                newchild = cJSON_CreateNumber(i);
                break;
                case 's':
                if (os_strlen(value) < 2) {
                    espbuffsentstring(conn, "value is invalid format: slamp\r\n");
                    break;
                }
                newchild = cJSON_CreateString(&value[1]);
                DBG_MSG("Create item: %s -> %p.\r\n", &value[1], newchild);
                break;
                default:
                    espbuffsentstring(conn, "value is invalid format\r\n");
                    newchild = NULL;
                break;
            }

            if (newchild != NULL) {
                if (lastchild != NULL) {
                    cJSON_ReplaceItemInObject(config, lastchild->string, newchild);
                } else {
                    DBG_MSG("Add config %s to %s.\r\n", stmp, config->string);
                    cJSON_AddItemToObject(config, stmp, newchild);
                }
                tmp = cJSON_PrintUnformatted(config);
                espbuffsentstring(conn, "Updated. Now content is:\r\n");
                espbuffsentstring(conn, tmp);
                os_free(tmp);
            }
        }
    }
    if (err)
        espbuffsentstring(conn, MSG_ERROR);
    else
        espbuffsentstring(conn, MSG_OK);
}
void ICACHE_FLASH_ATTR config_save(serverConnData *conn, uint8_t argc, char *argv[]){
    if (argc != 0)
        espbuffsentprintf(conn, "Usage: save\r\nUse help for more details\r\n");
    else {
        char *config;
        config = cJSON_PrintUnformatted(jconfig_root);
        CFG_save(config);
        os_free(config);
        espbuffsentstring(conn, "Config saved, please restart now.");
    }
    espbuffsentstring(conn, MSG_OK);
}

void ICACHE_FLASH_ATTR config_restart(serverConnData *conn, uint8_t argc, char *argv[]) {
    espbuffsentstring(conn, MSG_OK);
    os_delay_us(1000);
    system_restart();
}

#ifdef OTAENABLED
void ICACHE_FLASH_ATTR ota_upgrade(serverConnData *conn, uint8_t argc, char *argv[]) {
    espbuffsentprintf(conn, "START OTA UPGRADE...CUR ROM: %d.\r\n", ROMNUM);

    if (argc == 0)
        espbuffsentstring(conn, "OTA <SRV IP> <PORT> <FOLDER PATH>\r\n");
    else if (argc != 3)
        espbuffsentstring(conn, MSG_ERROR);
    else {
        uint16_t port = atoi(argv[2]);
        uint8_t iparr[4];
        uint8_t i;
        char tmp[4];
        char* pCurIp = argv[1];
        char * ch;

        DBG_MSG("OTA UPGRADE with IP:%s, port:%d, path:%s.\r\n", argv[1], argv[2], argv[3]);
        for (i=0; i<protocol_idx; ++i) {
            DBG_MSG("protocol_disconnect res: %d, index: %d\r\n", protocol_disconnect(&protocol_list[i]), i);
        }
        ch = strchr(pCurIp, '.');

        DBG_MSG("First: %s\r\n", pCurIp);
        DBG_MSG("After: %s\r\n", ch);

        for (i = 0; i < 4; ++i) {
            if (i < (ch - pCurIp + 1)) {
                tmp[i] = pCurIp[i];
            } else {
                tmp[i] = 0;
            }
        }
        iparr[0] = atoi(tmp);
        pCurIp = ch + 1;

        ch = strchr(pCurIp, '.');
        DBG_MSG("First: %s\r\n", pCurIp);
        DBG_MSG("After: %s\r\n", ch);
        for (i = 0; i < 4; ++i) {
            if (i < (ch - pCurIp + 1)) {
                tmp[i] = pCurIp[i];
            } else {
                tmp[i] = 0;
            }
        }
        iparr[1] = atoi(tmp);
        pCurIp = ch + 1;

        ch = strchr(pCurIp, '.');
        DBG_MSG("First: %s\r\n", pCurIp);
        DBG_MSG("After: %s\r\n", ch);
        for (i = 0; i < 4; ++i) {
            if (i < (ch - pCurIp + 1)) {
                tmp[i] = pCurIp[i];
            } else {
                tmp[i] = 0;
            }
        }
        iparr[2] = atoi(tmp);
        pCurIp = ch + 1;

        ch = strchr(pCurIp, '.');
        DBG_MSG("First: %s\r\n", pCurIp);
        DBG_MSG("After: %s\r\n", ch);
        for (i = 0; i < 4; ++i) {
            if (i < (argv[1] + strlen(argv[1]) - pCurIp + 1)) {
                tmp[i] = pCurIp[i];
            } else {
                tmp[i] = 0;
            }
        }
        iparr[3] = atoi(tmp);

        DBG_MSG("=> IP: %d %d %d %d\r\n", iparr[0], iparr[1], iparr[2], iparr[3]);

        if ((port == 0)||(port>65535)) {
             espbuffsentstring(conn, MSG_ERROR);
        } else {
            espbuffsentstring(conn, MSG_OK);
            handleUpgrade(2, iparr, port, argv[3]);
        }
    }
}
#endif

void ICACHE_FLASH_ATTR config_parse(serverConnData *conn, char *buf, int len) {
    char *lbuf = (char *)os_malloc(len + 1), **argv;
    uint8_t i, argc;
    // we need a '\0' end of the string
    os_memcpy(lbuf, buf, len);
    lbuf[len] = '\0';
    DBG_MSG("parse %d[%s]\r\n", len, lbuf);

    // remove any CR / LF
    for (i = 0; i < len; ++i)
        if (lbuf[i] == '\n' || lbuf[i] == '\r')
            lbuf[i] = '\0';

    // verify the command prefix
    if (os_strncmp(lbuf, "+++AT", 5) != 0) {
        DBG_MSG("Not a command\r\n");
        return;
    }
    // parse out buffer into arguments
    argv = config_parse_args(&lbuf[5], &argc);
    DBG_MSG("argc: %d, argv[0]: %s.\r\n", argc, argv[0]);

    if (argc == 0) {
        espbuffsentstring(conn, MSG_OK);
    } else {
        argc--; // to mimic C main() argc argv
        for (i = 0; config_commands[i].command; ++i) {
            if (os_strncmp(argv[0], config_commands[i].command, strlen(argv[0])) == 0) {
                config_commands[i].function(conn, argc, argv);
                break;
            }
        }
        if (!config_commands[i].command)
            espbuffsentprintf(conn, "%s - buf: %s\r\n", MSG_INVALID_CMD, argv[0]);
    }
    config_parse_args_free(argc, argv);
    os_free(lbuf);
}

#ifdef OTAENABLED
static void ICACHE_FLASH_ATTR ota_finished_callback(void *arg)
{
    struct upgrade_server_info *update = arg;
    if (update->upgrade_flag == true)
    {
        DBG_MSG("[OTA]success; rebooting!\r\n");
        system_upgrade_reboot();
    }
    else
    {
        DBG_MSG("[OTA]failed! %d - %d\r\n", update->upgrade_flag, system_upgrade_flag_check());
    }

    os_free(update->pespconn);
    os_free(update->url);
    os_free(update);
}

static void ICACHE_FLASH_ATTR handleUpgrade(uint8_t serverVersion, const char *server_ip, uint16_t port, const char *path)
{
    const char* file;
    struct upgrade_server_info* update;
    uint8_t userBin = system_upgrade_userbin_check();
    DBG_MSG("UserBIn = %d\r\n", userBin);
    switch (ROMNUM)
    {
        case 1: file = "user2.1024.new.2.bin"; break;
        case 2: file = "user1.1024.new.2.bin"; break;
        default:
            DBG_MSG("[OTA]Invalid userbin number!\r\n");
            return;
    }

    uint16_t version=1;
    if (serverVersion <= version)
    {
        DBG_MSG("[OTA]No update. Server version:%d, local version %d\r\n", serverVersion, version);
        return;
    }

    DBG_MSG("[OTA]Upgrade available version: %d\r\n", serverVersion);

    update = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
    update->pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));

    os_memcpy(update->ip, server_ip, 4);
    update->port = port;

    DBG_MSG("[OTA]Server "IPSTR":%d. Path: %s%s\r\n", IP2STR(update->ip), update->port, path, file);

    update->check_cb = ota_finished_callback;
    update->check_times = 10000;
    update->url = (uint8 *)os_zalloc(512);

    os_sprintf((char*)update->url,
        "GET %s%s HTTP/1.1\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
        path, file, IP2STR(update->ip), update->port);
    DBG_MSG("Update url: %s.\r\n", update->url);

    if (system_upgrade_start(update) == false)
    {
        DBG_MSG("[OTA]Could not start upgrade\r\n");

        os_free(update->pespconn);
        os_free(update->url);
        os_free(update);
    }
    else
    {
        DBG_MSG("[OTA]Upgrading...\r\n");
    }
}
#endif
