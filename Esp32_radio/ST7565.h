#include <Arduino.h>
#include <string.h>
#include <SPI.h>
/*
ST7565 module for Esp32_radio
For use with cheap COG ST7565 LCD module serial data mode (most modules have the option of choosing the par / ser data mode)

It's just simplified library (no validation for pins, pages, etc) in modified SSD1306.h file

  ST7565     ESP32
1-CS1             tft_cs  15
2-RESet
3-A0              tft_dc  2
12-DB6(SCLK)      SPI_SCK 18
13-DB7(SID)       SPI_MOSI  23
---Tested with VTM88896A module
14-VDD (3V)
15-GND
16-LED+ (5V 45mA)
17-LED-
*/

// Color definitions for the TFT screen (if used)
// OLED only have 2 colors.
#define BLACK 0
#define BLUE 0xFF
#define RED 0xFF
#define GREEN 0
#define CYAN GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW RED | GREEN
#define WHITE RED | GREEN | BLUE

#define ST7565_NPAG 8 // Number of pages (text lines)

#define ST7565_CONTRAST 35 //default contrast for LCD -
#define ST7565_INVERSION 0 //1-negative

// Data to display.  There are TFTSECS sections

struct page_struct
{
  uint8_t page[128]; // Buffer for one page (8 pixels heigh)
};

class ST7565SPI
{
public:
  ST7565SPI(uint8_t pinA0, uint8_t pinCS); // Constructor pin a0=dc
  void clear();                            // Clear buffer
  void display();                          // Display buffer
  void print(char c);                      // Print a character
  void print(const char *str);             // Print a string
  void setCursor(uint8_t x, uint8_t y);    // Position the cursor
  void fillRect(uint8_t x, uint8_t y,      // Fill a rectangle
                uint8_t w, uint8_t h,
                uint8_t color);
  void initLCD();

private:
  struct page_struct *stbuf = NULL;
  void sendCommand(uint8_t command);
  void sendData(uint8_t *data, uint8_t len);
  void setCS(bool val);
  void setDC(bool val); // 0-command 1-data
  void setPage(uint8_t add);
  void setCol(uint8_t add);
  void setInitLine(uint8_t add);
  const uint8_t *font;  // Font to use
  uint8_t xchar = 0;    // Current cursor position (text)
  uint8_t ychar = 0;    // Current cursor position (text)
  uint8_t pinstA0 = 0;  //pin DataCommand
  uint8_t pinstCS = 0;  //CSelect
};

ST7565SPI *tft = NULL;

#define TFTSECS 4
scrseg_struct tftdata[TFTSECS] = // Screen divided in 3 segments + 1 overlay
    {
        // One text line is 8 pixels
        {false, WHITE, 0, 8, ""},    // 1 top line
        {false, CYAN, 8, 32, ""},    // 4 lines in the middle
        {false, YELLOW, 40, 22, ""}, // 3 lines at the bottom, leave room for indicator
        {false, GREEN, 40, 22, ""}   // 3 lines at the bottom for rotary encoder
};

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()  // Use standard landscape format -could be implemented
#define dsp_setTextSize(a) // Set the text size
#define dsp_setTextColor(a)
#define dsp_print(a) tft->print(a) // Print a string
#define dsp_println(a) \
  tft->print(a);       \
  tft->print('\n')                                               // Print string plus newline
#define dsp_setCursor(a, b) tft->setCursor(a, b)                 // Position the cursor
#define dsp_fillRect(a, b, c, d, e) tft->fillRect(a, b, c, d, e) // Fill a rectangle
#define dsp_erase() tft->clear()                                 // Clear the screen
#define dsp_getwidth() 128                                       // Get width of screen
#define dsp_getheight() 64                                       // Get height of screen
#define dsp_update() tft->display()                              // Updates to the physical screen
#define dsp_usesSPI() true                                       // Does use SPI

