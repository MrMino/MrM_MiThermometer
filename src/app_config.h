#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#define CLOCK_SYS_CLOCK_HZ 24000000

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

enum{
    CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
    CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
    CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

typedef enum
{
    ATT_H_START = 0,

    // GAP
    GenericAccess_PS_H,
    GenericAccess_DeviceName_CD_H,
    GenericAccess_DeviceName_DP_H,
    GenericAccess_Appearance_CD_H,
    GenericAccess_Appearance_DP_H,
    CONN_PARAM_CD_H,
    CONN_PARAM_DP_H,

    // GATT
    GenericAttribute_PS_H,
    GenericAttribute_ServiceChanged_CD_H,
    GenericAttribute_ServiceChanged_DP_H,
    GenericAttribute_ServiceChanged_CCB_H,

    // Battery service
    BATT_PS_H,
    BATT_LEVEL_INPUT_CD_H,
    BATT_LEVEL_INPUT_DP_H,
    BATT_LEVEL_INPUT_CCB_H,

    // Temperature service
    TEMP_PS_H,
    TEMP_LEVEL_INPUT_CD_H,
    TEMP_LEVEL_INPUT_DP_H,
    TEMP_LEVEL_INPUT_CCB_H,

    HUMI_LEVEL_INPUT_CD_H,
    HUMI_LEVEL_INPUT_DP_H,
    HUMI_LEVEL_INPUT_CCB_H,

    // Mi Advertising
    Mi_PS_H,
    Mi_CMD_OUT_DESC_H,

    ATT_END_H,

} ATT_HANDLE;

#include "vendor/common/default_config.h"

#if defined(__cplusplus)
}
#endif
