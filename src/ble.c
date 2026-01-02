#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/user_config.h"
#include "app_config.h"
#include "drivers/8258/gpio_8258.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "ble.h"
#include "lcd.h"
#include "settings.h"

RAM uint8_t ble_connected = 0;

extern uint8_t my_tempVal[2];
extern uint8_t my_humiVal[2];
extern uint8_t my_batVal[1];

RAM uint8_t ble_name[] = {11, 0x09, 'M', 'r', 'M', '_', '0', '0', '0', '0', '0', '0'};

RAM bool show_temp_humi_Mi = true;

RAM uint8_t advertising_data_Mi[] = {
    21, 0x16, 0x95, 0xfe,  // Description
    0x50, 0x30,  // Start
    0x5B, 0x05, // Device ID
    0x00,  // Counter
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // MAC
    0x0D, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00,  // Alternating: temp+hump / batt lvl
};

RAM uint8_t advertising_data[] = {
    16, 0x16, 0x1a, 0x18,  // Description
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // MAC
    0xaa, 0xaa,  // Temperature
    0xbb,  // Humidity
    0xcc,  // Battery level (%)
    0xdd, 0xdd,  // Battery level (mV)
    0x00  // Counter
};

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

void app_switch_to_indirect_adv(uint8_t e, uint8_t *p, int n)
{
    bls_ll_setAdvParam(CONF_ADV_INTERVAL, CONF_ADV_INTERVAL+50, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    bls_ll_setAdvEnable(1);
}

void ble_disconnect_callback(uint8_t e, uint8_t *p, int n)
{
    ble_connected = 0;
    show_ble_symbol(0);
    update_lcd();
}

_attribute_ram_code_ void user_set_rf_power (uint8_t e, uint8_t *p, int n)
{
    rf_set_power_level_index (RF_POWER_P3p01dBm);
}

void ble_connect_callback(uint8_t e, uint8_t *p, int n)
{
    ble_connected = 1;
    bls_l2cap_requestConnParamUpdate(8, 8, 99, 400);
    show_ble_symbol(1);
    update_lcd();
}

_attribute_ram_code_ void blt_pm_proc(void)
{
    bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
}

void init_ble(){
    uint8_t  mac_random_static[6];
    blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

    //Set the BLE Name to the last three MACs the first ones are always the same
    const char* hex_ascii = {"0123456789ABCDEF"};
    ble_name[6] = hex_ascii[mac_public[2]>>4];
    ble_name[7] = hex_ascii[mac_public[2] &0x0f];
    ble_name[8] = hex_ascii[mac_public[1]>>4];
    ble_name[9] = hex_ascii[mac_public[1] &0x0f];
    ble_name[10] = hex_ascii[mac_public[0]>>4];
    ble_name[11] = hex_ascii[mac_public[0] &0x0f];

    advertising_data[4] = mac_public[5];
    advertising_data[5] = mac_public[4];
    advertising_data[6] = mac_public[3];
    advertising_data[7] = mac_public[2];
    advertising_data[8] = mac_public[1];
    advertising_data[9] = mac_public[0];

    advertising_data_Mi[9] = mac_public[0];
    advertising_data_Mi[10] = mac_public[1];
    advertising_data_Mi[11] = mac_public[2];
    advertising_data_Mi[12] = mac_public[3];
    advertising_data_Mi[13] = mac_public[4];
    advertising_data_Mi[14] = mac_public[5];

    // Controller Initialization
    blc_ll_initBasicMCU();
    blc_ll_initStandby_module(mac_public);
    blc_ll_initAdvertising_module(mac_public);
    blc_ll_initConnection_module();
    blc_ll_initSlaveRole_module();
    blc_ll_initPowerManagement_module();

    // Host Initialization
    blc_gap_peripheral_init();
    extern void my_att_init();
    my_att_init ();
    blc_l2cap_register_handler (blc_l2cap_packet_receive);
    blc_smp_setSecurityLevel(No_Security);

    // User application initialization
    bls_ll_setScanRspData((uint8_t *)ble_name, sizeof(ble_name));
    bls_ll_setAdvParam(
        CONF_ADV_INTERVAL, CONF_ADV_INTERVAL+50, ADV_TYPE_CONNECTABLE_UNDIRECTED,
        OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE
    );
    bls_ll_setAdvEnable(1);
    user_set_rf_power(0, 0, 0);
    bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);
    bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &ble_connect_callback);
    bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &ble_disconnect_callback);

    // Power Management initialization
    blc_ll_initPowerManagement_module();
    bls_pm_setSuspendMask(SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
    blc_pm_setDeepsleepRetentionThreshold(95, 95);
    blc_pm_setDeepsleepRetentionEarlyWakeupTiming(240);
    blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K);
}

bool ble_get_connected(){
    return ble_connected;
}

void set_adv_data(int16_t temp, uint16_t humi, uint8_t battery_level, uint16_t battery_mv){
    if (CONF_ADV_FORMAT == ADV_FORMAT_MI){
        humi = humi * 10;

        advertising_data_Mi[8]++;

        if (show_temp_humi_Mi){
            advertising_data_Mi[15] = 0x0d;
            advertising_data_Mi[17] = 0x04;
            advertising_data_Mi[18] = temp&0xff;
            advertising_data_Mi[19] = temp>>8;
            advertising_data_Mi[20] = humi&0xff;
            advertising_data_Mi[21] = humi>>8;
        }else{
            advertising_data_Mi[15] = 0x0a;
            advertising_data_Mi[17] = 0x01;
            advertising_data_Mi[18] = battery_level;
            advertising_data_Mi[19] = 0x00;
            advertising_data_Mi[20] = 0x00;
            advertising_data_Mi[21] = 0x00;
        }
        show_temp_humi_Mi = !show_temp_humi_Mi;

        bls_ll_setAdvData((uint8_t *)advertising_data_Mi, sizeof(advertising_data_Mi));
    }else if (CONF_ADV_FORMAT == ADV_FORMAT_CUSTOM_ATC){
        advertising_data[10] = temp>>8;
        advertising_data[11] = temp&0xff;
        advertising_data[12] = humi&0xff;
        advertising_data[13] = battery_level;
        advertising_data[14] = battery_mv>>8;
        advertising_data[15] = battery_mv&0xff;
        advertising_data[16]++;

        bls_ll_setAdvData((uint8_t *)advertising_data, sizeof(advertising_data));
    }else if (CONF_ADV_FORMAT == ADV_FORMAT_BTHOME_V2){
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
}

void ble_send_temp(uint16_t temp){
    my_tempVal[0] = temp & 0xFF;
    my_tempVal[1] = temp >> 8;
    bls_att_pushNotifyData(TEMP_LEVEL_INPUT_DP_H, my_tempVal, 2);
}

void ble_send_humi(uint16_t humi){
    humi*=100;
    my_humiVal[0] = humi & 0xFF;
    my_humiVal[1] = humi >> 8;
    bls_att_pushNotifyData(HUMI_LEVEL_INPUT_DP_H, (uint8_t *)my_humiVal, 2);
}

void ble_send_battery(uint8_t value){
    my_batVal[0] = value;
    bls_att_pushNotifyData(BATT_LEVEL_INPUT_DP_H, (uint8_t *)my_batVal, 1);
}
