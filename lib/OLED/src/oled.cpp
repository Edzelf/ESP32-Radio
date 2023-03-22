//***************************************************************************************************
//                                   O L E D . C P P                                                *
//***************************************************************************************************
// Driver for SSD1306/SSD1309/SH1106 display                                                        *
//***************************************************************************************************
// 25-02-2021, ES: Correct bug, isSH1106 was always set.                                            *
// 04-05-2022, ES: Uses Wire library now
//***************************************************************************************************
#include <Wire.h>
#include <string.h>
#include "oled.h"

char*       dbgprint ( const char* format, ... ) ;    // Print a formatted debug line

uint16_t     oledtyp ;                                // Type of OLED
OLED*        tft ;                                    // Object for display
scrseg_struct OLED_tftdata[TFTSECS] =                 // Screen divided in 3 segments + 1 overlay
{                                                     // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                      // 1 top line
  { false, CYAN,    8, 32, "" },                      // 4 lines in the middle
  { false, YELLOW, 40, 22, "" },                      // 3 lines at the bottom, leave room for indicator
  { false, GREEN,  40, 22, "" }                       // 3 lines at the bottom for rotary encoder
} ;


//***********************************************************************************************
//                                  O L E D _ D S P _ B E G I N                                 *
//***********************************************************************************************
// Init display.                                                                                *
//***********************************************************************************************
bool oled_dsp_begin ( int8_t sda_pin, int8_t scl_pin, uint16_t olt )
{
  oledtyp = olt ;                                  // Save OLED type
  dbgprint ( "Init OLED %d, I2C pins %d,%d",
             oledtyp,
             sda_pin,
             scl_pin ) ;
  if ( ( sda_pin >= 0 ) &&
       ( scl_pin >= 0 ) )
  {
    tft = new OLED ( sda_pin,
                     scl_pin ) ;                    // Create an instant for TFT
  }
  else
  {
    dbgprint ( "Init OLED failed!" ) ;
  }
  return ( tft != NULL ) ;
}


//***********************************************************************************************
//                                      O L E D :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void OLED::print ( char c )
{
  if ( ( c >= ' ' ) && ( c <= '~' ) )                    // Check the legal range
  {
    memcpy ( &ssdbuf[ychar].page[xchar],                 // Copy bytes for 1 character
             &font[(c-32)*OLEDFONTWIDTH],
             OLEDFONTWIDTH ) ;
    ssdbuf[ychar].dirty = true ;                         // Notice change
  }
  else if ( c == '\n' )                                  // New line?
  {
    xchar = SCREEN_WIDTH ;                               // Yes, force next line
  }
  xchar += ( OLEDFONTWIDTH + 1 ) ;                      // Move x cursor
  if ( xchar > ( SCREEN_WIDTH - OLEDFONTWIDTH ) )        // End of line?
  {
    xchar = 0 ;                                          // Yes, mimic CR
    ychar = ( ychar + 1 ) & 7 ;                          // And LF
  }
}


//***********************************************************************************************
//                                   O L E D :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void OLED::print ( const char* str )
{
  while ( *str )                                // Print until delimeter
  {
    print ( *str ) ;                            // Print next character
    str++ ;
  }
}


