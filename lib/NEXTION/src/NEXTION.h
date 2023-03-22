//**************************************************************************************************
// NEXTION.h                                                                                       *
//**************************************************************************************************
// Includes for NEXTION display.                                                                   *
//**************************************************************************************************
#ifndef NEXTION_H
#define NEXTION_H
#include <Arduino.h>

#define TIMEPOS     -52                         // Position (column) of time in topline relative to end
#define INIPARS     ini_block.nxt_rx_pin, ini_block.nxt_tx_pin  // Par. for dsp_begin (Serial RX and TX)
#define DISPLAYTYPE "NEXTION"

// Color definitions for the TFT screen (if used)
#define BLACK   0
#define BLUE    1
#define RED     1
#define GREEN   1
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE

#define DEFTXTSIZ   1                                 // Default text size


struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;

#define tftdata             NEXTION_tftdata
#define displaybattery      NEXTION_displaybattery
#define displayvolume       NEXTION_displayvolume
#define displaytime         NEXTION_displaytime

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()                                          // Use standard landscape format
#define dsp_print(a)        NEXTION_dsp_print(a)                   // Print a string
#define dsp_println(b)      NEXTION_dsp_println(b)                 // Print a string followed by newline 
#define dsp_fillRect(a,b,c,d,e)                                    // Fill a rectange
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()                                                // Erase screen
#define dsp_getwidth()      320                                    // Get width of screen
#define dsp_getheight()     240                                    // Get height of screen
#define dsp_update(a)       NEXTION_dsp_update(a)                  // Updates to the physical screen
#define dsp_usesSPI()       false                                  // Does not use SPI
#define dsp_begin           NEXTION_dsp_begin                      // Init driver


extern void*            NEXTION_tft ;                              // Dummy declaration
extern HardwareSerial*  nxtserial ;                                // Serial port for NEXTION

// Data to display.  There are TFTSECS sections
#define TFTSECS 4

extern scrseg_struct     tftdata[TFTSECS] ;                 // Screen divided in 3 segments + 1 overlay

void NEXTION_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval ) ;
void NEXTION_displayvolume  ( uint8_t vol ) ;
void NEXTION_displaytime    ( const char* str, uint16_t color = 0xFFFF ) ;
bool NEXTION_dsp_begin      ( int8_t rx, int8_t tx ) ;
void NEXTION_dsp_println    ( const char* str ) ;
void NEXTION_dsp_print      ( const char* str ) ;
void NEXTION_dsp_update     ( bool a ) ;                    // Updates to the physical screen


#endif