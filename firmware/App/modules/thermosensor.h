#pragma once

#include <stdint.h>

#define INCORRECT_TEMP_READING 255

// 0..254 degree in celsius
uint8_t getPlateTemperature();
