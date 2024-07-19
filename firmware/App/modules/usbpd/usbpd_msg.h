#pragma once

#include <stdint.h>

// always 2 bytes (only header)
#define USBPD_MESSAGE_HEADER_SIZE 2

// we envelop only single object into our outgoing message
#define USBPD_MESSAGE_MAX_OBJECTS_TO_BE_SENT  1

typedef enum {
  USBPDMessage_SourceCapabilities = 0x01,
  USBPDMessage_Request = 0x02,
} USBPDMessage_DataCommandCode;

typedef enum {
  USBPDMessage_GoodCrc = 0x01,
  USBPDMessage_PSReady = 0x06,
  USBPDMessage_SoftReset = 0x0D,
} USBPDMessage_ControlCommandCode;

typedef enum {
  USBPDMessage_PDO_Fixed = 0x00,
} USBPDMessage_PDO_Type;

/*
 * we can't use bitfields, because they are implementation-specific :(
 *
 * uint16_t reserved0: 1;
 * uint16_t numObjects: 3;
 * uint16_t messageId: 3;
 * uint16_t portPowerRole: 1;
 * uint16_t specRev: 2;
 * uint16_t portDataRole: 1;
 * uint16_t reserved1: 1;
 * uint16_t commandCode: 4;
 */

typedef struct {
  uint16_t header;

  // count of elements is specified in numObjects field
  uint32_t objects[USBPD_MESSAGE_MAX_OBJECTS_TO_BE_SENT];
} __attribute__((packed, aligned(2))) USBPDMessage;

static inline void USBPDMessage_buildGoodCRCMsg(USBPDMessage * msg, uint8_t specRev, uint8_t msgId) {
  msg->header = ((uint16_t)msgId << 9) | (specRev << 6) | USBPDMessage_GoodCrc;
}

static inline void USBPDMessage_buildSoftResetMsg(USBPDMessage * msg, uint8_t specRev, uint8_t msgId) {
  msg->header = ((uint16_t)msgId << 9) | (specRev << 6) | USBPDMessage_SoftReset;
}

static inline void USBPDMessage_buildRequestMsg(USBPDMessage * msg, uint8_t specRev, uint8_t msgId, uint32_t requestDataObject) {
  msg->header = 0x1000 | ((uint16_t)msgId << 9) | (specRev << 6) | USBPDMessage_Request;
  msg->objects[0] = requestDataObject;
}

static inline uint8_t USBPDMessage_commandCode(USBPDMessage * msg) {
  return msg->header & 0x1F;
}

static inline uint8_t USBPDMessage_specRev(USBPDMessage * msg) {
  return (msg->header >> 6) & 0x3;
}

static inline uint8_t USBPDMessage_msgId(USBPDMessage * msg) {
  return (msg->header >> 9) & 0x7;
}

static inline uint8_t USBPDMessage_numObjects(USBPDMessage * msg) {
  return (msg->header >> 12) & 0x7;
}
