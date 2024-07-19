#include "display.h"
#include "display_ssd1306.h"

void display_init() {
  ssd1306_Init(&hi2c3);
}

void display_redraw(char * firstLine, char * secondLine) {
  ssd1306_PutString(firstLine, 0, 0, 1);
  ssd1306_PutString(secondLine, 0, 1, 1);
  ssd1306_UpdateScreen(&hi2c3);
}

void display_draw(char * str, uint8_t x, uint8_t y, uint8_t updateScreen) {
  ssd1306_PutString(str, x, y, 0);
  if (updateScreen) {
    ssd1306_UpdateScreen(&hi2c3);
  }
}

void display_clear(uint8_t updateScreen) {
  ssd1306_Clear();

  if (updateScreen) {
    ssd1306_UpdateScreen(&hi2c3);
  }
}