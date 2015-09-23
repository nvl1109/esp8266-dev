/* config.h
*
* Copyright (c) 2015, LinhNV <nvl1109 at gmail dot com>
* All rights reserved.
*
*/

#ifndef GPIO_MOD_H_
#define GPIO_MOD_H_

#include "gpio.h"

#define NUMBER_OF_GPIO_PIN  (16)
#define GPIO_CALLBACK_TASK_QUEUE_SIZE (1)
#define GPIO_CALLBACK_TASK_PRIO       (2)

typedef void (*gpio_interrupt_callback)(void *);

/**
 * @brief Set pin function as gpio
 */
void ICACHE_FLASH_ATTR gpio_function(uint8_t pin_no);

void ICACHE_FLASH_ATTR gpio_interrupt_start(void);
void ICACHE_FLASH_ATTR gpio_interrupt_stop(void);
void ICACHE_FLASH_ATTR gpio_interrupt_install(uint8_t pin_no, GPIO_INT_TYPE type, gpio_interrupt_callback cb, void *arg);

#endif /* GPIO_MOD_H_ */