//***********************************************************************************************
//                                O L E D :: D I S P L A Y                                   *
//***********************************************************************************************
// Refresh the display.                                                                         *
//***********************************************************************************************
void OLED::display()
{
  uint8_t        pg ;                                         // Page number 0..OLED_NPAG - 1
  static bool    a  = true ;

  if ( a )
  {
    Serial.printf ( "I2C buf is %d\n", I2C_BUFFER_LENGTH ) ;
    a = false ;
  }
  for ( pg = 0 ; pg < OLED_NPAG ; pg++ )
  {
    if ( ssdbuf[pg].dirty )                                   // Refresh needed?
    {
      ssdbuf[pg].dirty = false ;                              // Yes, set page to "up-to-date"
      Wire.beginTransmission ( OLED_I2C_ADDRESS ) ;           // Begin transmission
      Wire.write ( (uint8_t)OLED_CONTROL_BYTE_CMD_STREAM ) ;  // Set single byte command mode
      Wire.write ( (uint8_t)(0xB0 | pg ) ) ;                  // Set page address
      if ( ( oledtyp == 1106 ) || ( oledtyp == 1309 ) )       // Is it an SH1106/ SSD1309?
      {
        Wire.write ( (uint8_t)0x00 ) ;                        // Set lower column address to 0
        Wire.write ( (uint8_t)0x10 ) ;                        // Set higher column address to 0
      }
      Wire.endTransmission() ;                                // End of transmission
      Wire.beginTransmission ( OLED_I2C_ADDRESS ) ;           // Begin transmission
      Wire.write ( (uint8_t)OLED_CONTROL_BYTE_DATA_STREAM ) ; // Set multi byte data mode
      if ( oledtyp == 1106 )                                  // Is it an SH1106?
      {
        uint8_t fillbuf[] = { 0, 0, 0, 0 } ;                  // To clear 4 bytes of SH1106 RAM
        Wire.write ( fillbuf, 4 ) ;                           // Yes, fill extra RAM with zeroes
      }
      Wire.write ( ssdbuf[pg].page, 64 ) ;                    // Send 1st half page with data
      Wire.endTransmission() ;                                // End of transmission
      // Channel is closed and reopened because of limited buffer space
      Wire.beginTransmission ( OLED_I2C_ADDRESS ) ;           // Begin transmission
      Wire.write ( (uint8_t)OLED_CONTROL_BYTE_DATA_STREAM ) ; // Set multi byte data mode
      Wire.write ( ssdbuf[pg].page + 64, 64 ) ;               // Send 2nd half page with data
      Wire.endTransmission() ;                                // End of transmission
    }
  }
}


//***********************************************************************************************
//                                 O L E D :: C L E A R                                         *
//***********************************************************************************************
// Clear the display buffer and the display.                                                    *
//***********************************************************************************************
void OLED::clear()
{
  for ( uint8_t pg = 0 ; pg < OLED_NPAG ; pg++ )         // Handle all pages
  {
    memset ( ssdbuf[pg].page, BLACK, SCREEN_WIDTH ) ;    // Clears all pixels of 1 page
    ssdbuf[pg].dirty = true ;                            // Force refresh
  }
  xchar = 0 ;
  ychar = 0 ;
}


//***********************************************************************************************
//                                   O L E D :: S E T C U R S O R                               *
//***********************************************************************************************
// Position the cursor.  It will be set at a page boundary (line of texts).                     *
//***********************************************************************************************
void OLED::setCursor ( uint8_t x, uint8_t y )          // Position the cursor
{
  xchar = x % SCREEN_WIDTH ;
  ychar = y / 8 % OLED_NPAG ;
}


//***********************************************************************************************
//                                   O L E D :: F I L L R E C T                                 *
//***********************************************************************************************
// Fill a rectangle.  It will clear a number of pages (lines of texts ).                        *
// Not very fast....                                                                            *
//***********************************************************************************************
void OLED::fillRect ( uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color )
{
  uint8_t  pg ;                                           // Page to fill
  uint8_t  pat ;                                          // Fill pattern
  uint8_t  xc, yc ;                                       // Running x and y in rectangle
  uint8_t* p ;                                            // Point into ssdbuf
  
  for ( yc = y ; yc < ( y + h ) ; yc++ )                  // Loop vertically
  {
    pg = ( yc / 8 ) % OLED_NPAG ;                         // Page involved
    pat = 1 << ( yc & 7 ) ;                               // Bit involved
    p = ssdbuf[pg].page + x ;                             // Point to right place
    for ( xc = x ; xc < ( x + w ) ; xc++ )                // Loop horizontally
    {
      if ( color )                                        // Set bit?
      {
        *p |= pat ;                                       // Yes, set bit
      }
      else
      {
        *p &= ~pat ;                                      // No, clear bit
      }
      p++ ;                                               // Next 8 pixels
    }
    ssdbuf[pg].dirty = true ;                             // Page has been changed
  }
}


