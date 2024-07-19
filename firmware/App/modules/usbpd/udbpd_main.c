#include "usbpd.h"
#include "usbpd_phy.h"
#include "usbpd_msg.h"

/*
 * Expected flow:
 *   Source: Source Capabilities [!]
 *   Sink:   GoodCRC             [!]
 *   Sink:   Request
 *   Source: GoodCRC
 *   Source: Accept
 *   Sink:   GoodCRC
 *   Source: PS_RDY              [!]
 *   Sink:   GoodCRC
 */

static enum {
  State_WaitingCapabilities,
  State_CapabilitiesMatched,
  State_RequestSent,
  State_Ready,
} state = State_WaitingCapabilities;

static uint16_t lastSpecRev;
static uint16_t desiredVoltageInMilliVolts;
static uint16_t desiredCurrentInMilliAmps;
static uint8_t chosenPowerDataObjectIdx;

// we can't use stack, because in interrupt we keep pointer to TX buffer
static USBPDMessage txMsg;

static void choosePowerDataObject(uint32_t const objects[], uint8_t count) {
  for (uint8_t idx = 0; idx < count; idx++) {
    uint32_t pdo = objects[idx];

    // XXX: support only fixed voltage
    uint8_t type = pdo >> 30;
    uint16_t maxCurrent = (pdo & 0x3ff) * 10;
    uint16_t voltage = ((pdo >> 10) & 0x03ff) * 50;
    if (type == USBPDMessage_PDO_Fixed && voltage == desiredVoltageInMilliVolts && maxCurrent >= desiredCurrentInMilliAmps) {
      chosenPowerDataObjectIdx = idx + 1;
      state = State_CapabilitiesMatched;
      return;
    }
  }
}

static void onMessage(uint8_t * msg) {
  USBPDMessage * rxMsg = (USBPDMessage *)msg;

  // better to use same value as we got from source
  lastSpecRev = USBPDMessage_specRev(rxMsg);
  uint8_t commandCode = USBPDMessage_commandCode(rxMsg);
  uint8_t numObjects = USBPDMessage_numObjects(rxMsg);

  if (numObjects == 0) {
    // need to send GoodCRC response for any other message except GoodCRC from source
    if (commandCode == USBPDMessage_GoodCrc) {
      return;
    }

    if (commandCode == USBPDMessage_PSReady) {
      state = State_Ready;
    }
  } else if (commandCode == USBPDMessage_SourceCapabilities) {
    choosePowerDataObject(rxMsg->objects, numObjects);
  }

  // send GoodCRC
  USBPDMessage_buildGoodCRCMsg(&txMsg, lastSpecRev, USBPDMessage_msgId(rxMsg));
  pd_transmitMessage((uint8_t *)&txMsg, USBPD_MESSAGE_HEADER_SIZE);
}

static void sendRequestMsg() {
  uint32_t current10 = (desiredCurrentInMilliAmps + 5) / 10;
  if (current10 > 0x3FF) {
    current10 = 0x3FF;
  }

  uint32_t requestPowerObject = ((uint32_t)chosenPowerDataObjectIdx << 28) | 0x1000000 | (current10 << 10) | current10;
  USBPDMessage_buildRequestMsg(&txMsg, lastSpecRev, 1, requestPowerObject);
  pd_transmitMessage((uint8_t *)&txMsg, USBPD_MESSAGE_HEADER_SIZE + sizeof(requestPowerObject));
  state = State_RequestSent;
}

static void onSent() {
  // after GoodCRC sent by default, we can send another message
  if (state == State_CapabilitiesMatched) {
    sendRequestMsg();
  }
}

static void onInit() {
  USBPDMessage_buildSoftResetMsg(&txMsg, 0x3, 0x0);
  pd_transmitMessage((uint8_t *)&txMsg, USBPD_MESSAGE_HEADER_SIZE);
}

void pd_startSinking(uint16_t voltageInMilliVolts, uint16_t currentInMilliAmps) {
  desiredVoltageInMilliVolts = voltageInMilliVolts;
  desiredCurrentInMilliAmps = currentInMilliAmps;

  pd_initPhyLayer(onMessage, onSent, onInit);
}

uint8_t pd_isReady() {
  return state == State_Ready ? 1 : 0;
}