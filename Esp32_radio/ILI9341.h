// ILI9341.h
//
// Contributed by Uksa007@gmail.com
// Separated from the main sketch to allow several display types.
// Includes for various ILI9341 displays.  Tested on 320 x 240.
// Requires Adafruit ILI9341 library, available from library manager.
// Below set your dsp_getwidth() and dsp_getwidth() to suite your display.

#include <Adafruit_ILI9341.h>

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

// Data to display.  There are TFTSECS sections
#define TFTSECS 4
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
  { false, CYAN,   20, 64, "" },                            // 8 lines in the middle
  { false, YELLOW, 90, 32, "" },                            // 4 lines at the bottom
  { false, GREEN,  90, 32, "" }                             // 4 lines at the bottom for rotary encoder
} ;


Adafruit_ILI9341*     tft = NULL ;                                  // For instance of display driver

// Various macro's to mimic the ILI9341 version of display functions
#define dsp_setRotation()       tft->setRotation ( 3 )             // Use landscape format (3 for upside down)
#define dsp_print(a)            tft->print ( a )                   // Print a string 
#define dsp_println(b)          tft->println ( b )                 // Print a string followed by newline 
#define dsp_fillRect(a,b,c,d,e) tft->fillRect ( a, b, c, d, e ) ;  // Fill a rectange
#define dsp_setTextSize(a)      tft->setTextSize(a)                // Set the text size
#define dsp_setTextColor(a)     tft->setTextColor(a)               // Set the text color
#define dsp_setCursor(a,b)      tft->setCursor ( a, b )            // Position the cursor
#define dsp_erase()             tft->fillScreen ( BLACK ) ;        // Clear the screen
#define dsp_getwidth()          320                                // Adjust to your display
#define dsp_getheight()         240                                // Get height of screen
#define dsp_update()                                               // Updates to the physical screen
#define dsp_usesSPI()           true                               // Does use SPI


bool dsp_begin()
{
  tft = new Adafruit_ILI9341 ( ini_block.tft_cs_pin,
                               ini_block.tft_dc_pin ) ;            // Create an instant for TFT

  tft->begin();                                                    // Init TFT interface
  return ( tft != NULL ) ;
}

