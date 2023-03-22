// ILI9341.h
//
// Contributed by Uksa007@gmail.com
// Separated from the main sketch to allow several display types.
// Includes for various ILI9341 displays.  Tested on 320 x 240.
// Requires Adafruit ILI9341 library, available from library manager.
// Below set your dsp_getwidth() and dsp_getwidth() to suite your display.

#include "ILI9341.h"

char*       dbgprint ( const char* format, ... ) ;          // Print a formatted debug line

// Data to display.  There are TFTSECS sections
scrseg_struct     ILI9341_tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
  { false, CYAN,   20, 64, "" },                            // 8 lines in the middle
  { false, YELLOW, 90, 32, "" },                            // 4 lines at the bottom
  { false, GREEN,  90, 32, "" }                             // 4 lines at the bottom for rotary encoder
} ;


Adafruit_ILI9341*     ILI9341_tft ;                                       // For instance of display driver


bool ILI9341_dsp_begin ( int8_t cs, int8_t dc )
{
  if ( ( ILI9341_tft = new Adafruit_ILI9341 ( cs, dc ) ) )                // Create an instance for TFT
  {
    ILI9341_tft->begin() ;                                                // Init TFT interface
  }
  return ( ILI9341_tft != NULL ) ;
}

//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Show the current battery charge level on the screen.                                            *
// It will overwrite the top divider.                                                              *
// No action if bat0/bat100 not defined in the preferences.                                        *
//**************************************************************************************************
void ILI9341_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval )
{
  if ( ILI9341_tft )
  {
    if ( bat0 < bat100 )                                  // Levels set in preferences?
    {
      static uint16_t oldpos = 0 ;                        // Previous charge level
      uint16_t        ypos ;                              // Position on screen
      uint16_t        v ;                                 // Constrainted ADC value
      uint16_t        newpos ;                            // Current setting

      v = constrain ( adcval, bat0, bat100 ) ;            // Prevent out of scale
      newpos = map ( v, bat0, bat100,                     // Compute length of green bar
                     0, dsp_getwidth() ) ;
      if ( newpos != oldpos )                             // Value changed?
      {
        oldpos = newpos ;                                 // Remember for next compare
        ypos = ILI9341_tftdata[1].y - 5 ;                 // Just before 1st divider
        dsp_fillRect ( 0, ypos, newpos, 2, GREEN ) ;      // Paint green part
        dsp_fillRect ( newpos, ypos,
                       dsp_getwidth() - newpos,
                       2, RED ) ;                          // Paint red part
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
void ILI9341_displayvolume ( uint8_t vol )
{
  if ( ILI9341_tft )
  {
    static uint8_t oldvol = 0 ;                         // Previous volume
    uint16_t       len ;                                // Length of volume indicator in pixels

    if ( vol != oldvol )                                // Volume changed?
    {
      oldvol = vol ;                                    // Remember for next compare
      len = map ( vol, 0, 100, 0, dsp_getwidth() ) ;    // Compute length on TFT
      dsp_fillRect ( 0, dsp_getheight() - 2,
                     len, 2, RED ) ;                    // Paint red part
      dsp_fillRect ( len, dsp_getheight() - 2,
                     dsp_getwidth() - len, 2, GREEN ) ; // Paint green part
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
void ILI9341_displaytime ( const char* str, uint16_t color )
{
  static char oldstr[9] = "........" ;             // For compare
  uint8_t     i ;                                  // Index in strings
  uint16_t    pos = dsp_getwidth() + TIMEPOS ;     // X-position of character, TIMEPOS is negative

  if ( str[0] == '\0' )                            // Empty string?
  {
    for ( i = 0 ; i < 8 ; i++ )                    // Set oldstr to dots
    {
      oldstr[i] = '.' ;
    }
    return ;                                       // No actual display yet
  }
  if ( ILI9341_tft )                               // TFT active?
  {
    dsp_setTextColor ( color ) ;                   // Set the requested color
    for ( i = 0 ; i < 8 ; i++ )                    // Compare old and new
    {
      if ( str[i] != oldstr[i] )                   // Difference?
      {
        dsp_fillRect ( pos, 0, 6, 8, BLACK ) ;     // Clear the space for new character
        dsp_setCursor ( pos, 0 ) ;                 // Prepare to show the info
        dsp_print ( str[i] ) ;                     // Show the character
        oldstr[i] = str[i] ;                       // Remember for next compare
      }
      pos += 6 ;                                   // Next position
    }
  }
}


