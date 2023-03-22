//***************************************************************************************************
//*  LCD2004.cpp -- Driver for LCD 2004 display with I2C backpack.                                  *
//***************************************************************************************************
// The backpack communicates with the I2C bus and converts the serial data to parallel for the      *
// 2004 board.                                                                                      *
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
//                                                                                                  *
// Note that the display function are limited due to the minimal available space.                   *
//  History:                                                                                        *
//   Date     Author        Remarks                                                                 *
// ----------  --  ------------------------------------------------------------------               *
// 12-05-2022, ES: Correction scrolling lines.                                                      *
//***************************************************************************************************

#include <Arduino.h>
#include "LCD2004.h"
#include <time.h>

char*       dbgprint ( const char* format, ... ) ;          // Print a formatted debug line
extern      struct tm timeinfo ;                            // Will be filled by NTP server


scrseg_struct     LCD2004_tftdata[TFTSECS] =                // Screen divided in 4 segments
{
  { false, WHITE,   0,  0, "" },                            // 1 top line (dummy)
  { false, WHITE,   0,  0, "" },                            // 8 lines in the middle
  { false, WHITE,   0,  0, "" },                            // 4 lines at the bottom
  { false, WHITE,   0,  0, "" }                             // 4 lines at the bottom for rotary encoder
} ;

LCD2004* LCD2004_tft = NULL ;

bool LCD2004_dsp_begin (  int8_t sda, int8_t scl  )
{
  dbgprint ( "Init LCD2004, I2C pins %d,%d", sda, scl ) ;
  if ( ( sda >= 0 ) && ( scl >= 0 ) )
  {
    LCD2004_tft = new LCD2004 ( sda, scl ) ;                // Create an instance for TFT
  }
  else
  {
    dbgprint ( "Init LCD2004 failed!" ) ;
  }
  return ( LCD2004_tft != NULL ) ;
}


//***********************************************************************************************
//                                L C D 2 0 0 4  write functions                                *
//***********************************************************************************************
// Write functins for command, data and general.                                                *
//***********************************************************************************************
void LCD2004::swrite ( uint8_t val, uint8_t rs )          // General write, 8 bits data
{
  strobe ( ( val & 0xF0 ) | rs ) ;                        // Send 4 LSB bits
  strobe ( ( val << 4 ) | rs ) ;                          // Send 4 MSB bits
}


void LCD2004::write_data ( uint8_t val )
{
  swrite ( val, FLAG_RS_DATA ) ;                           // Send data (RS = HIGH)
}


