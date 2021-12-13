//***************************************************************************************************
//*  LCD2004.h -- Driver for LCD 2004 display with I2C backpack.                                    *
//***************************************************************************************************
// The backpack communicates with the I2C bus and converts the serial data to parallel for the      *
// 2004 board.  In the serial data, the 8 bits are assigned as follows:                             *
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

#include <driver/i2c.h>

#define I2C_ADDRESS 0x27                                     // Adjust for your display
#define ACKENA      true                                     // Enable ACK for I2C communication
// Color definitions for the TFT screen (if used)
#define BLACK   0
#define BLUE    1
#define RED     1
#define GREEN   0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE

#define DELAY_ENABLE_PULSE_SETTLE    50      // Command requires > 37us to settle
#define FLAG_BACKLIGHT_ON    0b00001000      // Bit 3, backlight enabled (disabled if clear)
#define FLAG_ENABLE          0b00000100      // Bit 2, Enable
#define FLAG_RS_DATA         0b00000001      // Bit 0, RS=data (command if clear)
#define FLAG_RS_COMMAND      0b00000000      // Command

#define COMMAND_CLEAR_DISPLAY               0x01
#define COMMAND_RETURN_HOME                 0x02
#define COMMAND_ENTRY_MODE_SET              0x04
#define COMMAND_DISPLAY_CONTROL             0x08
#define COMMAND_FUNCTION_SET                0x20
#define COMMAND_SET_DDRAM_ADDR              0x80
//
#define FLAG_DISPLAY_CONTROL_DISPLAY_ON     0x04
#define FLAG_DISPLAY_CONTROL_CURSOR_ON      0x02
//
#define FLAG_FUNCTION_SET_MODE_4BIT         0x00
#define FLAG_FUNCTION_SET_LINES_2           0x08
#define FLAG_FUNCTION_SET_DOTS_5X8          0x00
//
#define FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT 0x02
#define FLAG_ENTRY_MODE_SET_ENTRY_SHIFT_ON  0x01
//
// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()                                          // Use standard landscape format
#define dsp_print(a)                                               // Print a string 
#define dsp_println(a)                                             // Print string plus newline
#define dsp_fillRect(a,b,c,d,e)                                    // Fill a rectange
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()         tft->sclear()                          // Clear the screen
#define dsp_getwidth()      20                                     // Get width of screen
#define dsp_getheight()     4                                      // Get height of screen
#define dsp_usesSPI()       false                                  // Does not use SPI

#define TFTSECS 4                                           // 4 sections, only 2 used
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 4 segments
{
  { false, WHITE,   0,  0, "" },                            // 1 top line (dummy)
  { false, WHITE,   0,  0, "" },                            // 8 lines in the middle
  { false, WHITE,   0,  0, "" },                            // 4 lines at the bottom
  { false, WHITE,   0,  0, "" }                             // 4 lines at the bottom for rotary encoder
} ;

class LCD2004
{
  public:
                     LCD2004 ( uint8_t sda, uint8_t scl ) ; // Constructor
    void             print ( char c ) ;                     // Send 1 char
    void             reset() ;                              // Perform reset
    void             sclear() ;                             // Clear the screen
    void             shome() ;                              // Go to home position
    void             scursor ( uint8_t col, uint8_t row ) ; // Position the cursor
    void             scroll ( bool son ) ;                  // Set scroll on/off
  private:
    i2c_config_t     i2c_config ;                           // I2C configuration
    i2c_cmd_handle_t hnd ;                                  // Handle for driver
    void             scommand ( uint8_t cmd ) ;
    void             strobe ( uint8_t cmd ) ;
    void             swrite ( uint8_t val, uint8_t rs ) ;
    void             write_cmd ( uint8_t val ) ;
    void             write_data ( uint8_t val ) ;
    uint8_t          bl  = FLAG_BACKLIGHT_ON ;              // Backlight in every command
    uint8_t          xchar = 0 ;                            // Current cursor position (text)
    uint8_t          ychar = 0 ;                            // Current cursor position (text)
} ;

LCD2004* tft = NULL ;

bool dsp_begin()
{
  dbgprint ( "Init LCD2004, I2C pins %d,%d", ini_block.tft_sda_pin,
             ini_block.tft_scl_pin ) ;
  if ( ( ini_block.tft_sda_pin >= 0 ) &&
       ( ini_block.tft_scl_pin >= 0 ) )
  {
    tft = new LCD2004 ( ini_block.tft_sda_pin,
                        ini_block.tft_scl_pin ) ;           // Create an instance for TFT
  }
  else
  {
    dbgprint ( "Init LCD2004 failed!" ) ;
  }
  return ( tft != NULL ) ;
}


