#pragma once

#include <stdbool.h>
#include <stdint.h>

bool mic_level_init(void);
uint16_t mic_level_get(void);
void mic_level_reset(void);
