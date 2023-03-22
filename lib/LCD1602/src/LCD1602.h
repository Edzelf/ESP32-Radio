//***************************************************************************************************
//*  LCD1602.h -- Driver for LCD 1602 display with I2C backpack.                                    *
//***************************************************************************************************
// The backpack communicates with the I2C bus and converts the serial data to parallel for the      *
// 1602 board.                                                                                      *
// Do not forget the PULL-UP resistors (4.7k on both SDA and CLK).                                  *
// In the serial data, the 8 bits are assigned as follows:                                          *
// Bit   Destination  Description                                                                   *
// ---   -----------  ------------------------------------                                          *
//  0    RS           H=data, L=command                                                             *
//  1    RW           H=read, L=write.  Only write is used.                                         *
//  2    E            Enable                                                                        *
//  3    BL           Backlight, H=on, L=off.  Always on.                                           *
//  4    D4           Data bit 4                                                                    *
//  5    D5           Data bit 5                                                                    *
//  6    D6           Data bit 6                                                                    *
//  7    D7           Data bit 7                                                                    *
//***************************************************************************************************
//
// Note that the display function are limited due to the minimal available space.

#ifndef LCD1602_H
#define LCD1602_H
#include <Arduino.h>
#include <Wire.h>

#define LCD_I2C_ADDRESS 0x27                                          // Adjust for your display
#define ACKENA          true                                          // Enable ACK for I2C communication
#define INIPARS         ini_block.tft_sda_pin, ini_block.tft_scl_pin  // Parameters for dsp_begin
#define TIMEPOS         0                                             // Position (column) of time in topline (unused)
#define DISPLAYTYPE     "LCD1602"

// Color definitions for the TFT screen (if used)
#define BLACK   0
#define BLUE    1
#define RED     1
#define GREEN   0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE
#define GRAY    RED | GREEN | BLUE

#define DEFTXTSIZ   1                        // Default text size

#define DELAY_ENABLE_PULSE_SETTLE    50      // Command requires > 37us to settle
#define FLAG_BACKLIGHT_ON    0b00001000      // Bit 3, backlight enabled (disabled if clear)
#define FLAG_ENABLE          0b00000100      // Bit 2, Enable
#define FLAG_RS_DATA         0b00000001      // Bit 0, RS=data (command if clear)
#define FLAG_RS_COMMAND      0b00000000      // Command

#define COMMAND_CLEAR_DISPLAY               0x01
#define COMMAND_RETURN_HOME                 0x02
#define COMMAND_ENTRY_MODE_SET              0x04
#define COMMAND_DISPLAY_CONTROL             0x08
#define COMMAND_FUNCTION_SET                0x20
#define COMMAND_SET_DDRAM_ADDR              0x80
//
#define FLAG_DISPLAY_CONTROL_DISPLAY_ON     0x04
#define FLAG_DISPLAY_CONTROL_CURSOR_ON      0x02
//
#define FLAG_FUNCTION_SET_MODE_4BIT         0x00
#define FLAG_FUNCTION_SET_LINES_2           0x08
#define FLAG_FUNCTION_SET_DOTS_5X8          0x00
//
#define FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT 0x02
#define FLAG_ENTRY_MODE_SET_ENTRY_SHIFT_ON  0x01
//
// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()                                          // Use standard landscape format
#define dsp_print(a)                                               // Print a string 
#define dsp_println(a)                                             // Print string plus newline
#define dsp_fillRect(a,b,c,d,e)                                    // Fill a rectange
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()         LCD1602_tft->sclear()                  // Clear the screen
#define dsp_getwidth()      16                                     // Get width of screen
#define dsp_getheight()     2                                      // Get height of screen
#define dsp_usesSPI()       false                                  // Does not use SPI
#define dsp_begin           LCD1602_dsp_begin                      // Init driver
#define dsp_update          LCD1602_dsp_update                     // Update one line


#define TFTSECS 4                                     // 4 sections, only 2 used

#define tftdata             LCD1602_tftdata
#define displaybattery      LCD1602_displaybattery
#define displayvolume       LCD1602_displayvolume
#define displaytime         LCD1602_displaytime

struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;

extern scrseg_struct LCD1602_tftdata[TFTSECS] ;

void LCD1602_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval ) ;
void LCD1602_displayvolume  ( uint8_t vol ) ;
void LCD1602_displaytime    ( const char* str, uint16_t color = 0xFFFF ) ;
bool LCD1602_dsp_begin      ( int8_t sda, int8_t scl ) ;
void LCD1602_dsp_update     ( bool isvolume ) ;

class LCD1602
{
  public:
                     LCD1602 ( int8_t sda, int8_t scl ) ;   // Constructor
    void             print ( char c ) ;                     // Send 1 char
    void             reset() ;                              // Perform reset
    void             sclear() ;                             // Clear the screen
    void             shome() ;                              // Go to home position
    void             scursor ( uint8_t col, uint8_t row ) ; // Position the cursor
    void             scroll ( bool son ) ;                  // Set scroll on/off
  private:
    void             scommand ( uint8_t cmd ) ;
    void             strobe ( uint8_t cmd ) ;
    void             swrite ( uint8_t val, uint8_t rs ) ;
    void             write_cmd ( uint8_t val ) ;
    void             write_data ( uint8_t val ) ;
    uint8_t          bl  = FLAG_BACKLIGHT_ON ;              // Backlight in every command
    uint8_t          xchar = 0 ;                            // Current cursor position (text)
    uint8_t          ychar = 0 ;                            // Current cursor position (text)
} ;

extern LCD1602* LCD1602_tft  ;

#endif