//***********************************************************************************************
//                                O L E D constructor                                           *
//***********************************************************************************************
// Constructor for the display.                                                                 *
//***********************************************************************************************
OLED::OLED ( int8_t sda, int8_t scl )
{
  uint8_t      initbuf[] =                                   // Initial commands to init OLED
                  {
                    OLED_CONTROL_BYTE_CMD_STREAM,            // Stream next bytes
                    OLED_CMD_DISPLAY_OFF,                    // Display off
                    OLED_CMD_SET_DISPLAY_CLK_DIV, 0x80,      // Set divide ratio
                    OLED_CMD_SET_MUX_RATIO, SCREEN_HEIGHT-1, // Set multiplex ration (1:HEIGHT)
                    OLED_CMD_SET_DISPLAY_OFFSET, 0,          // Set diplay offset to 0
                    OLED_CMD_SET_DISPLAY_START_LINE,         // Set start line address
                    OLED_CMD_SET_CHARGE_PUMP, 0x14,          // Enable charge pump
                    OLED_CMD_SET_MEMORY_ADDR_MODE, 0x00,     // Set horizontal addressing mode
                    OLED_CMD_SET_SEGMENT_REMAP,              // Mirror X
                    OLED_CMD_SET_COM_SCAN_MODE,              // Mirror Y
                    OLED_CMD_SET_COM_PIN_MAP, 0x12,          // Set com pins hardware config
                    OLED_CMD_SET_CONTRAST, 0xFF,             // Set contrast
                    OLED_CMD_SET_PRECHARGE, 0xF1,            // Set precharge period
                    OLED_CMD_SET_VCOMH_DESELCT, 0x20,        // Set VCOMH
                    OLED_CMD_DISPLAY_RAM,                    // Output followes RAM
                    OLED_CMD_DISPLAY_NORMAL,                 // Set normal color
                    OLED_CMD_SCROLL_OFF,                     // Stop scrolling
                    OLED_CMD_DISPLAY_ON                      // Display on
                  } ;
  ssdbuf = (page_struct*) malloc ( 8 * sizeof(page_struct) ) ;  // Create buffer for screen
  font = OLEDfont ;
  Wire.begin ( sda, scl ) ;                                     // Init I2c
  Wire.beginTransmission ( OLED_I2C_ADDRESS ) ;                 // Begin transmission
  Wire.write ( initbuf, sizeof(initbuf) ) ;                     // Write init buffer
  Wire.endTransmission() ;                                      // End of transmission
  clear() ;                                                     // Clear the display
}


//**************************************************************************************************
//                               O L E D _ D I S P L A Y B A T T E R Y                             *
//**************************************************************************************************
// Show the current battery charge level on the screen.                                            *
// It will overwrite the top divider.                                                              *
// No action if bat0/bat100 not defined in the preferences.                                        *
//**************************************************************************************************
void oled_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval )
{
  if ( tft )
  {
    if ( bat0 < bat100 )                                  // Levels set in preferences?
    {
      static uint16_t oldpos = 0 ;                        // Previous charge level
      uint16_t        ypos ;                              // Position on screen
      uint16_t        v ;                                 // Constrainted ADC value
      uint16_t        newpos ;                            // Current setting

      v = constrain ( adcval, bat0,                       // Prevent out of scale
                      bat100 ) ;
      newpos = map ( v, bat0,                             // Compute length of green bar
                     bat100,
                     0, dsp_getwidth() ) ;
      if ( newpos != oldpos )                             // Value changed?
      {
        oldpos = newpos ;                                 // Remember for next compare
        ypos = tftdata[1].y - 5 ;                         // Just before 1st divider
        dsp_fillRect ( 0, ypos, newpos, 2, GREEN ) ;      // Paint green part
        dsp_fillRect ( newpos, ypos,
                       dsp_getwidth() - newpos,
                       2, RED ) ;                          // Paint red part
      }
    }
  }
}


//**************************************************************************************************
//                             O L E D _ D I S P L A Y V O L U M E                                 *
//**************************************************************************************************
// Show the current volume as an indicator on the screen.                                          *
// The indicator is 2 pixels heigh.                                                                *
//**************************************************************************************************
void oled_displayvolume ( uint8_t vol )
{
  if ( tft )
  {
    static uint8_t oldvol = 0 ;                         // Previous volume
    uint16_t       pos ;                                // Positon of volume indicator

    if ( vol != oldvol )                                // Volume changed?
    {
      oldvol = vol ;                                    // Remember for next compare
      pos = map ( vol, 0, 100, 0, dsp_getwidth() ) ;   // Compute position on TFT
      dsp_fillRect ( 0, dsp_getheight() - 2,
                     pos, 2, RED ) ;                    // Paint red part
      dsp_fillRect ( pos, dsp_getheight() - 2,
                     dsp_getwidth() - pos, 2, GREEN ) ; // Paint green part
    }
  }
}


//**************************************************************************************************
//                            O L E D _ D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Show the time on the LCD at a fixed position in a specified color                               *
// To prevent flickering, only the changed part of the timestring is displayed.                    *
// An empty string will force a refresh on next call.                                              *
// A character on the screen is 8 pixels high and 6 pixels wide.                                   *
//**************************************************************************************************
void oled_displaytime ( const char* str, uint16_t color )
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
  if ( tft )                                       // TFT active?
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