bool dsp_begin()
{
  dbgprint("Init ST7565, controll pins CS %d, A0 %d", ini_block.tft_cs_pin, ini_block.tft_dc_pin);
  if ((ini_block.tft_cs_pin >= 0) &&
      (ini_block.tft_dc_pin >= 0))
  {
    tft = new ST7565SPI(ini_block.tft_dc_pin,
                        ini_block.tft_cs_pin); // Create an instant for TFT
  }
  else
  {
    dbgprint("Init ST7565SPI failed!");
  }
  if (tft)
  {
    tft->initLCD();
    tft->clear();
  }
  return (tft != NULL);
}

//constants, etc for ST7565

const uint8_t SSD1306FONTWIDTH = 5;
const uint8_t SSD1306font[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, // SPACE
        0x00, 0x00, 0x5F, 0x00, 0x00, // !
        0x00, 0x03, 0x00, 0x03, 0x00, // "
        0x14, 0x3E, 0x14, 0x3E, 0x14, // #
        0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
        0x43, 0x33, 0x08, 0x66, 0x61, // %
        0x36, 0x49, 0x55, 0x22, 0x50, // &
        0x00, 0x05, 0x03, 0x00, 0x00, // '
        0x00, 0x1C, 0x22, 0x41, 0x00, // (
        0x00, 0x41, 0x22, 0x1C, 0x00, // )
        0x14, 0x08, 0x3E, 0x08, 0x14, // *
        0x08, 0x08, 0x3E, 0x08, 0x08, // +
        0x00, 0xA0, 0x60, 0x00, 0x00, // ,
        0x08, 0x08, 0x08, 0x08, 0x08, // -
        0x00, 0x60, 0x60, 0x00, 0x00, // .
        0x20, 0x10, 0x08, 0x04, 0x02, // /

        0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
        0x00, 0x04, 0x02, 0x7F, 0x00, // 1
        0x42, 0x61, 0x51, 0x49, 0x46, // 2
        0x22, 0x41, 0x49, 0x49, 0x36, // 3
        0x18, 0x14, 0x12, 0x7F, 0x10, // 4
        0x27, 0x45, 0x45, 0x45, 0x39, // 5
        0x3E, 0x49, 0x49, 0x49, 0x32, // 6
        0x01, 0x01, 0x71, 0x09, 0x07, // 7
        0x36, 0x49, 0x49, 0x49, 0x36, // 8
        0x26, 0x49, 0x49, 0x49, 0x3E, // 9
        0x00, 0x36, 0x36, 0x00, 0x00, // :
        0x00, 0x56, 0x36, 0x00, 0x00, // ;
        0x08, 0x14, 0x22, 0x41, 0x00, // <
        0x14, 0x14, 0x14, 0x14, 0x14, // =
        0x00, 0x41, 0x22, 0x14, 0x08, // >
        0x02, 0x01, 0x51, 0x09, 0x06, // ?

        0x3E, 0x41, 0x59, 0x55, 0x5E, // @
        0x7E, 0x09, 0x09, 0x09, 0x7E, // A
        0x7F, 0x49, 0x49, 0x49, 0x36, // B
        0x3E, 0x41, 0x41, 0x41, 0x22, // C
        0x7F, 0x41, 0x41, 0x41, 0x3E, // D
        0x7F, 0x49, 0x49, 0x49, 0x41, // E
        0x7F, 0x09, 0x09, 0x09, 0x01, // F
        0x3E, 0x41, 0x41, 0x49, 0x3A, // G
        0x7F, 0x08, 0x08, 0x08, 0x7F, // H
        0x00, 0x41, 0x7F, 0x41, 0x00, // I
        0x30, 0x40, 0x40, 0x40, 0x3F, // J
        0x7F, 0x08, 0x14, 0x22, 0x41, // K
        0x7F, 0x40, 0x40, 0x40, 0x40, // L
        0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
        0x7F, 0x02, 0x04, 0x08, 0x7F, // N
        0x3E, 0x41, 0x41, 0x41, 0x3E, // O

        0x7F, 0x09, 0x09, 0x09, 0x06, // P
        0x1E, 0x21, 0x21, 0x21, 0x5E, // Q
        0x7F, 0x09, 0x09, 0x09, 0x76, // R
        0x26, 0x49, 0x49, 0x49, 0x32, // S
        0x01, 0x01, 0x7F, 0x01, 0x01, // T
        0x3F, 0x40, 0x40, 0x40, 0x3F, // U
        0x1F, 0x20, 0x40, 0x20, 0x1F, // V
        0x7F, 0x20, 0x10, 0x20, 0x7F, // W
        0x41, 0x22, 0x1C, 0x22, 0x41, // X
        0x07, 0x08, 0x70, 0x08, 0x07, // Y
        0x61, 0x51, 0x49, 0x45, 0x43, // Z
        0x00, 0x7F, 0x41, 0x00, 0x00, // [
        0x02, 0x04, 0x08, 0x10, 0x20, // backslash
        0x00, 0x00, 0x41, 0x7F, 0x00, // ]
        0x04, 0x02, 0x01, 0x02, 0x04, // ^
        0x40, 0x40, 0x40, 0x40, 0x40, // _

        0x00, 0x01, 0x02, 0x04, 0x00, // `
        0x20, 0x54, 0x54, 0x54, 0x78, // a
        0x7F, 0x44, 0x44, 0x44, 0x38, // b
        0x38, 0x44, 0x44, 0x44, 0x44, // c
        0x38, 0x44, 0x44, 0x44, 0x7F, // d
        0x38, 0x54, 0x54, 0x54, 0x18, // e
        0x04, 0x04, 0x7E, 0x05, 0x05, // f
        0x18, 0xA4, 0xA4, 0xA4, 0x78, // g
        0x7F, 0x08, 0x04, 0x04, 0x78, // h
        0x00, 0x44, 0x7D, 0x40, 0x00, // i
        0x20, 0x40, 0x44, 0x3D, 0x00, // j
        0x7F, 0x10, 0x28, 0x44, 0x00, // k
        0x00, 0x41, 0x7F, 0x40, 0x00, // l
        0x7C, 0x04, 0x78, 0x04, 0x78, // m
        0x7C, 0x08, 0x04, 0x04, 0x78, // n
        0x38, 0x44, 0x44, 0x44, 0x38, // o

        0xFC, 0x24, 0x24, 0x24, 0x18, // p
        0x18, 0x24, 0x24, 0x24, 0xFC, // q
        0x00, 0x7C, 0x08, 0x04, 0x04, // r
        0x48, 0x54, 0x54, 0x54, 0x20, // s
        0x04, 0x04, 0x3F, 0x44, 0x44, // t
        0x3C, 0x40, 0x40, 0x20, 0x7C, // u
        0x1C, 0x20, 0x40, 0x20, 0x1C, // v
        0x3C, 0x40, 0x30, 0x40, 0x3C, // w
        0x44, 0x28, 0x10, 0x28, 0x44, // x
        0x0C, 0x50, 0x50, 0x50, 0x3C, // y
        0x44, 0x64, 0x54, 0x4C, 0x44, // z
        0x00, 0x08, 0x36, 0x41, 0x41, // {
        0x00, 0x00, 0x7F, 0x00, 0x00, // |
        0x41, 0x41, 0x36, 0x08, 0x00, // }
        0x02, 0x01, 0x02, 0x04, 0x02  // ~
};

