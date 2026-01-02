#include "tl_common.h"

#include "stack/ble/ble.h"

typedef struct
{
    /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMin;
    /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMax;
    /** Number of LL latency connection events (0x0000 - 0x03e8) */
    u16 latency;
    /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
    u16 timeout;
} gap_periConnectParams_t;

static const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;
static const u16 extReportRefUUID = GATT_UUID_EXT_REPORT_REF;
static const u16 reportRefUUID = GATT_UUID_REPORT_REF;
static const u16 characterPresentFormatUUID = GATT_UUID_CHAR_PRESENT_FORMAT;
static const u16 userdesc_UUID = GATT_UUID_CHAR_USER_DESC;
static const u16 serviceChangeUUID = GATT_UUID_SERVICE_CHANGE;
static const u16 primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;
static const u16 characterUUID = GATT_UUID_CHARACTER;
static const u16 devServiceUUID = SERVICE_UUID_DEVICE_INFORMATION;
static const u16 PnPUUID = CHARACTERISTIC_UUID_PNP_ID;
static const u16 devNameUUID = GATT_UUID_DEVICE_NAME;
static const u16 gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;
static const u16 appearanceUIID = GATT_UUID_APPEARANCE;
static const u16 periConnParamUUID = GATT_UUID_PERI_CONN_PARAM;
static const u16 appearance = GAP_APPEARE_UNKNOWN;
static const u16 gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;
static const gap_periConnectParams_t periConnParameters = {20, 40, 0, 1000};
static u16 serviceChangeVal[2] = {0};
static u8 serviceChangeCCC[2] = {0, 0};
static const u8 devName[] = {'A', 'T', 'C'};
static const u8 PnPtrs [] = {0x02, 0x8a, 0x24, 0x66, 0x82, 0x01, 0x00};

// Battery
static const u16 batServiceUUID = SERVICE_UUID_BATTERY;
static const u16 batCharUUID = CHARACTERISTIC_UUID_BATTERY_LEVEL;
static u8 batteryValueInCCC[2];
RAM u8 batVal[1] = {100};

// Temp
static const u16 tempServiceUUID = 0x181A;
static const u16 tempCharUUID = 0x2A1F;
static const u16 humiCharUUID = 0x2A6F;
static u8 tempValueInCCC[2];
RAM u8 tempVal[2] = {0};
static u8 humiValueInCCC[2];
RAM u8 humiVal[2] = {0};

//0x95FE
static const u16 FE95_ServiceUUID = 0xFE95;
static const u8 MiName[] = {'M', 'i'};

// Include attribute (Battery service)
static const u16 include[3] = {BATT_PS_H, BATT_LEVEL_INPUT_CCB_H, SERVICE_UUID_BATTERY};


// GAP attribute values
static const u8 devNameCharVal[5] = {
    CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(GenericAccess_DeviceName_DP_H), U16_HI(GenericAccess_DeviceName_DP_H),
    U16_LO(GATT_UUID_DEVICE_NAME), U16_HI(GATT_UUID_DEVICE_NAME)
};
static const u8 appearanceCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_Appearance_DP_H), U16_HI(GenericAccess_Appearance_DP_H),
    U16_LO(GATT_UUID_APPEARANCE), U16_HI(GATT_UUID_APPEARANCE)
};
static const u8 periConnParamCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(CONN_PARAM_DP_H), U16_HI(CONN_PARAM_DP_H),
    U16_LO(GATT_UUID_PERI_CONN_PARAM), U16_HI(GATT_UUID_PERI_CONN_PARAM)
};

// GATT attribute values
static const u8 serviceChangeCharVal[5] = {
    CHAR_PROP_INDICATE,
    U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
    U16_LO(GATT_UUID_SERVICE_CHANGE), U16_HI(GATT_UUID_SERVICE_CHANGE)
};

// Battery attribute values
static const u8 batCharVal[5] = {
    CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(BATT_LEVEL_INPUT_DP_H), U16_HI(BATT_LEVEL_INPUT_DP_H),
    U16_LO(CHARACTERISTIC_UUID_BATTERY_LEVEL), U16_HI(CHARACTERISTIC_UUID_BATTERY_LEVEL)
};

