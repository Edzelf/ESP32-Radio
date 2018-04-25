// bluetft.h
// Includes for various ST7735 displays.  Size is 160 x 128.
// Separated from the main sketch to allow several display types.

#include <Adafruit_ST7735.h>

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

// Data to display.  There are TFTSECS sections
#define TFTSECS 4
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
  { false, CYAN,   20, 64, "" },                            // 8 lines in the middle
  { false, YELLOW, 90, 32, "" },                            // 4 lines at the bottom
  { false, GREEN,  90, 32, "" }                             // 4 lines at the bottom for rotary encoder
} ;


Adafruit_ST7735*     tft = NULL ;                                  // For instance of display driver

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()       tft->setRotation ( 1 )             // Use landscape format (3 for upside down)
#define dsp_print(a)            tft->print ( a )                   // Print a string 
#define dsp_println(b)          tft->println ( b )                 // Print a string followed by newline 
#define dsp_fillRect(a,b,c,d,e) tft->fillRect ( a, b, c, d, e ) ;  // Fill a rectange
#define dsp_setTextSize(a)      tft->setTextSize(a)                // Set the text size
#define dsp_setTextColor(a)     tft->setTextColor(a)               // Set the text color
#define dsp_setCursor(a,b)      tft->setCursor ( a, b )            // Position the cursor
#define dsp_erase()             tft->fillScreen ( BLACK ) ;        // Clear the screen
#define dsp_getwidth()          160                                // Get width of screen
#define dsp_getheight()         128                                // Get height of screen
#define dsp_update()                                               // Updates to the physical screen

bool dsp_begin()
{
  tft = new Adafruit_ST7735 ( ini_block.tft_cs_pin,
                              ini_block.tft_dc_pin ) ;            // Create an instant for TFT
  // Uncomment one of the following initR lines for ST7735R displays
  //tft->initR ( INITR_GREENTAB ) ;                               // Init TFT interface
  //tft->initR ( INITR_REDTAB ) ;                                 // Init TFT interface
  //tft->initR ( INITR_BLACKTAB ) ;                               // Init TFT interface
  //tft->initR ( INITR_144GREENTAB ) ;                            // Init TFT interface
  //tft->initR ( INITR_MINI160x80 ) ;                             // Init TFT interface
  tft->initR ( INITR_BLACKTAB ) ;                                 // Init TFT interface
  // Uncomment the next line for ST7735B displays
  //tft_initB() ;
  return ( tft != NULL ) ;
}