//***********************************************************************************************
//                                L C D 2 0 0 4  write functions                                *
//***********************************************************************************************
// Write functins for command, data and general.                                                *
//***********************************************************************************************
void LCD2004::swrite ( uint8_t val, uint8_t rs )          // General write, 8 bits data
{
  strobe ( ( val & 0xf0 ) | rs ) ;                        // Send 4 LSB bits
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
  hnd = i2c_cmd_link_create() ;                           // Create a link
  if ( i2c_master_start ( hnd ) |
       i2c_master_write_byte ( hnd, (I2C_ADDRESS << 1) |  // Add I2C address to output buffer
                                    I2C_MASTER_WRITE,
                               ACKENA ) |
       i2c_master_write_byte ( hnd, cmd | bl,             // Add command including BL state
                               ACKENA ) |
       i2c_master_stop ( hnd ) |                          // End of data for I2C
       i2c_master_cmd_begin ( I2C_NUM_0, hnd,             // Send bufferd data to LCD
                              10 / portTICK_PERIOD_MS ) )
  {
    dbgprint ( "LCD2004 communication error!" ) ;         // Something went wrong (not connected)
  }
  i2c_cmd_link_delete ( hnd ) ;                           // Link not needed anymore
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


//***********************************************************************************************
//                                L C D 2 0 0 4                                                 *
//***********************************************************************************************
// Constructor for the display.                                                                 *
//***********************************************************************************************
LCD2004::LCD2004 ( uint8_t sda, uint8_t scl )
{
  esp_err_t        espRc ;

  i2c_config.mode = I2C_MODE_MASTER,
  i2c_config.sda_io_num = (gpio_num_t)sda ;
  i2c_config.sda_pullup_en = GPIO_PULLUP_DISABLE ;
  i2c_config.scl_io_num = (gpio_num_t)scl ;
  i2c_config.scl_pullup_en = GPIO_PULLUP_DISABLE ;
  i2c_config.master.clk_speed = 100000 ;
  if ( i2c_param_config ( I2C_NUM_0, &i2c_config ) != ESP_OK )
  {
    dbgprint ( "param_config error!" ) ;
  }
  else if ( i2c_driver_install ( I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0 ) != ESP_OK )
  {
    dbgprint ( "driver_install error!" ) ;
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
//***********************************************************************************************
void dsp_update_line ( uint8_t lnr )
{
  uint8_t         i ;                                // Index in string
  const char*     p ;

  p = dline[lnr].str.c_str() ;
  dline[lnr].len = strlen ( p ) ;
  //dbgprint ( "Strlen is %d, str is %s", len, p ) ;
  if ( dline[lnr].len > dsp_getwidth() )
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
    if ( dline[lnr].len > dsp_getwidth() )
    {
      dline[lnr].len = dsp_getwidth() ;
    }
  }
  else
  {
    dline[lnr].pos = 0 ;                             // String fits on screen
  }
  dline[lnr].pos++ ;
  tft->scursor ( 0, lnr ) ;
  for ( i = 0 ; i < dline[lnr].len ; i++ )
  {
    if ( ( *p >= ' ' ) && ( *p <= '~' ) )            // Printable?
    {
      tft->print ( *p ) ;                            // Yes
    }
    else
    {
      tft->print ( ' ' ) ;                           // Yes, print space
    }
    p++ ;
  }
  for ( i = 0 ; i < ( dsp_getwidth() - dline[lnr].len ) ; i++ )  // Fill remainder
  {
    tft->print ( ' ' ) ;
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
void dsp_update()
{
  static uint16_t cnt = 0 ;                             // Reduce updates

  if ( cnt++ != 8 )                                     // Action every 8 calls
  {
    return ;
  }
  cnt = 0 ;
  if ( enc_menu_mode != VOLUME )                        // Encoder menu mode?
  {
    dline[1].str = tftdata[3].str.substring(0,dsp_getwidth()) ;     // Yes, different lines
    dline[2].str = tftdata[3].str.substring(dsp_getwidth()) ;
  }
  else
  {
    dline[2].str = tftdata[1].str ;                     // Local copy
    dline[1].str = tftdata[2].str ;                     // Local copy
  }  
  dline[2].str.trim() ;                                 // Remove non printing
  dline[1].str.trim() ;                                 // Remove non printing
  if ( dline[2].str.length() > dsp_getwidth() )
  {
    dline[2].str += String ( "  " ) ;
  }
  if ( dline[1].str.length() > dsp_getwidth() )
  {
    dline[1].str += String ( "  " ) ;
  }
  dsp_update_line ( 1 ) ;
  dsp_update_line ( 2 ) ;
}


//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void displaybattery()
{
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Display volume for this type of display.                                                        *
//**************************************************************************************************
void displayvolume()
{
  static uint8_t   oldvol = 0 ;                       // Previous volume
  uint8_t          newvol ;                           // Current setting
  uint16_t         pos ;                              // Positon of volume indicator

  dline[3].str = "";

  newvol = vs1053player->getVolume() ;                // Get current volume setting
  if ( newvol != oldvol )                             // Volume changed?
  {
    oldvol = newvol ;                                 // Remember for next compare
    pos = map ( newvol, 0, 100, 0, dsp_getwidth() ) ; // Compute end position on TFT
    for ( int i = 0 ; i < dsp_getwidth() ; i++ )      // Set oldstr to dots
    {
      if ( i <= pos )
      {
        dline[3].str += "\x7F" ;                      // Add block character
      }
      else
      {
        dline[3].str += " " ;                         // Or blank sign
      }
    }
    dsp_update_line(3) ;
  }
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Display date and time to LCD line 0.                                                            *
//**************************************************************************************************
void displaytime ( const char* str, uint16_t color )
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
  dsp_update_line ( 0 ) ;
}
