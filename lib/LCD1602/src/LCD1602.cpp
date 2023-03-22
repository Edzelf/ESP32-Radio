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

#include <Wire.h>
#include "LCD1602.h"

char*       dbgprint ( const char* format, ... ) ;          // Print a formatted debug line

scrseg_struct     LCD1602_tftdata[TFTSECS] =                // Screen divided in 4 segments
                      {
                        { false, WHITE,   0,  0, "" },      // 1 top line (dummy)
                        { false, WHITE,   0,  0, "" },      // 8 lines in the middle
                        { false, WHITE,   0,  0, "" },      // 4 lines at the bottom
                        { false, WHITE,   0,  0, "" }       // 4 lines at the bottom for rotary encoder
                      } ;


LCD1602* LCD1602_tft = NULL ;

//***********************************************************************************************
//                                L C D 1 6 0 2  write functions                                *
//***********************************************************************************************
// Write functins for command, data and general.                                                *
//***********************************************************************************************
void LCD1602::swrite ( uint8_t val, uint8_t rs )          // General write, 8 bits data
{
  strobe ( ( val & 0xf0 ) | rs ) ;                        // Send 4 LSB bits
  strobe ( ( val << 4 ) | rs ) ;                          // Send 4 MSB bits
}


void LCD1602::write_data ( uint8_t val )
{
  swrite ( val, FLAG_RS_DATA ) ;                           // Send data (RS = HIGH)
}