// Temp attribute values
static const u8 tempCharVal[5] = {
    CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(TEMP_LEVEL_INPUT_DP_H), U16_HI(TEMP_LEVEL_INPUT_DP_H),
    U16_LO(0x2A1F), U16_HI(0x2A1F)
};

// Humi attribute values
static const u8 humiCharVal[5] = {
    CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(HUMI_LEVEL_INPUT_DP_H), U16_HI(HUMI_LEVEL_INPUT_DP_H),
    U16_LO(0x2A6F), U16_HI(0x2A6F)
};

static const attribute_t Attributes[] = {
    {ATT_END_H - 1, 0, 0, 0, 0, 0},    // total num of attribute
    // 0001 - 0007 gap
    {7, ATT_PERMISSIONS_READ, 2, 2, (u8*)(&primaryServiceUUID), (u8*)(&gapServiceUUID), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(devNameCharVal), (u8*)(&characterUUID), (u8*)(devNameCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(devName), (u8*)(&devNameUUID), (u8*)(devName), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(appearanceCharVal), (u8*)(&characterUUID), (u8*)(appearanceCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(appearance), (u8*)(&appearanceUIID), (u8*)(&appearance), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(periConnParamCharVal), (u8*)(&characterUUID), (u8*)(periConnParamCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(periConnParameters), (u8*)(&periConnParamUUID), (u8*)(&periConnParameters), 0},
    // 0008 - 000b gatt
    {4, ATT_PERMISSIONS_READ, 2, 2, (u8*)(&primaryServiceUUID), (u8*)(&gattServiceUUID), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(serviceChangeCharVal), (u8*)(&characterUUID), (u8*)(serviceChangeCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(serviceChangeVal), (u8*)(&serviceChangeUUID), (u8*)(&serviceChangeVal), 0},
    {0, ATT_PERMISSIONS_RDWR, 2, sizeof(serviceChangeCCC), (u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},
    // Battery Service
    {4, ATT_PERMISSIONS_READ, 2, 2, (u8*)(&primaryServiceUUID), (u8*)(&batServiceUUID), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(batCharVal), (u8*)(&characterUUID), (u8*)(batCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(batVal), (u8*)(&batCharUUID), (u8*)(batVal), 0},
    {0, ATT_PERMISSIONS_RDWR, 2, sizeof(batteryValueInCCC), (u8*)(&clientCharacterCfgUUID), (u8*)(batteryValueInCCC), 0},
    // Temp Service
    {7, ATT_PERMISSIONS_READ, 2, 2, (u8*)(&primaryServiceUUID), (u8*)(&tempServiceUUID), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(tempCharVal), (u8*)(&characterUUID), (u8*)(tempCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(tempVal), (u8*)(&tempCharUUID), (u8*)(tempVal), 0},
    {0, ATT_PERMISSIONS_RDWR, 2, sizeof(tempValueInCCC), (u8*)(&clientCharacterCfgUUID), (u8*)(tempValueInCCC), 0},

    {0, ATT_PERMISSIONS_READ, 2, sizeof(humiCharVal), (u8*)(&characterUUID), (u8*)(humiCharVal), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(humiVal), (u8*)(&humiCharUUID), (u8*)(humiVal), 0},
    {0, ATT_PERMISSIONS_RDWR, 2, sizeof(humiValueInCCC), (u8*)(&clientCharacterCfgUUID), (u8*)(humiValueInCCC), 0},
    // Mi 0x95FE
    {2, ATT_PERMISSIONS_READ, 2, 2, (u8*)(&primaryServiceUUID), (u8*)(&FE95_ServiceUUID), 0},
    {0, ATT_PERMISSIONS_READ, 2, sizeof(MiName), (u8*)(&userdesc_UUID), (u8*)(MiName), 0},
};

void att_init(void)
{
    bls_att_setAttributeTable ((u8*) Attributes);
}
