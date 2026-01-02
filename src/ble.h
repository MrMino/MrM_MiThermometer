#pragma once

#include <stdbool.h>
#include <stdint.h>

void init_ble();
void set_adv_data(int16_t temp, uint16_t humi, uint8_t battery_level, uint16_t battery_mv);
void blt_pm_proc(void);
