//**************************************************************************************************
// NEXTION.cpp                                                                                     *
//**************************************************************************************************
// Driver for NEXTION display.                                                                     *
//**************************************************************************************************
#include "NEXTION.h"

char*       dbgprint ( const char* format, ... ) ;          // Print a formatted debug line
char        strbuf[40] ;                                    // Buffer for incomplete line

HardwareSerial*   nxtserial = NULL ;                        // Serial port for NEXTION
void*             NEXTION_tft = (void*)1 ;                  // Dummy declaration

// Data to display.  There are TFTSECS sections
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
  { false, CYAN,   20, 64, "" },                            // line in the middle
  { false, YELLOW, 90, 32, "" },                            // line at the bottom
  { false, GREEN,  90, 32, "" }                             // line at the bottom for rotary encoder
} ;

uint8_t pagenr = 0 ;

void NEXTION_nextioncmd ( const char *cmd )
{
  dbgprint ( "Nextion command %s", cmd ) ;
  nxtserial->printf ( "%s\xFF\xFF\xFF", cmd ) ;
}


bool NEXTION_dsp_begin ( int8_t rx, int8_t tx )
{
  dbgprint ( "Init Nextion, pins %d, %d", rx, tx ) ;
  nxtserial = new HardwareSerial ( 2 ) ;
  nxtserial->begin ( 115200, SERIAL_8N1, rx, tx ) ;         // Initialize serial port
  NEXTION_nextioncmd ( "cls BLACK" ) ;                      // Erase screen, non vital
  NEXTION_nextioncmd ( "page 0" ) ;                         // Select page 0
  strbuf[0] = '\0' ;                                        // Empty string buffer
  return true ;
}


void NEXTION_dsp_println ( const char* str )
{
  // Print texts like t1.txt="abc"
  static uint16_t lnnr = 0 ;

  if ( pagenr == 0 )
  {
    if ( str[0] == '\f' )                                   // Formfeed?
    {
      pagenr = 1 ;                                          // Select next page
      NEXTION_nextioncmd ( "page 1" ) ;                     // Select page 1
    }
    else
    {
      lnnr = ( lnnr % 10 ) + 1 ;
      dbgprint ( "NEXTION output '%s' to %d",
                 str, lnnr ) ;
      nxtserial->printf ( "t%d.txt=\"%s\"\xFF\xFF\xFF", lnnr, str ) ;
    }
  }
}


void NEXTION_dsp_print ( const char* str )
{
  // Print texts like t1.txt="abc"
  if ( str[0] != '\n' )                                     // End of string?
  {                                                         // No, add to string
    int totlen =  strlen ( strbuf ) + strlen (str ) ;       // Will be total length
    if ( totlen < sizeof(strbuf )  )                        // Protect against overflow
    {
      strcat ( strbuf, str ) ;                              // Add to string in buffer
    }
  }
  else
  {
    dsp_println ( strbuf ) ;                                // End of line, print buffer
    strbuf[0] = '\0' ;                                      // Emty buffer for next line
  }
}


void NEXTION_dsp_update ( bool a )                          // Updates to the physical screen
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
// Set to 0 if bat0/bat100 not defined in the preferences.                                         *
//**************************************************************************************************
void NEXTION_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval )
{
  static uint16_t oldpos = 60000 ;                      // Previous charge level
  uint16_t        v ;                                   // Constrainted ADC value
  uint16_t        newpos ;                              // Current setting

  if ( bat0 >= bat100 )                                 // Levels set in preferences?
  {
    newpos = 0 ;                                        // No, force to zero
  }
  else
  {
    v = constrain ( adcval, bat0, bat100 ) ;            // Prevent out of scale
    newpos = map ( v, bat0, bat100,                     // Compute length of green bar
                   0, 100 ) ;
  }
  if ( newpos != oldpos )                               // Value changed?
  {
    oldpos = newpos ;                                   // Remember for next compare
    nxtserial->printf ( "j0.val=%d\xFF\xFF\xFF",
                        newpos ) ;                      // Set value of progress bar
  }
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Show the current volume as an indicator on the screen.                                          *
// The indicator is 2 pixels heigh.                                                                *
//**************************************************************************************************
void NEXTION_displayvolume (  uint8_t vol )
{
  static uint8_t oldvol = 0 ;                         // Previous volume

  if ( vol != oldvol )                                // Volume changed?
  {
    oldvol = vol ;                                    // Remember for next compare
    nxtserial->printf ( "h0.val=%d\xFF\xFF\xFF",
                        vol ) ;                       // Move the slider
  }
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Show the time on the screen.                                                                    *
//**************************************************************************************************
void NEXTION_displaytime ( const char* str, uint16_t color )
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

