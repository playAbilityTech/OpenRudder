#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <stdint.h>

void gpio_pins_init();
bool read_gpio(uint64_t now);
void write_gpio();
void gpio_process_pending_dir();

#endif
