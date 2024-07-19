#include "main.h"
#include "plate.h"
#include "thermosensor.h"
#include "display.h"

// we can't do it too granularly, because we have 5s periods of measurement
#define PWM_DUTY_STEP_IN_PERCENTS   10
#define INITIAL_PWM_DUTY            50

#define SCREEN_REFRESH_PERIOD_IN_MS 500

PlateProgram programs[PROGRAM_COUNT] = {
  {
    .programName = "Sn63Pb37",
    .stages = {
      { .stageName = "preheat", .targetTemperature = 135, .timeout = 90, .tempChange5sMin = 10, .tempChange5sMax = 15 },
      { .stageName = "soaking", .targetTemperature = 185, .timeout = 120, .tempChange5sMin = 3, .tempChange5sMax = 5 },
      { .stageName = "reflow rise", .targetTemperature = 215, .timeout = 60, .tempChange5sMin = TEMPERATURE_CHANGE_MAX_RATE },
      { .stageName = "reflow", .targetTemperature = 215, .timeout = 40, .tempChange5sMax = 0 },
      { .stageName = "cool down", .targetTemperature = 150, .timeout = 200, .tempChange5sMin = -10, .tempChange5sMax = -20 },
    }
  },
  {
    .programName = "80` constant",
    .stages = {
      { .stageName = "preheat", .targetTemperature = 80, .timeout = 40, .tempChange5sMin = 10, .tempChange5sMax = 20 },
      { .stageName = "keep", .targetTemperature = 80, .timeout = 0, .tempChange5sMax = 0 },
    }
  },
  {
    .programName = "200` constant",
    .stages = {
      { .stageName = "preheat", .targetTemperature = 200, .timeout = 90, .tempChange5sMin = 10, .tempChange5sMax = 20 },
      { .stageName = "keep", .targetTemperature = 200, .timeout = 0, .tempChange5sMax = 0 },
    }
  }
};

static PlateProgram * currentProgram;
static uint8_t currentStageIdx = 0;

static uint32_t currentStageTs  = 0;
static uint32_t lastMeasurementTs;
static uint32_t lastScreenRefreshTs = 0;
static uint8_t lastMeasurementTemp;
static int8_t currentPWMDuty;

static inline void setPWM(int8_t duty) {
   currentPWMDuty = (int8_t)(duty > 100 ? 100 : (duty < 0 ? 0 : duty));
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (uint8_t)currentPWMDuty);
}

static void uint8toStr(char * buf, uint8_t val) {
  buf[0] = (val / 100) + '0';
  buf[1] = ((val % 100) / 10) + '0';
  buf[2] = (val % 10) + '0';
  buf[3] = 0;
}

static void displayInfo(char * stageName, uint8_t currentTemp, uint8_t elapsedSecondsInStage) {
  char buf[4];

  display_draw("stg:            ", 0, 0, 0);
  display_draw(stageName, 5, 0, 0);

  // temp
  uint8toStr(buf, currentTemp);
  display_draw(buf, 0, 1, 0);
  display_draw("` ", 3, 1, 0);

  // seconds
  uint8toStr(buf, elapsedSecondsInStage);
  display_draw(buf, 5, 1, 0);
  display_draw("s ", 8, 1, 0);

  // pwm
  uint8toStr(buf, (uint8_t)currentPWMDuty);
  display_draw(buf, 10, 1, 0);
  display_draw("%", 13, 1, 1);
}

