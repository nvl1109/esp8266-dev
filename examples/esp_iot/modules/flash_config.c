/*
/* flash_config.c
*
* Copyright (c) 2015, LinhNV (nvl1109@gmail.com)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "flash_config.h"
#include "user_config.h"
#include "debug.h"

void ICACHE_FLASH_ATTR
CFG_save( char * pstr ) {
    SpiFlashOpResult res;
    uint32_t write_length = CFG_CONFIG_SIZE;
    if (pstr == NULL) {
        INFO("ERR: Save flash config of NULL pointer\r\n");
        return;
    }

    res = spi_flash_erase_sector(CFG_LOCATION);
    DBG_MSG("Erased with result %d\r\n", res);

    if (os_strlen(pstr) < write_length)
        write_length = os_strlen(pstr) + 1;

    if (os_strlen(pstr) <= 0) {
        INFO("ERR: Config  is empty\r\n");
        return;
    }

    res = spi_flash_write((CFG_LOCATION) * SPI_FLASH_SEC_SIZE, (uint32 *)pstr, write_length);
    DBG_MSG("flash 0x%08X written with result %d, %d bytes\r\n", (CFG_LOCATION) * SPI_FLASH_SEC_SIZE,
            res, write_length);
}

char * ICACHE_FLASH_ATTR
CFG_read( void ) {
    SpiFlashOpResult res = SPI_FLASH_RESULT_ERR;
    char * pstr = (char *)os_malloc(CFG_CONFIG_SIZE);

    if (pstr != NULL) {
        DBG_MSG("Read flash config at 0x%08X and save to str with pointer %p\r\n", (CFG_LOCATION) * SPI_FLASH_SEC_SIZE, pstr);
        res = spi_flash_read((CFG_LOCATION) * SPI_FLASH_SEC_SIZE, (uint32 *)pstr, CFG_CONFIG_SIZE);
        DBG_MSG("   Result: %d\r\n", res);
        if (res != SPI_FLASH_RESULT_OK) {
            os_free(pstr);
            INFO("ERR: Can not read from spi flash, result: %d\r\n", res);
            return NULL;
        }
    } else {
        INFO("ERR: Malloc memory with %d bytes is failed\r\n", CFG_CONFIG_SIZE);
    }
    return pstr;
}
