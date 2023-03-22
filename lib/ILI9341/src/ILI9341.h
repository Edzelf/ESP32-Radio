//**************************************************************************************************
// ILI9341.h
//**************************************************************************************************
//
// Contributed by Uksa007@gmail.com
// Separated from the main sketch to allow several display types.
// Includes for various ILI9341 displays.  Tested on 320 x 240.
// Requires Adafruit ILI9341 library, available from library manager.
// Below set your dsp_getwidth() and dsp_getwidth() to suite your display.
#ifndef ILI9341_H
#define ILI9341_H

#include <Adafruit_ILI9341.h>

#define TIMEPOS     -52                         // Position (column) of time in topline relative to end
#define INIPARS     ini_block.tft_cs_pin, ini_block.tft_dc_pin  // Prameters for dsp_begin
#define DISPLAYTYPE "ILI9341"

// Color definitions for the TFT screen (if used)
// TFT has bits 6 bits (0..5) for RED, 6 bits (6..11) for GREEN and 4 bits (12..15) for BLUE.
#define BLACK   ILI9341_BLACK
#define BLUE    ILI9341_BLUE
#define RED     ILI9341_RED
#define GREEN   ILI9341_GREEN
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | BLUE | GREEN
#define GRAY    0x7bf0

#define DEFTXTSIZ   1                                 // Default text size

struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;


#define TFTSECS 4

#define tftdata             ILI9341_tftdata
#define displaybattery      ILI9341_displaybattery
#define displayvolume       ILI9341_displayvolume
#define displaytime         ILI9341_displaytime


extern Adafruit_ILI9341*    ILI9341_tft ;                                 // For instance of display driver

// Various macro's to mimic the ILI9341 version of display functions
#define dsp_setRotation()       ILI9341_tft->setRotation ( 3 )            // Use landscape format (3 for upside down)
#define dsp_print(a)            ILI9341_tft->print ( a )                  // Print a string 
#define dsp_println(b)          ILI9341_tft->println ( b )                // Print a string followed by newline 
#define dsp_fillRect(a,b,c,d,e) ILI9341_tft->fillRect ( a, b, c, d, e ) ; // Fill a rectange
#define dsp_setTextSize(a)      ILI9341_tft->setTextSize(a)               // Set the text size
#define dsp_setTextColor(a)     ILI9341_tft->setTextColor(a)              // Set the text color
#define dsp_setCursor(a,b)      ILI9341_tft->setCursor ( a, b )           // Position the cursor
#define dsp_erase()             ILI9341_tft->fillScreen ( BLACK ) ;       // Clear the screen
#define dsp_getwidth()          320                                       // Adjust to your display
#define dsp_getheight()         240                                       // Get height of screen
#define dsp_update(a)                                                     // Updates to the physical screen
#define dsp_usesSPI()           true                                      // Does use SPI
#define dsp_begin               ILI9341_dsp_begin                         // Init driver

extern scrseg_struct     ILI9341_tftdata[TFTSECS] ;                       // Screen divided in segments

void ILI9341_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval ) ;
void ILI9341_displayvolume  ( uint8_t vol ) ;
void ILI9341_displaytime    ( const char* str, uint16_t color = 0xFFFF ) ;
bool ILI9341_dsp_begin      ( int8_t cs, int8_t dc ) ;

#endif
