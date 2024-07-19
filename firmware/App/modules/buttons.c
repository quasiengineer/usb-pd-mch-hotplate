#include "main.h"
#include "buttons.h"

#define DEBOUNCE_THRESHOLD_IN_MS 100

typedef struct {
  uint32_t lastPushedTs;
  GPIO_PinState pinState;
} ButtonState;

volatile static ButtonState btnStates[BtnLast + 1] = { 0 };

static uint8_t shouldProcessButtonPush(Button btn) {
  uint16_t pin;

  switch (btn) {
    case BtnUp:
      pin = BTN_UP_Pin;
      break;

    case BtnDown:
      pin = BTN_DOWN_Pin;
      break;

    case BtnStartStop:
      pin = BTN_START_STOP_Pin;
      break;
  }

  volatile ButtonState * btnState = &btnStates[btn];
  GPIO_PinState pinState = HAL_GPIO_ReadPin(BTN_UP_GPIO_Port, pin);
  uint32_t ts = HAL_GetTick();

  if (pinState == GPIO_PIN_RESET) {
    btnState->pinState = pinState;
    return 0;
  }

  if (btnState->pinState == GPIO_PIN_SET) {
    btnState->lastPushedTs = ts;
    return 0;
  }

  // transition from RESET to SET means that button has been pushed
  btnState->pinState = GPIO_PIN_SET;
  // check if button has been in reset state too short
  if (ts - btnState->lastPushedTs < DEBOUNCE_THRESHOLD_IN_MS) {
    btnState->lastPushedTs = ts;
    return 0;
  }

  return 1;
}

Button getUnprocessedButtonPush() {
  for (Button btn = BtnFirst; btn <= BtnLast; ++btn) {
    if (shouldProcessButtonPush(btn) == 1) {
      return btn;
    }
  }

  return BtnNone;
}
