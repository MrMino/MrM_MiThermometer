#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/user_config.h"
#include "app_config.h"
#include "drivers/8258/gpio_8258.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "ble.h"
#include "settings.h"

// BTHome v2 (unencrypted) ADV
// Includes Flags (recommended) + Service Data (UUID 0xFCD2)
//
// Objects (in required numeric order):
// 0x00 packet id (uint8)            (optional but useful)
// 0x01 battery % (uint8)
// 0x02 temperature (sint16, 0.01°C)
// 0x03 humidity (uint16, 0.01%)
// 0x0C voltage (uint16, 0.001V)     (battery_mv fits perfectly: mV -> factor 0.001V)
RAM uint8_t advertising_data_BTHome[] = {
    0x02, 0x01, 0x06,  // Flags
    0x11, 0x16, 0xD2, 0xFC,  // Service Data: len=0x11, type=0x16, UUID=0xFCD2 (D2 FC)
    0x40,  // Device Info: 0x40 = BTHome v2, unencrypted, regular interval
    0x00, 0x00,  // 0x00 packet id (optional)
    0x01, 0x00,  // 0x01 battery (%)
    0x02, 0x00, 0x00,  // 0x02 temperature (0.01°C, little-endian)
    0x03, 0x00, 0x00,  // 0x03 humidity (0.01%, little-endian)
    0x0C, 0x00, 0x00,  // 0x0C voltage (0.001V, little-endian) -> battery_mv in mV
};

uint8_t mac_public[6];

_attribute_ram_code_ void user_set_rf_power (uint8_t e, uint8_t *p, int n)
{
    rf_set_power_level_index (RF_POWER_P3p01dBm);
}

_attribute_ram_code_ void blt_pm_proc(void)
{
    bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
}

void init_ble(){
    uint8_t  mac_random_static[6];
    blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

    // Controller Initialization
    blc_ll_initBasicMCU();
    blc_ll_initStandby_module(mac_public);
    blc_ll_initAdvertising_module(mac_public);
    blc_ll_initConnection_module();
    blc_ll_initSlaveRole_module();
    blc_ll_initPowerManagement_module();

    // User application initialization
    bls_ll_setAdvParam(
        CONF_ADV_INTERVAL, CONF_ADV_INTERVAL+50, ADV_TYPE_NONCONNECTABLE_UNDIRECTED,
        OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE
    );
    bls_ll_setAdvEnable(1);
    user_set_rf_power(0, 0, 0);
    bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);

    // Power Management initialization
    blc_ll_initPowerManagement_module();
    bls_pm_setSuspendMask(SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV);
    blc_pm_setDeepsleepRetentionThreshold(95, 95);
    blc_pm_setDeepsleepRetentionEarlyWakeupTiming(240);
    blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K);
}

void set_adv_data(int16_t temp, uint16_t humi, uint8_t battery_level, uint16_t battery_mv){
    uint16_t humi_0_01 = humi * 100;
    int16_t temp_0_01 = temp * 10;
    advertising_data_BTHome[9]++;
    advertising_data_BTHome[11] = battery_level;
    advertising_data_BTHome[13] = (uint8_t)(temp_0_01 & 0xFF);
    advertising_data_BTHome[14] = (uint8_t)((temp_0_01 >> 8) & 0xFF);
    advertising_data_BTHome[16] = (uint8_t)(humi_0_01 & 0xFF);
    advertising_data_BTHome[17] = (uint8_t)((humi_0_01 >> 8) & 0xFF);
    advertising_data_BTHome[19] = (uint8_t)(battery_mv & 0xFF);
    advertising_data_BTHome[20] = (uint8_t)((battery_mv >> 8) & 0xFF);

    bls_ll_setAdvData((uint8_t *)advertising_data_BTHome, sizeof(advertising_data_BTHome));
}
