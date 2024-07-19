#pragma once

#include <stdint.h>

void display_init();
void display_redraw(char * firstLine, char * secondLine);
void display_clear(uint8_t updateScreen);
void display_draw(char * str, uint8_t x, uint8_t y, uint8_t updateScreen);