void LCD2004::write_cmd ( uint8_t val )
{
  swrite ( val, FLAG_RS_COMMAND ) ;                        // Send command (RS = LOW)
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S T R O B E                                  *
//***********************************************************************************************
// Send data followed by strobe to clock data to LCD.                                           *
//***********************************************************************************************
void LCD2004::strobe ( uint8_t cmd )
{
  scommand ( cmd | FLAG_ENABLE ) ;                  // Send command with E high
  delayMicroseconds ( DELAY_ENABLE_PULSE_SETTLE ) ; // Wait a short time
  scommand ( cmd ) ;                                // Same command with E low
  delayMicroseconds ( DELAY_ENABLE_PULSE_SETTLE ) ; // Wait a short time
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S C O M M A N D                              *
//***********************************************************************************************
// Send a command to the LCD.                                                                   *
// Actual I/O.  Open a channel to the I2C interface and write one byte.                         *
//***********************************************************************************************
void LCD2004::scommand ( uint8_t cmd )
{
  Wire.beginTransmission ( LCD_I2C_ADDRESS ) ;
  Wire.write ( cmd | bl ) ;                               // Add command including BL state
  Wire.endTransmission() ;
}



//***********************************************************************************************
//                                L C D 2 0 0 4 :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void LCD2004::print ( char c )
{
  write_data ( c ) ;
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S C U R S O R                                *
//***********************************************************************************************
// Place the cursor at the requested position.                                                  *
//***********************************************************************************************
void LCD2004::scursor ( uint8_t col, uint8_t row )
{
  const int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 } ;
  
  write_cmd ( COMMAND_SET_DDRAM_ADDR |
              ( col + row_offsets[row] ) ) ; 
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S C L E A R                                  *
//***********************************************************************************************
// Clear the LCD.                                                                               *
//***********************************************************************************************
void LCD2004::sclear()
{
  write_cmd ( COMMAND_CLEAR_DISPLAY ) ;
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S C R O L L                                  *
//***********************************************************************************************
// Set scrolling on/off.                                                                        *
//***********************************************************************************************
void LCD2004::scroll ( bool son )
{
  uint8_t ecmd = COMMAND_ENTRY_MODE_SET |               // Assume no scroll
                 FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT ;

  if ( son )                                            // Scroll on?
  {
    ecmd |= FLAG_ENTRY_MODE_SET_ENTRY_SHIFT_ON ;        // Yes, change function
  }
  write_cmd ( ecmd ) ;                                  // Perform command
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: S H O M E                                    *
//***********************************************************************************************
// Go to home position.                                                                         *
//***********************************************************************************************
void LCD2004::shome()
{
  write_cmd ( COMMAND_RETURN_HOME ) ;
}


//***********************************************************************************************
//                                L C D 2 0 0 4 :: R E S E T                                    *
//***********************************************************************************************
// Reset the LCD.                                                                               *
//***********************************************************************************************
void LCD2004::reset()
{
  uint8_t initcmds[] = {
              COMMAND_FUNCTION_SET |
              FLAG_FUNCTION_SET_MODE_4BIT |
              FLAG_FUNCTION_SET_LINES_2 |
              FLAG_FUNCTION_SET_DOTS_5X8,
              COMMAND_DISPLAY_CONTROL |
              FLAG_DISPLAY_CONTROL_DISPLAY_ON,
              COMMAND_CLEAR_DISPLAY,
              COMMAND_ENTRY_MODE_SET |
              FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT } ;

  scommand ( 0 ) ;                                // Put expander to known state
  delay ( 35 ) ;
  for ( int i = 0 ; i < 3 ; i++ )                 // Repeat 3 times
  {
    strobe ( 0x03 << 4 ) ;                        // Select 4-bit mode
    delay ( 5 ) ;
  }
  strobe ( 0x02 << 4 ) ;                          // 4-bit
  delay ( 5 ) ;
  for ( int i = 0 ; i < sizeof(initcmds) ; i++ )  // Send initial commands
  {
    write_cmd ( initcmds[i] ) ;
    delay ( 5 ) ;
  }
  shome() ;
}


//**************************************************************************************************
//                                          I 2 C S C A N                                          *
//**************************************************************************************************
// Utility to scan the I2C bus.                                                                    *
//**************************************************************************************************
// void i2cscan()
// {
//   byte error, address ;

//   dbgprint ( "Scanning I2C bus..." ) ;

//   for ( address = 1 ; address < 127 ; address++ ) 
//   {
//     Wire.beginTransmission ( address ) ;
//     error = Wire.endTransmission() ;
//     if ( error == 0 )
//     {
//       dbgprint ( "I2C device 0x%02X found", address ) ;
//     }
//     else if ( error == 4 ) 
//     {
//       dbgprint ( "Error 4 at address 0x%02X", address ) ;
//     }    
//   }
// }


//***********************************************************************************************
//                                L C D 2 0 0 4                                                 *
//***********************************************************************************************
// Constructor for the display.                                                                 *
//***********************************************************************************************
LCD2004::LCD2004 ( int8_t sda, int8_t scl )
{
  uint8_t error ;

  if ( ! Wire.begin ( sda, scl ) )                             // Init I2c
  {
    dbgprint ( "I2C driver install error!" ) ;
  }
  else
  {
    // i2cscan() ;                                               // Scan I2C bus
    Wire.beginTransmission ( LCD_I2C_ADDRESS ) ;
    error = Wire.endTransmission() ;
    if ( error )
    {
      dbgprint ( "Display not found on I2C 0x%02X",
                  LCD_I2C_ADDRESS ) ;
    }    
  }
  reset() ;
}

struct dsp_str
{
  String          str ;
  uint16_t        len ;                                 // Length of string to show
  uint16_t        pos ;                                 // Start on this position of string
  uint8_t         row ;                                 // Row on display  
} ;

dsp_str dline[4] = { { "", 0, 0, 0 },
                     { "", 0, 0, 0 },
                     { "", 0, 0, 0 },
                     { "", 0, 0, 0 }
                   } ;

//***********************************************************************************************
//                                D S P _U P D A T E _ L I N E                                  *
//***********************************************************************************************
// Show a selected line                                                                         *
// The resulting line is a scrolling part of the complete line.  The startposition is           *
// ncremented by one for each call.                                                             *
//***********************************************************************************************
void LCD2004_dsp_update_line ( uint8_t lnr )
{
  int             i ;                                 // Index in string
  const char*     p ;                                 // Pointer to converted string
  dsp_str*        line = &dline[lnr] ;                // Pointer to line in dline

  p = line->str.c_str() ;                             // Get pointer to string
  line->len = strlen ( p ) ;                          // Get string length
  //dbgprint ( "Str %d, len is %d, str is %s",
  //           lnr,
  //           line->len, p ) ;
  if ( line->len > dsp_getwidth() )                   // Full string fits?
  {
    if ( line->pos >= line->len )                     // No, pos beyond or at end of string?
    {
      line->pos = 0 ;                                 // Yes, restart string
    }
    else
    {
      p += line->pos ;                                // No, set begin of substring
    }
  }
  else
  {
    line->pos = 0 ;                                   // String fits on screen
  }
  LCD2004_tft->scursor ( 0, lnr ) ;
  for ( i = 0 ; i < dsp_getwidth() ; i++ )
  {
    if ( ( ( *p >= ' ' ) && ( *p <= '~' ) ) ||        // Printable?
         ( *p == '\xFF' ) )                           // Allow block
    {
      LCD2004_tft->print ( *p ) ;                     // Yes
    }
    else
    {
      LCD2004_tft->print ( ' ' ) ;                    // Yes, print space
    }
    if ( *p )                                         // End of string reached?
    {
      p++ ;                                           // No, update pointer
    }
  }
  line->pos++ ;                                       // No, move start position next call
}


//***********************************************************************************************
//                                D S P _U P D A T E                                            *
//***********************************************************************************************
// Show a selection of the 4 sections                                                           *
//***********************************************************************************************
void LCD2004_dsp_update ( bool isvolume )
{
  static uint16_t cnt = 0 ;                             // Reduce updates

  if ( cnt++ != 8 )                                     // Action every 8 calls
  {
    return ;
  }
  cnt = 0 ;
  if ( ! isvolume )                                     // Encoder menu mode?
  {
    dline[1].str = LCD2004_tftdata[3].str.substring(0,dsp_getwidth()) ;     // Yes, different lines
    dline[2].str = LCD2004_tftdata[3].str.substring(dsp_getwidth()) ;
  }
  else
  {
    dline[2].str = LCD2004_tftdata[1].str ;               // Local copy
    dline[1].str = LCD2004_tftdata[2].str ;               // Local copy
  }
  for ( int lnr = 1 ; lnr < 3 ; lnr++ )                   // Prepare lines
  {
    dline[lnr].str.trim() ;                               // Remove non printing
    if ( dline[lnr].str.length() > dsp_getwidth() )       // Will this be a scrolling text?
    {
      dline[lnr].str += String ( "  " ) ;                 // Yes, 2 spaces after scrolling
    }
    LCD2004_dsp_update_line ( lnr ) ;
  }
}


//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void LCD2004_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval )
{
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Display volume for this type of display.                                                        *
//**************************************************************************************************
void LCD2004_displayvolume ( uint8_t vol )
{
  static uint8_t   oldvol = 0 ;                       // Previous volume
  uint16_t         pos ;                              // Positon of volume indicator

  dline[3].str = "";

  if ( vol != oldvol )                                // Volume changed?
  {
    dbgprint ( "Update volume to %d", vol ) ;
    oldvol = vol ;                                    // Remember for next compare
    pos = map ( vol, 0, 100, 0, dsp_getwidth() ) ;    // Compute end position on TFT
    for ( int i = 0 ; i < dsp_getwidth() ; i++ )      // Set oldstr to dots
    {
      if ( i <= pos )
      {
        dline[3].str += '\xFF' ;                     // Add block character
      }
      else
      {
        dline[3].str += ' ' ;                         // Or blank sign
      }
    }
    LCD2004_dsp_update_line(3) ;
  }
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Display date and time to LCD line 0.                                                            *
//**************************************************************************************************
void LCD2004_displaytime ( const char* str, uint16_t color )
{
  const char* WDAYS [] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" } ;
  char        datetxt[24] ;
  static char oldstr = '\0' ;                            // To check time difference

  if ( ( str == NULL ) || ( str[0] == '\0' ) )           // Check time string
  {
    return ;                                             // Not okay, return
  }
  else
  {
    if ( str[7] == oldstr )                              // Difference?
    {
      return ;                                           // No, quick return
    }
    sprintf ( datetxt, "%s %02d.%02d.  %s",              // Format new time to a string
                       WDAYS[timeinfo.tm_wday],
                       timeinfo.tm_mday,
                       timeinfo.tm_mon + 1,
                       str ) ;
  }
  dline[0].str = String ( datetxt ) ;                    // Copy datestring or empty string to LCD line 0   
  oldstr = str[7] ;                                      // For next compare, last digit of time
  LCD2004_dsp_update_line ( 0 ) ;
}
