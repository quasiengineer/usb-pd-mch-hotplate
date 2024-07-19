#pragma once

#include <stdint.h>

void pd_startSinking(uint16_t voltageInMilliVolts, uint16_t currentInMilliAmps);

// return 1 when source informed that desired voltage and current would be provided
uint8_t pd_isReady();