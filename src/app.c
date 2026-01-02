#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "battery.h"
#include "ble.h"
#include "lcd.h"
#include "sensor.h"
#include "settings.h"

RAM uint32_t last_delay = 0xFFFF0000, last_battery_delay = 0xFFFF0000;
RAM bool last_smiley;
int16_t temp = 0;
uint16_t humi = 0;
RAM uint8_t adv_count = 0;
RAM uint8_t meas_count = 254;
RAM int16_t last_temp;
RAM uint16_t last_humi;
RAM uint8_t battery_level;
RAM uint16_t battery_mv;
RAM bool show_batt_or_humi;

RAM int16_t comfort_x[] = {2000, 2560, 2700, 2500, 2050, 1700, 1600, 1750};
RAM uint16_t comfort_y[] = {2000, 1980, 3200, 6000, 8200, 8600, 7700, 3800};

void user_init_normal(void){
    random_generator_init();
    init_ble();
    init_sensor();
    init_lcd();
    show_atc_mac();
    show_fw_version();
    battery_mv = get_battery_mv();
    battery_level = get_battery_level(get_battery_mv());
}

_attribute_ram_code_ void user_init_deepRetn(void){
    init_lcd_deepsleep();
    blc_ll_initBasicMCU();
    rf_set_power_level_index (RF_POWER_P3p01dBm);
    blc_ll_recoverDeepRetention();
}

void main_loop(){
    if ((clock_time()-last_delay) > 5000*CLOCK_SYS_CLOCK_1MS){

        if ((clock_time()-last_battery_delay) > 5*60000*CLOCK_SYS_CLOCK_1MS){
            battery_mv = get_battery_mv();
            battery_level = get_battery_level(get_battery_mv());
            last_battery_delay = clock_time();
        }

        if (meas_count >= CONF_MEASUREMENT_ITERATIONS){
            read_sensor(&temp, &humi);
            temp += CONF_TEMP_OFFSET;
            humi += CONF_HUMI_OFFSET;
            meas_count=0;

            if (temp != last_temp || humi != last_humi){
                if (CONF_ADV_TEMP_C_OR_F)
                    set_adv_data(((((temp*10)/5)*9)+3200)/10, humi, battery_level, battery_mv);
                else
                    set_adv_data(temp, humi, battery_level, battery_mv);
                last_temp = temp;
                last_humi = humi;
            }
        }
        meas_count++;

        if (CONF_LCD_TEMP_C_OR_F){
            show_temp_symbol(2);
            show_big_number(((((last_temp*10)/5)*9)+3200)/10, 1);
        }else{
            show_temp_symbol(1);
            show_big_number(last_temp, 1);
        }

        if (!CONF_LCD_BATTERY_INDICATOR) show_batt_or_humi = true;

        if (show_batt_or_humi){
            show_small_number(last_humi, 1);
            show_battery_symbol(0);
        }else{
            show_small_number(((battery_level==100) ? 99 : battery_level), 1);
            show_battery_symbol(1);
        }

        show_batt_or_humi = !show_batt_or_humi;

        if (ble_get_connected()){
            ble_send_temp(last_temp);
            ble_send_humi(last_humi);
            ble_send_battery(battery_level);
        }

        update_lcd();
        last_delay = clock_time();
    }
    blt_sdk_main_loop();
    blt_pm_proc();
}
