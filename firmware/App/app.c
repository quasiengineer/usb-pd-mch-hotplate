#include "main.h"
#include "app.h"
#include "usbpd.h"
#include "display.h"
#include "buttons.h"
#include "plate.h"

#define USBPD_REQUIRED_VOLTAGE_IN_MV 20000
#define USBPD_REQUIRED_CURRENT_IN_MA 5000

// 2s
#define USBPD_INIT_TIMEOUT_IN_MS (2 * 1000)

#define LED_FLICKERING_PERIOD_IN_MS 1000

typedef enum {
  App_Started = 0,
  App_Ready,
  App_Running,
  App_Failed,
} AppState;

static uint32_t startTs;
static uint32_t ledToggledTs = 0;

static AppState state;
static uint8_t currentMenuProgram = 0;
static uint8_t lastMenuBtnClicked = BtnUp;

static void displayMenu() {
  display_clear(0);

  if (lastMenuBtnClicked == BtnUp) {
    display_draw(programs[currentMenuProgram].programName, 2, 0, 0);
    display_draw(programs[(currentMenuProgram + 1) % PROGRAM_COUNT].programName, 2, 1, 0);
    display_draw("> ", 0, 0, 1);
  } else {
    display_draw(programs[(currentMenuProgram + PROGRAM_COUNT - 1) % PROGRAM_COUNT].programName, 2, 0, 0);
    display_draw(programs[currentMenuProgram].programName, 2, 1, 0);
    display_draw("> ", 0, 1, 1);
  }
}

static void setHotplateAsReady() {
  state = App_Ready;
  HAL_GPIO_WritePin(LED_START_STOP_GPIO_Port, LED_START_STOP_Pin, GPIO_PIN_RESET);
  currentMenuProgram = 0;
  lastMenuBtnClicked = BtnUp;
  displayMenu();
}

void handleButtonClicks() {
  Button btn = getUnprocessedButtonPush();
  if (btn == BtnNone) {
    return;
  }

  if (state == App_Ready) {
    switch (btn) {
      case BtnUp:
        currentMenuProgram = (currentMenuProgram + PROGRAM_COUNT - 1) % PROGRAM_COUNT;
        lastMenuBtnClicked = BtnUp;
        displayMenu();
        return;

      case BtnDown:
        currentMenuProgram = (currentMenuProgram + 1) % PROGRAM_COUNT;
        lastMenuBtnClicked = BtnDown;
        displayMenu();
        return;

      case BtnStartStop:
        HAL_GPIO_WritePin(LED_START_STOP_GPIO_Port, LED_START_STOP_Pin, GPIO_PIN_SET);
        display_clear(1);
        startProgram(&programs[currentMenuProgram]);
        state = App_Running;
        return;
    }
  }

  if (state == App_Running && btn == BtnStartStop) {
    shutdownPlate();
    setHotplateAsReady();
  }
}

/*
 * Infinite loop
 */
void infiniteLoopIteration() {
  // XXX: i am not going run that sw for 40+ days, so we don't overflow
  uint32_t currentTs = HAL_GetTick();

  if (state == App_Started) {
    if (pd_isReady()) {
      setHotplateAsReady();
    } else if (currentTs - startTs > USBPD_INIT_TIMEOUT_IN_MS) {
      state = App_Failed;
      display_redraw("Error occurred!", "Power issue");
    }
  }

  if (state == App_Running) {
    switch (plateProgramTick()) {
      case PlateProgramTickAction_Error:
        state = App_Failed;
        break;

      case PlateProgramTickAction_Finished:
        setHotplateAsReady();
        break;

      case PlateProgramTickAction_Continue:
        break;
    }
  }

  handleButtonClicks();

  if (currentTs - ledToggledTs > LED_FLICKERING_PERIOD_IN_MS) {
    HAL_GPIO_TogglePin(LED_OUT_GPIO_Port, LED_OUT_Pin);
    ledToggledTs = currentTs;
  }
}

void initApp() {
  display_init();

  state = App_Started;
  startTs = HAL_GetTick();

  display_redraw("Hotplate started", "  Wait for power");
  pd_startSinking(USBPD_REQUIRED_VOLTAGE_IN_MV, USBPD_REQUIRED_CURRENT_IN_MA);

  // plate control
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  shutdownPlate();
}