// bluetft.h
// Includes for blue ILI9163C display.  Size is 160 x 128.
// Separated from the main sketch to allow several display types.

#include <Ucglib.h>

// Color definitions for the TFT screen (if used)
// TFT has bits 6 bits (0..5) for RED, 6 bits (6..11) for GREEN and 4 bits (12..15) for BLUE.
#define BLACK   0x0000
#define BLUE    0xF800
#define RED     0x001F
#define GREEN   0x07E0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE


Ucglib_ILI9163_18x128x128_HWSPI* tft = NULL ;                         // For instance of display driver

// Various macro's to mimic the ILI9163C version of display functions
#define dsp_setRotation()       tft->setRotate270()                   // Use landscape format
#define dsp_print(a)            tft->print ( a )                      // Print a string 
#define dsp_println(a)          tft->println ( a )                    // Print string plus newline
#define dsp_fillRect(a,b,c,d,e) tft->setColor ( ( e & 0x001F ) << 1, \
                                                ( e & 0x07E0 ) >> 5, \
                                                ( e & 0xF800 ) >> 11 ) ; \
                                tft->drawBox ( a, b, c, d )           // Fill a rectange
#define dsp_setTextSize(a)                                            // Set the text size
#define dsp_setTextColor(a)     tft->setColor ( ( a & 0x001F ) << 1, \
                                                ( a & 0x07E0 ) >> 5, \
                                                ( a & 0xF800 ) >> 11 )
#define dsp_setCursor(a,b)      tft->setPrintPos ( a, b )             // Position the cursor

void dsp_begin()
{
  tft = new Ucglib_ILI9163_18x128x128_HWSPI (  ini_block.tft_dc_pin,
                                               ini_block.tft_cs_pin,
                                               UCG_PIN_VAL_NONE ) ;

  tft->begin ( UCG_FONT_MODE_TRANSPARENT ) ;                          // Init TFT interface
  tft->setColor(0, 120, 0, 0);
  tft->setColor(2, 0, 120, 0);
  tft->setColor(1, 120, 0, 120);
  tft->setColor(3, 0, 120, 120);
  tft->drawGradientBox(0, 0, tft->getWidth(), tft->getHeight());
  delay ( 1000 ) ;
  tft->setColor(0, 255, 255, 255);    // use white as main color for the font
  tft->setFontMode(UCG_FONT_MODE_TRANSPARENT);
  tft->setPrintPos(4,14);
  tft->setFont(ucg_font_helvB08_tr);
  tft->println("Success 1");
  tft->println("Success 2");
  delay ( 1000 ) ;
}




