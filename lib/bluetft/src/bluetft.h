//**************************************************************************************************
// bluetft.h                                                                                       *
//**************************************************************************************************
// Separated from the main sketch to allow several display types.                                  *
// Includes for various ST7735 displays.  Size is 160 x 128.  Select INITR_BLACKTAB                *
// for this and set dsp_getwidth() to 160.                                                         *
// Works also for the 128 x 128 version.  Select INITR_144GREENTAB for this and                    *
// set dsp_getwidth() to 128.                                                                      *
//**************************************************************************************************
#ifndef BLUETFT_H
#define BLUETFT_H
#include <Adafruit_ST7735.h>

#define TIMEPOS     -52                         // Position (column) of time in topline relative to end
#define INIPARS     ini_block.tft_cs_pin, ini_block.tft_dc_pin  // Prameters for dsp_begin
#define DISPLAYTYPE "BLUETFT"

// Color definitions for the TFT screen (if used)
// TFT has bits 6 bits (0..5) for RED, 6 bits (6..11) for GREEN and 4 bits (12..15) for BLUE.
#define BLACK   ST7735_BLACK
#define BLUE    ST7735_BLUE
#define RED     ST7735_RED
#define GREEN   ST7735_GREEN
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | BLUE | GREEN
#define GRAY    0x7bf0

#define DEFTXTSIZ  1                                  // Default text size


struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;


// Data to display.  There are TFTSECS sections
#define TFTSECS 4

#define tftdata             bluetft_tftdata
#define displaybattery      bluetft_displaybattery
#define displayvolume       bluetft_displayvolume
#define displaytime         bluetft_displaytime

extern Adafruit_ST7735*     bluetft_tft ;                                 // For instance of display driver

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()       bluetft_tft->setRotation ( 1 )            // Use landscape format (3 for upside down)
#define dsp_print(a)            bluetft_tft->print ( a )                  // Print a string 
#define dsp_println(b)          bluetft_tft->println ( b )                // Print a string followed by newline 
#define dsp_fillRect(a,b,c,d,e) bluetft_tft->fillRect ( a, b, c, d, e ) ; // Fill a rectange
#define dsp_setTextSize(a)      bluetft_tft->setTextSize(a)               // Set the text size
#define dsp_setTextColor(a)     bluetft_tft->setTextColor(a)              // Set the text color
#define dsp_setCursor(a,b)      bluetft_tft->setCursor ( a, b )           // Position the cursor
#define dsp_erase()             bluetft_tft->fillScreen ( BLACK ) ;       // Clear the screen
#define dsp_getwidth()          160                                       // Adjust to your display
#define dsp_getheight()         128                                       // Get height of screen
#define dsp_update(a)                                                     // Updates to the physical screen
#define dsp_usesSPI()           true                                      // Does use SPI
#define dsp_begin               bluetft_dsp_begin                         // Init driver

extern scrseg_struct     bluetft_tftdata[TFTSECS] ;                       // Screen divided in segments

//void bluetft_dsp_fillRect   ( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color ) ;
void bluetft_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval ) ;
void bluetft_displayvolume  ( uint8_t vol ) ;
void bluetft_displaytime    ( const char* str, uint16_t color = 0xFFFF ) ;
bool bluetft_dsp_begin      ( int8_t cs, int8_t dc ) ;
#endif
