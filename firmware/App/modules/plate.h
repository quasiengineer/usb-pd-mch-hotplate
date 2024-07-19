#pragma once

#include <stdint.h>

#define PROGRAM_COUNT                 3
#define MAX_PROGRAM_STAGES            5
#define TEMPERATURE_CHANGE_MAX_RATE   INT8_MAX

typedef enum {
  PlateProgramTickAction_Continue = 0,
  PlateProgramTickAction_Finished,
  PlateProgramTickAction_Error
} PlateProgramTickAction;

typedef struct {
  char * stageName;
  uint8_t targetTemperature;
  uint8_t timeout;
  int8_t tempChange5sMin;
  int8_t tempChange5sMax;
} PlateProgramStage;

typedef struct {
  char * programName;
  PlateProgramStage stages[MAX_PROGRAM_STAGES];
} PlateProgram;

extern PlateProgram programs[PROGRAM_COUNT];

// returns 1 when finished
PlateProgramTickAction plateProgramTick();

void startProgram(PlateProgram * program);
void shutdownPlate();