//***********************************************************************************************
//									ST7565SPI internal func
//***********************************************************************************************
// 0-command 1-data
void ST7565SPI::setDC(bool val)
{
  digitalWrite(pinstA0, val);
};

void ST7565SPI::sendCommand(uint8_t command)
{
  setDC(false);
  SPI.write(command);
};

void ST7565SPI::sendData(uint8_t *data, uint8_t len)
{
  setDC(true);
  SPI.writeBytes(data, len);
};

void ST7565SPI::setCS(bool val)
{
  digitalWrite(pinstCS, val);
};

void ST7565SPI::setPage(uint8_t add)
{
  add = 0xb0 | add;
  sendCommand(add); 
}

void ST7565SPI::setCol(uint8_t add)
{
sendCommand((0x10 | (add >> 4)));
SPI.write((0x0f & add));  
}

void ST7565SPI::setInitLine(uint8_t add)
{
  add |= 0x40;       
  sendCommand(add);  
}


void ST7565SPI::initLCD()
{
  setCS(false);
  sendCommand(0xA2); // added 1/9 bias a3 1/7
  //          0b1010000x  0-normal -1 reverse
  SPI.write(0b10100000);    // ADC segment driver direction asc (a1-desc)
  SPI.write(0xC8);          // COM output scan direction desc (c0-asc)
  SPI.write((0x20 | 0x05)); // resistor ratio
  SPI.write(0x81); // electronic volume mode set
  SPI.write(0x15); // electronic volume register set
  SPI.write(0x2F); // operating mode
  SPI.write(0x40); // start line set
  SPI.write(0xAF); // display on
  //contrast
  SPI.write(0x81);
  SPI.write(ST7565_CONTRAST);
  //inversion
  SPI.write(ST7565_INVERSION ? 0xA7 : 0xA6);
  setCS(true);
}

