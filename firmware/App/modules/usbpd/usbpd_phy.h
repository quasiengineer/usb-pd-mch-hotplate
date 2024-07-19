#pragma once

#include <stdint.h>

typedef void (* pd_callbackRecv)(uint8_t * msg);
typedef void (* pd_callbackSent)();
typedef void (* pd_callbackInit)();

void pd_initPhyLayer(pd_callbackRecv onMessageCallback, pd_callbackSent onSentCallback, pd_callbackInit onInitCallback);
void pd_transmitMessage(uint8_t * msg, uint8_t sz);