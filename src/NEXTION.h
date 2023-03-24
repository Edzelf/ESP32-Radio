// NEXTION.h
// Includes for NEXTION display
//

// Color definitions for the TFT screen (if used)
#define BLACK   0
#define BLUE    1
#define RED     1
#define GREEN   1
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()                                          // Use standard landscape format
#define dsp_print(a)                                               // Print a string 
#define dsp_fillRect(a,b,c,d,e)                                    // Fill a rectange
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()                                                // Erase screen
#define dsp_getwidth()      320                                    // Get width of screen
#define dsp_getheight()     240                                    // Get height of screen
#define dsp_usesSPI()       false                                  // Does not use SPI

void* tft = (void*)1 ;                                             // Dummy declaration

// Data to display.  There are TFTSECS sections
#define TFTSECS 4
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
  { false, CYAN,   20, 64, "" },                            // 8 lines in the middle
  { false, YELLOW, 90, 32, "" },                            // 4 lines at the bottom
  { false, GREEN,  90, 32, "" }                             // 4 lines at the bottom for rotary encoder
} ;

uint8_t pagenr = 0 ;

void nextioncmd ( const char *cmd )
{
  dbgprint ( "Nextion command %s", cmd ) ;
  nxtserial->printf ( "%s\xFF\xFF\xFF", cmd ) ;
}


bool dsp_begin()
{
  dbgprint ( "Init Nextion, pins 16,17" ) ;
  nxtserial = new HardwareSerial ( 2 ) ;
  displaytype = T_NEXTION ;                                 // Set displaytype
  reservepin ( 16 ) ;                                       // Reserve RX pin of serial port
  reservepin ( 17 ) ;                                       // Reserve TX pin of serial port
  nxtserial->begin ( 9600 ) ;                               // Initialize serial port
  nextioncmd ( "cls BLACK" ) ;                              // Erase screen, non vital
  nextioncmd ( "page 0" ) ;                                 // Select page 0
  return true ;
}


void dsp_println ( const char* str )
{
  // Print texts like t1.txt="abc"
  static uint16_t lnnr = 0 ;

  if ( pagenr == 0 )
  {
    if ( str[0] == '\f' )                                   // Formfeed?
    {
      pagenr = 1 ;                                          // Select next page
      nextioncmd ( "page 1" ) ;                             // Select page 1
    }
    else
    {
      lnnr = ( lnnr % 10 ) + 1 ;
      nxtserial->printf ( "t%d.txt=\"%s\"\xFF\xFF\xFF", lnnr, str ) ;
    }
  }
}


void dsp_update()                                            // Updates to the physical screen
{
  uint16_t inx ;

  if ( pagenr == 1 )
  {
    for ( inx = 0 ; inx < TFTSECS ; inx++ )
    {
      if ( tftdata[inx].update_req )
      {
        dbgprint ( "Nextion t%d.txt=\"%s\"", inx, tftdata[inx].str.c_str() ) ; 
        nxtserial->printf ( "t%d.txt=\"%s\"\xFF\xFF\xFF", inx, tftdata[inx].str.c_str() ) ;
        tftdata[inx].update_req = false ;
      }
    }
  }
}


//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Show the current battery charge level on the screen.                                            *
// No action if bat0/bat100 not defined in the preferences.                                        *
//**************************************************************************************************
void displaybattery()
{
  if ( ini_block.bat0 < ini_block.bat100 )              // Levels set in preferences?
  {
    static uint16_t oldpos = 0 ;                        // Previous charge level
    uint16_t        ypos ;                              // Position on screen
    uint16_t        v ;                                 // Constrainted ADC value
    uint16_t        newpos ;                            // Current setting

    v = constrain ( adcval, ini_block.bat0,             // Prevent out of scale
                    ini_block.bat100 ) ;
    newpos = map ( v, ini_block.bat0,                   // Compute length of green bar
                   ini_block.bat100,
                   0, 100 ) ;
    if ( newpos != oldpos )                             // Value changed?
    {
      oldpos = newpos ;                                 // Remember for next compare
      nxtserial->printf ( "j0.val=%d\xFF\xFF\xFF",
                          newpos ) ;                    // Set value of progress bar

    }
  }
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Show the current volume as an indicator on the screen.                                          *
// The indicator is 2 pixels heigh.                                                                *
//**************************************************************************************************
void displayvolume()
{
  static uint8_t oldvol = 0 ;                         // Previous volume
  uint8_t        newvol ;                             // Current setting
  uint16_t       pos ;                                // Positon of volume indicator

  newvol = vs1053player->getVolume() ;                // Get current volume setting
  if ( newvol != oldvol )                             // Volume changed?
  {
    oldvol = newvol ;                                 // Remember for next compare
    nxtserial->printf ( "h0.val=%d\xFF\xFF\xFF",
                        newvol ) ;                    // Move the slider
  }
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Show the time on the screen.                                                                    *
//**************************************************************************************************
void displaytime ( const char* str, uint16_t color )
{
  static char oldtim = '.' ;                       // For compare

  if ( str[0] == '\0' )                            // Empty string?
  {
    oldtim = '.' ;                                 // Force change next call
    return ;                                       // No actual display yet
  }
  if ( str[7] != oldtim )                          // Difference?
  {
    nxtserial->printf ( "tm.txt=\"%s\"\xFF\xFF\xFF", str ) ;
    oldtim = str[7] ;                              // Remember for next compare
  }
}

