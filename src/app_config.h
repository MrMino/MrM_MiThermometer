#pragma once

#define CLOCK_SYS_CLOCK_HZ 24000000

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

enum{
    CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
    CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
    CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

#include "vendor/common/default_config.h"