PlateProgramTickAction plateProgramTick() {
  if (!currentProgram) {
    return PlateProgramTickAction_Finished;
  }

  uint8_t currentTemp = getPlateTemperature();
  uint32_t currentTs = HAL_GetTick();
  uint8_t elapsedSecondsInStage = (currentTs - currentStageTs) / 1000;
  PlateProgramStage * currentStage = &currentProgram->stages[currentStageIdx];

  if (currentTs - lastScreenRefreshTs > SCREEN_REFRESH_PERIOD_IN_MS) {
    displayInfo(currentStage->stageName, currentTemp, elapsedSecondsInStage);
    lastScreenRefreshTs = currentTs;
  }

  if (currentTs - lastMeasurementTs < 5000) {
    return PlateProgramTickAction_Continue;
  }

  uint8_t tempIncreasing = currentTemp > lastMeasurementTemp;
  uint8_t currentRate = tempIncreasing ? currentTemp - lastMeasurementTemp : lastMeasurementTemp - currentTemp;
  lastMeasurementTs = currentTs;
  lastMeasurementTemp = currentTemp;

  // check if we reached target temperature
  if (
      ((currentStage->tempChange5sMax > 0) && currentTemp >= currentStage->targetTemperature)
      || ((currentStage->tempChange5sMax < 0) && currentTemp <= currentStage->targetTemperature)
      || ((currentStage->tempChange5sMax == 0) && (currentStage->timeout != 0) && (elapsedSecondsInStage >= currentStage->timeout))
  ) {
    ++currentStageIdx;
    PlateProgramStage * nextStage = &currentProgram->stages[currentStageIdx];
    if (currentStageIdx >= MAX_PROGRAM_STAGES || nextStage->stageName == 0) {
      shutdownPlate();
      return PlateProgramTickAction_Finished;
    }

    currentStageTs = currentTs;
    setPWM(nextStage->tempChange5sMin == TEMPERATURE_CHANGE_MAX_RATE ? 100 : INITIAL_PWM_DUTY);
    return PlateProgramTickAction_Continue;
  }

  // 0 max rate means that we want to keep temperature close to target temp
  if (currentStage->tempChange5sMax == 0) {
    if (currentStage->targetTemperature > currentTemp) {
      setPWM((int8_t) (currentPWMDuty + PWM_DUTY_STEP_IN_PERCENTS));
    } else if (currentStage->targetTemperature < currentTemp) {
      setPWM((int8_t) (currentPWMDuty - PWM_DUTY_STEP_IN_PERCENTS));
    }

    return PlateProgramTickAction_Continue;
  }

  // check timeout
  if (elapsedSecondsInStage > currentStage->timeout) {
    char buf[4];
    display_draw("stg: ", 0, 0, 0);
    display_draw(currentStage->stageName, 5, 0, 0);
    display_draw("Timeout(", 0, 1, 0);
    uint8toStr(buf, currentStage->timeout);
    display_draw(buf, 8, 1, 0);
    display_draw("s)", 11, 1, 1);
    return PlateProgramTickAction_Error;
  }

  // check temperature rate
  if (tempIncreasing) {
    if (currentStage->tempChange5sMax < 0) {
      // temp increasing, but we want it to be decreasing => turn plate off
      setPWM(0);
      return PlateProgramTickAction_Continue;
    }

    if (currentRate < currentStage->tempChange5sMin) {
      // increasing too slow?
      setPWM((int8_t) (currentPWMDuty + PWM_DUTY_STEP_IN_PERCENTS));
    } else if (currentRate > currentStage->tempChange5sMax) {
      // too fast?
      setPWM((int8_t) (currentPWMDuty - PWM_DUTY_STEP_IN_PERCENTS));
    }
  } else {
    if (currentStage->tempChange5sMax > 0) {
      // temp decreasing, but we want it to be increasing => turn plate full power
      setPWM(100);
      return PlateProgramTickAction_Continue;
    }

    if (currentRate < -currentStage->tempChange5sMin) {
      // decreasing too slow?
      setPWM((int8_t) (currentPWMDuty - PWM_DUTY_STEP_IN_PERCENTS));
    } else if (currentRate > -currentStage->tempChange5sMax) {
      // too fast?
      setPWM((int8_t) (currentPWMDuty + PWM_DUTY_STEP_IN_PERCENTS));
    }
  }

  return PlateProgramTickAction_Continue;
}

void startProgram(PlateProgram * program) {
  currentProgram = program;
  currentStageIdx = 0;

  lastMeasurementTs = HAL_GetTick();
  currentStageTs = lastMeasurementTs;
  lastMeasurementTemp = getPlateTemperature();
  setPWM(INITIAL_PWM_DUTY);
}

void shutdownPlate() {
  setPWM(0);
  currentProgram = 0;
}