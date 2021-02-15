#include <Adafruit_SSD1306.h>

// oled.h
// Includes for 0.96 inch oled display.  Size is 128 x 64.
// Separated from the main sketch to allow several display types.

#include <U8g2lib.h>


// Color definitions for the TFT screen (if used)
// OLED only have 2 colors.
#define BLACK   0
#define BLUE    1
#define RED     1
#define GREEN   1
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE

uint8_t tft_x, tft_y ;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C* tft = NULL ;                         // For instance of display driver

// Various macro's to mimic the ILI9163C version of display functions
#define dsp_setRotation()                                                 // Use standard landscape format
#define dsp_print(a)            tft->print ( a )                          // Print a string 
#define dsp_println(a)          tft->println ( a )                        // Print string plus newline
#define dsp_fillRect(a,b,c,d,e) tft->drawBox ( a, b, c, d )               // Fill a rectange
#define dsp_setTextSize(a)                                                // Set the text size
#define dsp_setTextColor(a)     tft->setDrawColor ( a )
#define dsp_setCursor(a,b)      tft->setCursor ( a, b )                   // Position the cursor

void dsp_begin()
{
  tft = new U8G2_SSD1306_128X64_NONAME_1_SW_I2C ( U8G2_R0,              // No rotation
                                                  ini_block.tft_cs_pin, // Is in fact SCL
                                                  ini_block.tft_dc_pin, // Is in fact SDA
                                                  U8X8_PIN_NONE ) ;     // No reset pin

  tft->begin() ;                                                // Init TFT interface
  tft->setFont ( u8x8_font_chroma48medium8_r ) ;
  tft->println ( "Hallo dan toch" ) ;
  tft->println ( "Hallo dan toch" ) ;
  tft->sendBuffer() ;                                           // transfer internal memory to the display

  delay ( 1000 ) ;
}