//***********************************************************************************************
//                                ST7565SPI :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void ST7565SPI::print(char c)
{
  if ((c >= ' ') && (c <= '~')) // Check the legal range
  {
    memcpy(&stbuf[ychar].page[xchar], // Copy bytes for 1 character
           &font[(c - 32) * SSD1306FONTWIDTH], SSD1306FONTWIDTH);
  }
  else if (c == '\n') // New line?
  {
    xchar = dsp_getwidth(); // Yes, force next line
  }
  xchar += SSD1306FONTWIDTH;                       // Move x cursor
  if (xchar > (dsp_getwidth() - SSD1306FONTWIDTH)) // End of line?
  {
    xchar = 0;               // Yes, mimic CR
    ychar = (ychar + 1) & 7; // And LF
  }
}

//***********************************************************************************************
//                                ST7565SPI :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void ST7565SPI::print(const char *str)
{
  while (*str) // Print until delimeter
  {
    print(*str); // Print next character
    str++;
  }
}

//***********************************************************************************************
//                                ST7565SPI :: D I S P L A Y                                *
//***********************************************************************************************
// Refresh the display.                                                                         *
//***********************************************************************************************
void ST7565SPI::display()
{
  uint8_t pg; // Page number 0..SSD1306_NPAG - 1
  setCS(false);
  setInitLine(0x40);
  for (pg = 0; pg < ST7565_NPAG; pg++)
  {
      setPage(pg);
      setCol(0);
      sendData(stbuf[pg].page, 128);
  }
  setCS(true);
}

//***********************************************************************************************
//                              ST7565SPI :: C L E A R                                      *
//***********************************************************************************************
// Clear the display buffer and the display.                                                    *
//***********************************************************************************************
void ST7565SPI::clear()
{
  for (uint8_t pg = 0; pg < ST7565_NPAG; pg++)
  {
    memset(stbuf[pg].page, BLACK, dsp_getwidth()); // Clears all pixels of 1 page
  }
  xchar = 0;
  ychar = 0;
}

//***********************************************************************************************
//                                ST7565SPI :: S E T C U R S O R                            *
//***********************************************************************************************
// Position the cursor.  It will be set at a page boundary (line of texts).                     *
//***********************************************************************************************
void ST7565SPI::setCursor(uint8_t x, uint8_t y) // Position the cursor
{
  xchar = x % dsp_getwidth();
  ychar = y / 8 % ST7565_NPAG;
}

//***********************************************************************************************
//                                ST7565SPI :: F I L L R E C T                              *
//***********************************************************************************************
// Fill a rectangle.  It will clear a number of pages (lines of texts ).                        *
// Not very fast....                                                                            *
//***********************************************************************************************
void ST7565SPI::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
  uint8_t pg;     // Page to fill
  uint8_t pat;    // Fill pattern
  uint8_t xc, yc; // Running x and y in rectangle
  uint8_t *p;     // Point into ssdbuf

  for (yc = y; yc < (y + h); yc++) // Loop vertically
  {
    pg = (yc / 8) % ST7565_NPAG;     // Page involved
    pat = 1 << (yc & 7);             // Bit involved
    p = stbuf[pg].page + x;          // Point to right place
    for (xc = x; xc < (x + w); xc++) // Loop horizontally
    {
      if (color) // Set bit?
      {
        *p |= pat; // Yes, set bit
      }
      else
      {
        *p &= ~pat; // No, clear bit
      }
      p++; // Next 8 pixels
    }
  }
}

