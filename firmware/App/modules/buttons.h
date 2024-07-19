#pragma once

typedef enum {
  BtnUp = 0,
  BtnDown,
  BtnStartStop,
  BtnNone,

  BtnFirst = BtnUp,
  BtnLast = BtnStartStop,
} Button;

extern Button getUnprocessedButtonPush();