/* config.h
*
* Copyright (c) 2015, LinhNV <nvl1109 at gmail dot com>
* All rights reserved.
*
*/

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "gpio_mod.h"
#include "config.h"
#include "debug.h"

static uint32_t gpio_no_pins[NUMBER_OF_GPIO_PIN] = {
    PERIPHS_IO_MUX_GPIO0_U,     // GPIO0
    PERIPHS_IO_MUX_U0TXD_U,     // GPIO1
    PERIPHS_IO_MUX_GPIO2_U,     // GPIO2
    PERIPHS_IO_MUX_U0RXD_U,     // GPIO3
    PERIPHS_IO_MUX_GPIO4_U,     // GPIO4
    PERIPHS_IO_MUX_GPIO5_U,     // GPIO5
    0,
    0,
    0,
    PERIPHS_IO_MUX_SD_DATA2_U,  // GPIO9
    PERIPHS_IO_MUX_SD_DATA3_U,  // GPIO10
    0,
    PERIPHS_IO_MUX_MTDI_U,      // GPIO12
    PERIPHS_IO_MUX_MTCK_U,      // GPIO13
    PERIPHS_IO_MUX_MTMS_U,      // GPIO14
    PERIPHS_IO_MUX_MTDO_U       // GPIO15
};
static uint32_t gpio_func_arr[NUMBER_OF_GPIO_PIN] = {
    FUNC_GPIO0,
    FUNC_GPIO1,
    FUNC_GPIO2,
    FUNC_GPIO3,
    FUNC_GPIO4,
    FUNC_GPIO5,
    0,
    0,
    0,
    FUNC_GPIO9,
    FUNC_GPIO10,
    0,
    FUNC_GPIO12,
    FUNC_GPIO13,
    FUNC_GPIO14,
    FUNC_GPIO15
};

static gpio_interrupt_callback gpio_interrupt_cb[NUMBER_OF_GPIO_PIN];
static void * gpio_interrupt_arg[NUMBER_OF_GPIO_PIN];

os_event_t gpio_interrupt_procTaskQueue[GPIO_CALLBACK_TASK_QUEUE_SIZE];
static bool isTaskCreated = false;

static void ICACHE_FLASH_ATTR gpio_intr_handler(int * dummy);
void ICACHE_FLASH_ATTR gpio_interrupt_cb_task(os_event_t *e);

void ICACHE_FLASH_ATTR
gpio_interrupt_cb_task(os_event_t *e) {
    uint8_t i;
    uint32_t cbmask = (uint32_t)e->par;
    DBG_MSG("cbmask: 0x%X\r\n", cbmask);
    for (i=0; i<NUMBER_OF_GPIO_PIN; ++i) {
        if ((cbmask & BIT(i)) != 0) {
            if (gpio_interrupt_cb[i] != NULL) {
                gpio_interrupt_cb[i]((void *)gpio_interrupt_arg[i]);
            }
        }
    }
}

void ICACHE_FLASH_ATTR gpio_function(uint8_t pin_no) {
    DBG_MSG("pin_no: %d\r\n", pin_no);

    if (pin_no >= NUMBER_OF_GPIO_PIN) return;

    PIN_FUNC_SELECT(gpio_no_pins[pin_no], gpio_func_arr[pin_no]);
}

static void ICACHE_FLASH_ATTR gpio_intr_handler(int * dummy) {
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    uint8_t i;
    uint32_t cbmask = 0;
    bool res;
    //clear interrupt status for GPIO
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
    for (i=0; i<NUMBER_OF_GPIO_PIN; ++i) {
        if (gpio_status & BIT(i)) {
            DBG_MSG("Interrupt %d\r\n", i);
            cbmask |= BIT(i);
        }
    }
    DBG_MSG("cbmask: 0x%X\r\n", cbmask);
    res = system_os_post(GPIO_CALLBACK_TASK_PRIO, 0, (os_param_t)cbmask);
    DBG_MSG("system_os_post result %d\r\n", res);
}

void ICACHE_FLASH_ATTR gpio_interrupt_start(void) {

    if (isTaskCreated == false) {
        system_os_task(gpio_interrupt_cb_task, GPIO_CALLBACK_TASK_PRIO, gpio_interrupt_procTaskQueue, GPIO_CALLBACK_TASK_QUEUE_SIZE);
        isTaskCreated = true;
        DBG_MSG("Create gpio interrupt callback task\r\n");
    }

    ETS_GPIO_INTR_DISABLE();
    ETS_GPIO_INTR_ATTACH(gpio_intr_handler, NULL);
    // enable interrupt for his GPIO
    //     GPIO_PIN_INTR_... defined in gpio.h
    ETS_GPIO_INTR_ENABLE();
}
void ICACHE_FLASH_ATTR gpio_interrupt_stop(void) {
    ETS_GPIO_INTR_ATTACH(NULL, NULL);
    ETS_GPIO_INTR_DISABLE();
}
void ICACHE_FLASH_ATTR gpio_interrupt_install(uint8_t pin_no, GPIO_INT_TYPE type, gpio_interrupt_callback cb, void *arg) {
    gpio_function(pin_no);
    // All people repeat this mantra but I don't know what it means
    gpio_register_set(GPIO_PIN_ADDR(pin_no),
                      GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)  |
                      GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE) |
                      GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

    // clear gpio status. Say ESP8266EX SDK Programming Guide in  5.1.6. GPIO interrupt handler
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin_no));
    gpio_pin_intr_state_set(GPIO_ID_PIN(pin_no), type);
    gpio_interrupt_cb[pin_no] = cb;
    gpio_interrupt_arg[pin_no] = arg;
}