//***********************************************************************************************
//                                ST7565SPI                                                *
//***********************************************************************************************
// Constructor for the display.                                                                 *
//***********************************************************************************************
ST7565SPI::ST7565SPI(uint8_t pinA0, uint8_t pinCS)
{
  stbuf = (page_struct *)malloc(8 * sizeof(page_struct)); // Create buffer for screen
  pinstA0 = pinA0;
  pinstCS = pinCS;
  font = SSD1306font;
  pinMode(pinstCS, OUTPUT);
  pinMode(pinstA0, OUTPUT);
  setCS(false);
  sendCommand(0xE2); //reset
  setCS(true);
}

//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Show the current battery charge level on the screen.                                            *
// It will overwrite the top divider.                                                              *
// No action if bat0/bat100 not defined in the preferences.                                        *
//**************************************************************************************************
void displaybattery()
{
  if (tft)
  {
    if (ini_block.bat0 < ini_block.bat100) // Levels set in preferences?
    {
      static uint16_t oldpos = 0; // Previous charge level
      uint16_t ypos;              // Position on screen
      uint16_t v;                 // Constrainted ADC value
      uint16_t newpos;            // Current setting

      v = constrain(adcval, ini_block.bat0, // Prevent out of scale
                    ini_block.bat100);
      newpos = map(v, ini_block.bat0, // Compute length of green bar
                   ini_block.bat100,
                   0, dsp_getwidth());
      if (newpos != oldpos) // Value changed?
      {
        oldpos = newpos;                         // Remember for next compare
        ypos = tftdata[1].y - 5;                 // Just before 1st divider
        dsp_fillRect(0, ypos, newpos, 2, GREEN); // Paint green part
        dsp_fillRect(newpos, ypos,
                     dsp_getwidth() - newpos,
                     2, RED); // Paint red part
      }
    }
  }
}

//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Show the current volume as an indicator on the screen.                                          *
// The indicator is 2 pixels heigh.                                                                *
//**************************************************************************************************
void displayvolume()
{
  if (tft)
  {
    static uint8_t oldvol = 0; // Previous volume
    uint8_t newvol;            // Current setting
    uint16_t pos;              // Positon of volume indicator

    newvol = vs1053player->getVolume(); // Get current volume setting
    if (newvol != oldvol)               // Volume changed?
    {
      oldvol = newvol;                              // Remember for next compare
      pos = map(newvol, 0, 100, 0, dsp_getwidth()); // Compute position on TFT
      dsp_fillRect(0, dsp_getheight() - 2,
                   pos, 2, RED); // Paint red part
      dsp_fillRect(pos, dsp_getheight() - 2,
                   dsp_getwidth() - pos, 2, GREEN); // Paint green part
    }
  }
}

//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Show the time on the LCD at a fixed position in a specified color                               *
// To prevent flickering, only the changed part of the timestring is displayed.                    *
// An empty string will force a refresh on next call.                                              *
// A character on the screen is 8 pixels high and 6 pixels wide.                                   *
//**************************************************************************************************
void displaytime(const char *str, uint16_t color)
{
  static char oldstr[9] = "........";      // For compare
  uint8_t i;                               // Index in strings
  uint16_t pos = dsp_getwidth() + TIMEPOS; // X-position of character, TIMEPOS is negative

  if (str[0] == '\0') // Empty string?
  {
    for (i = 0; i < 8; i++) // Set oldstr to dots
    {
      oldstr[i] = '.';
    }
    return; // No actual display yet
  }
  if (tft) // TFT active?
  {
    dsp_setTextColor(color); // Set the requested color
    for (i = 0; i < 8; i++)  // Compare old and new
    {
      if (str[i] != oldstr[i]) // Difference?
      {
        dsp_fillRect(pos, 0, 6, 8, BLACK); // Clear the space for new character
        dsp_setCursor(pos, 0);             // Prepare to show the info
        dsp_print(str[i]);                 // Show the character
        oldstr[i] = str[i];                // Remember for next compare
      }
      pos += 6; // Next position
    }
  }
}
