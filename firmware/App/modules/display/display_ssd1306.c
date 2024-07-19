#include "display_ssd1306.h"

// Screenbuffer
//
// Each byte defines 8-bit column of pixels
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

//
//  Send a byte to the command register
//
static uint8_t ssd1306_WriteCommand(I2C_HandleTypeDef *hi2c, uint8_t command)
{
  return HAL_I2C_Mem_Write(hi2c, SSD1306_I2C_ADDR, 0x00, 1, &command, 1, 10);
}

//
//  Initialize the oled screen
//
uint8_t ssd1306_Init(I2C_HandleTypeDef *hi2c)
{
  // Wait for the screen to boot
  HAL_Delay(100);
  int status = 0;

  // Init LCD
  status += ssd1306_WriteCommand(hi2c, 0xAE);   // Display off
  status += ssd1306_WriteCommand(hi2c, 0x20);   // Set Memory Addressing Mode
  status += ssd1306_WriteCommand(hi2c, 0x10);   // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  status += ssd1306_WriteCommand(hi2c, 0xB0);   // Set Page Start Address for Page Addressing Mode,0-7
  status += ssd1306_WriteCommand(hi2c, 0xC8);   // Set COM Output Scan Direction
  status += ssd1306_WriteCommand(hi2c, 0x00);   // Set low column address
  status += ssd1306_WriteCommand(hi2c, 0x10);   // Set high column address
  status += ssd1306_WriteCommand(hi2c, 0x40);   // Set start line address
  status += ssd1306_WriteCommand(hi2c, 0x81);   // set contrast control register
  status += ssd1306_WriteCommand(hi2c, 0xFF);
  status += ssd1306_WriteCommand(hi2c, 0xA1);   // Set segment re-map 0 to 127
  status += ssd1306_WriteCommand(hi2c, 0xA6);   // Set normal display

  status += ssd1306_WriteCommand(hi2c, 0xA8);   // Set multiplex ratio(1 to 64)
  status += ssd1306_WriteCommand(hi2c, SSD1306_HEIGHT - 1);

  status += ssd1306_WriteCommand(hi2c, 0xA4);   // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  status += ssd1306_WriteCommand(hi2c, 0xD3);   // Set display offset
  status += ssd1306_WriteCommand(hi2c, 0x00);   // No offset
  status += ssd1306_WriteCommand(hi2c, 0xD5);   // Set display clock divide ratio/oscillator frequency
  status += ssd1306_WriteCommand(hi2c, 0xF0);   // Set divide ratio
  status += ssd1306_WriteCommand(hi2c, 0xD9);   // Set pre-charge period
  status += ssd1306_WriteCommand(hi2c, 0x22);

  status += ssd1306_WriteCommand(hi2c, 0xDA);   // Set com pins hardware configuration
  status += ssd1306_WriteCommand(hi2c, SSD1306_COM_LR_REMAP << 5 | SSD1306_COM_ALTERNATIVE_PIN_CONFIG << 4 | 0x02);

  status += ssd1306_WriteCommand(hi2c, 0xDB);   // Set vcomh
  status += ssd1306_WriteCommand(hi2c, 0x20);   // 0x20,0.77xVcc
  status += ssd1306_WriteCommand(hi2c, 0x8D);   // Set DC-DC enable
  status += ssd1306_WriteCommand(hi2c, 0x14);   //
  status += ssd1306_WriteCommand(hi2c, 0xAF);   // Turn on SSD1306 panel

  if (status != 0) {
    return 1;
  }

  // Clear screen
  ssd1306_Clear();

  // Flush buffer to screen
  ssd1306_UpdateScreen(hi2c);

  return 0;
}

void ssd1306_Clear()
{
  // Fill screenbuffer with a constant value (color)
  uint32_t i;

  for(i = 0; i < sizeof(SSD1306_Buffer); i++)
  {
    SSD1306_Buffer[i] = 0x00;
  }
}

//
//  Write the screenbuffer with changed to the screen
//
void ssd1306_UpdateScreen(I2C_HandleTypeDef *hi2c)
{
  uint8_t i;

  for (i = 0; i < 8; i++) {
    ssd1306_WriteCommand(hi2c, 0xB0 + i);
    ssd1306_WriteCommand(hi2c, 0x00);
    ssd1306_WriteCommand(hi2c, 0x10);

    HAL_I2C_Mem_Write(hi2c, SSD1306_I2C_ADDR, 0x40, 1, &SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH, 100);
  }
}

//
//  Draw one pixel in the screenbuffer
//  X => X Coordinate
//  Y => Y Coordinate
void ssd1306_SetPixel(uint8_t x, uint8_t y, uint8_t display)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    // Don't write outside the buffer
    return;
  }

  if (display)
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  }
  else
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}

//  Draw 1 char to the screen buffer
//  ch      => Character to write
//  x       => X position in pixels
//  y       => Y position in pixels
void ssd1306_WriteChar(char ch, uint8_t x, uint8_t y)
{
  // Translate font to screenbuffer
  for (uint8_t i = 0; i < CHARACTER_HEIGHT; i++)
  {
    uint16_t b = Font8x16[(ch - 32) * CHARACTER_HEIGHT + i];
    for (uint8_t j = 0; j < CHARACTER_WIDTH; j++)
    {
      ssd1306_SetPixel(x + j, y + i, ((b << j) & 0x8000) == 0 ? 0 : 1);
    }
  }
}

void ssd1306_PutString(const char * str, uint8_t charPosX, uint8_t charPosY, uint8_t fillEndWithSpaces) {
  uint8_t yOffset = charPosY * CHARACTER_HEIGHT;
  uint8_t xOffset = charPosX * CHARACTER_WIDTH;

  for (; *str; ++str, xOffset += CHARACTER_WIDTH) {
    ssd1306_WriteChar(*str, xOffset, yOffset);
  }

  if (fillEndWithSpaces) {
    uint8_t byteRowsPerLine = SSD1306_HEIGHT / CHARACTER_HEIGHT;
    for (uint8_t y = 0; y < byteRowsPerLine; ++y) {
      uint16_t yRowOffset = (charPosY * byteRowsPerLine + y) * SSD1306_WIDTH;
      for (uint8_t x = xOffset; x < SSD1306_WIDTH; ++x) {
        SSD1306_Buffer[x + yRowOffset] = 0x00;
      }
    }
  }
}