void LCD1602::write_cmd ( uint8_t val )
{
  swrite ( val, FLAG_RS_COMMAND ) ;                        // Send command (RS = LOW)
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: S T R O B E                                  *
//***********************************************************************************************
// Send data followed by strobe to clock data to LCD.                                           *
//***********************************************************************************************
void LCD1602::strobe ( uint8_t cmd )
{
  scommand ( cmd | FLAG_ENABLE ) ;                  // Send command with E high
  scommand ( cmd ) ;                                // Same command with E low
  delayMicroseconds ( DELAY_ENABLE_PULSE_SETTLE ) ; // Wait a short time
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: S C O M M A N D                              *
//***********************************************************************************************
// Send a command to the LCD.                                                                   *
// Actual I/O.  Open a channel to the I2C interface and write one byte.                         *
//***********************************************************************************************
void LCD1602::scommand ( uint8_t cmd )
{
  Wire.beginTransmission ( LCD_I2C_ADDRESS ) ;
  Wire.write ( cmd | bl ) ;                               // Add command including BL state
  Wire.endTransmission() ;
}



//***********************************************************************************************
//                                L C D 1 6 0 2 :: P R I N T                                    *
//***********************************************************************************************
// Put a character in the buffer.                                                               *
//***********************************************************************************************
void LCD1602::print ( char c )
{
  write_data ( c ) ;
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: S C U R S O R                                *
//***********************************************************************************************
// Place the cursor at the requested position.                                                  *
//***********************************************************************************************
void LCD1602::scursor ( uint8_t col, uint8_t row )
{
  const int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 } ;
  
  write_cmd ( COMMAND_SET_DDRAM_ADDR |
              ( col + row_offsets[row] ) ) ; 
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: S C L E A R                                  *
//***********************************************************************************************
// Clear the LCD.                                                                               *
//***********************************************************************************************
void LCD1602::sclear()
{
  write_cmd ( COMMAND_CLEAR_DISPLAY ) ;
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: S C R O L L                                  *
//***********************************************************************************************
// Set scrolling on/off.                                                                        *
//***********************************************************************************************
void LCD1602::scroll ( bool son )
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
//                                L C D 1 6 0 2 :: S H O M E                                    *
//***********************************************************************************************
// Go to home position.                                                                         *
//***********************************************************************************************
void LCD1602::shome()
{
  write_cmd ( COMMAND_RETURN_HOME ) ;
}


//***********************************************************************************************
//                                L C D 1 6 0 2 :: R E S E T                                    *
//***********************************************************************************************
// Reset the LCD.                                                                               *
//***********************************************************************************************
void LCD1602::reset()
{
  scommand ( 0 ) ;                                // Put expander to known state
  delayMicroseconds ( 1000 ) ;
  for ( int i = 0 ; i < 3 ; i++ )                 // Repeat 3 times
  {
    strobe ( 0x03 << 4 ) ;                        // Select 4-bit mode
    delayMicroseconds ( 4500 ) ;
  }
  strobe ( 0x02 << 4 ) ;                          // 4-bit
  delayMicroseconds ( 4500 ) ;
  write_cmd ( COMMAND_FUNCTION_SET |
              FLAG_FUNCTION_SET_MODE_4BIT |
              FLAG_FUNCTION_SET_LINES_2 |
              FLAG_FUNCTION_SET_DOTS_5X8 ) ;
  write_cmd ( COMMAND_DISPLAY_CONTROL |
              FLAG_DISPLAY_CONTROL_DISPLAY_ON ) ;
  sclear() ;
  write_cmd ( COMMAND_ENTRY_MODE_SET |
              FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT ) ;
  shome() ;
  for ( char a = 'a' ; a < 'q' ; a++ )
  {
    print ( a ) ;
  }
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
//                                L C D 1 6 0 2                                                 *
//***********************************************************************************************
// Constructor for the display.                                                                 *
//***********************************************************************************************
LCD1602::LCD1602 ( int8_t sda, int8_t scl )
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

dsp_str dline[2] = { { "", 0, 0, 0 },
                     { "", 0, 0, 0 }
                   } ;

bool LCD1602_dsp_begin ( int8_t sda, int8_t scl )
{
  dbgprint ( "Init LCD1602, I2C pins %d,%d", sda, scl ) ;
  if ( ( sda >= 0 ) && ( scl >= 0 ) )
  {
    LCD1602_tft = new LCD1602 ( sda, scl ) ;            // Create an instance for TFT
  }
  else
  {
    dbgprint ( "Init LCD1602 failed!" ) ;
  }
  return ( LCD1602_tft != NULL ) ;
}


//***********************************************************************************************
//                                D S P _U P D A T E _ L I N E                                  *
//***********************************************************************************************
// Show a selected line                                                                         *
//***********************************************************************************************
void LCD1602_dsp_update_line ( uint8_t lnr )
{
  uint8_t         i ;                                // Index in string
  const char*     p ;

  p = dline[lnr].str.c_str() ;
  dline[lnr].len = strlen ( p ) ;
  //dbgprint ( "Strlen is %d, str is %s", len, p ) ;
  if ( dline[lnr].len > 16 )
  {
    if ( dline[lnr].pos >= dline[lnr].len )
    {
      dline[lnr].pos = 0 ;
    }
    else
    {
      p += dline[lnr].pos ;
    }
    dline[lnr].len -= dline[lnr].pos ;
    if ( dline[lnr].len > 16 )
    {
      dline[lnr].len = 16 ;
    }
  }
  else
  {
    dline[lnr].pos = 0 ;                             // String fits on screen
  }
  dline[lnr].pos++ ;
  LCD1602_tft->scursor ( 0, lnr ) ;
  for ( i = 0 ; i < dline[lnr].len ; i++ )
  {
    if ( ( *p >= ' ' ) && ( *p <= '~' ) )            // Printable?
    {
      LCD1602_tft->print ( *p ) ;                    // Yes
    }
    else
    {
      LCD1602_tft->print ( ' ' ) ;                   // Yes, print space
    }
    p++ ;
  }
  for ( i = 0 ; i < ( 16 - dline[lnr].len ) ; i++ )  // Fill remainder
  {
    LCD1602_tft->print ( ' ' ) ;
  }
  if ( *p == '\0' )                                  // At end of line?
  {
    dline[lnr].pos = 0 ;                             // Yes, start allover
  }
}


//***********************************************************************************************
//                                D S P _U P D A T E                                            *
//***********************************************************************************************
// Show a selection of the 4 sections                                                           *
//***********************************************************************************************
void LCD1602_dsp_update ( bool isvolume )
{
  static uint16_t cnt = 0 ;                                 // Reduce updates

  if ( cnt++ != 8 )                                         // Action every 8 calls
  {
    return ;
  }
  cnt = 0 ;
  if ( ! isvolume )                                         // Encoder menu mode?
  {
    dline[0].str = LCD1602_tftdata[3].str.substring(0,16) ; // Yes, different lines
    dline[1].str = LCD1602_tftdata[3].str.substring(16) ;
  }
  else
  {
    dline[0].str = LCD1602_tftdata[1].str ;                 // Local copy
    dline[1].str = LCD1602_tftdata[2].str ;                 // Local copy
  }  
  dline[0].str.trim() ;                                     // Remove non printing
  dline[1].str.trim() ;                                     // Remove non printing
  if ( dline[0].str.length() > 16 )
  {
    dline[0].str += String ( "  " ) ;
  }
  if ( dline[1].str.length() > 16 )
  {
    dline[1].str += String ( "  " ) ;
  }
  LCD1602_dsp_update_line ( 0 ) ;
  LCD1602_dsp_update_line ( 1 ) ;
}


//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void LCD1602_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval )
{
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void LCD1602_displayvolume ( uint8_t vol )
{
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void LCD1602_displaytime ( const char* str, uint16_t color )
{
}
