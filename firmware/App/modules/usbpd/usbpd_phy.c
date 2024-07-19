#include "usbpd_phy.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_ucpd.h"
#include "stm32g4xx_ll_pwr.h"

static uint8_t rxBuf[30];
static uint8_t rxBufPos = 0;

static uint8_t * txBuf;
static uint8_t txBufPos = 0;

static pd_callbackRecv onMessage;
static pd_callbackSent onSent;
static pd_callbackInit onInit;

inline static void initClocking() {
  // enable clocking, power for UCPD would be enabled via LL_UCPD_Init() call
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
}

inline static void initGPIO() {
  // CC1
  LL_GPIO_InitTypeDef gpioCCInit = {
      .Pin = LL_GPIO_PIN_6,
      .Mode = LL_GPIO_MODE_ANALOG,
      .Speed = LL_GPIO_SPEED_FREQ_LOW,
      .OutputType = LL_GPIO_OUTPUT_OPENDRAIN,
      .Pull = LL_GPIO_PULL_NO,
      .Alternate = LL_GPIO_AF_0,
  };
  LL_GPIO_Init(GPIOB, &gpioCCInit);

  // CC2
  gpioCCInit.Pin = LL_GPIO_PIN_4;
  LL_GPIO_Init(GPIOB, &gpioCCInit);
}

inline static void initUCPD() {
  LL_UCPD_InitTypeDef ucpdInit = {};
  LL_UCPD_StructInit(&ucpdInit);
  LL_UCPD_Init(UCPD1, &ucpdInit);

  // we only need SOP packets
  LL_UCPD_SetRxOrderSet(UCPD1, LL_UCPD_ORDERSET_SOP);

  // we can get rid of CC dead battery pull-ups, UCPD should control voltage now
  LL_PWR_DisableUCPDDeadBattery();

  // initial events to determine what polarity is used and to continue configuration
  LL_UCPD_EnableIT_TypeCEventCC1(UCPD1);
  LL_UCPD_EnableIT_TypeCEventCC2(UCPD1);

  // should be enabled before accessing some registers (CR and IMR in particular)
  LL_UCPD_Enable(UCPD1);

  // Sink role
  LL_UCPD_SetSNKRole(UCPD1);
  LL_UCPD_SetRpResistor(UCPD1, LL_UCPD_RESISTOR_NONE);
  LL_UCPD_SetccEnable(UCPD1, LL_UCPD_CCENABLE_CC1CC2);

  LL_UCPD_SetRxMode(UCPD1, LL_UCPD_RXMODE_NORMAL);
  LL_UCPD_RxEnable(UCPD1);

  // interrupt
  NVIC_SetPriority(UCPD1_IRQn, NVIC_GetPriority(TIM7_IRQn));
  NVIC_EnableIRQ(UCPD1_IRQn);
}

void pd_initPhyLayer(pd_callbackRecv onMessageCallback, pd_callbackSent onSentCallback, pd_callbackInit onInitCallback) {
  onMessage = onMessageCallback;
  onSent = onSentCallback;
  onInit = onInitCallback;

  initClocking();
  initGPIO();
  initUCPD();
}

static void handleCCVoltageChange() {
  uint8_t activeCCLine = 0;
  if (LL_UCPD_GetTypeCVstateCC1(UCPD1) != 0) {
    activeCCLine = 1;
  } else if (LL_UCPD_GetTypeCVstateCC2(UCPD1) != 0) {
    activeCCLine = 2;
  }

  if (!activeCCLine) {
    return;
  }

  // we don't handle other changes for CC lines
  LL_UCPD_DisableIT_TypeCEventCC1(UCPD1);
  LL_UCPD_DisableIT_TypeCEventCC1(UCPD1);

  LL_UCPD_SetCCPin(UCPD1, activeCCLine == 1 ? LL_UCPD_CCPIN_CC1 : LL_UCPD_CCPIN_CC2);

  LL_UCPD_EnableIT_RxMsgEnd(UCPD1);
  LL_UCPD_EnableIT_RxNE(UCPD1);
  LL_UCPD_EnableIT_TxMSGSENT(UCPD1);
  LL_UCPD_EnableIT_TxMSGDISC(UCPD1);
  LL_UCPD_EnableIT_TxMSGABT(UCPD1);
  LL_UCPD_EnableIT_TxIS(UCPD1);

  onInit();
}

// re-declare interrupt handler
extern void UCPD1_IRQHandler() {
  uint32_t status = UCPD1->SR;

  if (status & UCPD_SR_RXNE) {
    rxBuf[rxBufPos] = LL_UCPD_ReadData(UCPD1);
    rxBufPos++;
  }

  if (status & UCPD_SR_TXIS) {
    LL_UCPD_WriteData(UCPD1, txBuf[txBufPos]);
    txBufPos++;
  }

  if ((status & (UCPD_SR_TYPECEVT1 | UCPD_SR_TYPECEVT2)) != 0) {
    LL_UCPD_ClearFlag_TypeCEventCC1(UCPD1);
    LL_UCPD_ClearFlag_TypeCEventCC2(UCPD1);
    handleCCVoltageChange();
  }

  // incoming message
  if ((status & UCPD_SR_RXMSGEND) != 0) {
    LL_UCPD_ClearFlag_RxMsgEnd(UCPD1);
    if ((status & UCPD_SR_RXERR) == 0) {
      onMessage(rxBuf);
    }
    rxBufPos = 0;
  }

  if ((status & UCPD_SR_TXMSGABT) != 0)
  {
    LL_UCPD_ClearFlag_TxMSGABT(UCPD1);
  }

  if ((status & UCPD_SR_TXMSGDISC) != 0)
  {
    LL_UCPD_ClearFlag_TxMSGDISC(UCPD1);
  }

  // message sent
  if ((status & UCPD_SR_TXMSGSENT) != 0) {
    LL_UCPD_ClearFlag_TxMSGSENT(UCPD1);
    onSent();
  }
}

void pd_transmitMessage(uint8_t * msg, uint8_t sz) {
  txBuf = msg;
  txBufPos = 0;

  // start transmitting
  LL_UCPD_WriteTxOrderSet(UCPD1, LL_UCPD_ORDERED_SET_SOP);
  LL_UCPD_WriteTxPaySize(UCPD1, sz);
  LL_UCPD_SendMessage(UCPD1);
}