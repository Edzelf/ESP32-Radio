// Dummytft.h
// Includes for dummy display

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
#define dsp_println(a)                                             // Print string plus newline
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()                                                // Clear the screen
#define dsp_getwidth()      0                                      // Get width of screen
#define dsp_getheight()     0                                      // Get height of screen
#define dsp_update()                                               // Updates to the physical screen
#define dsp_usesSPI()       false                                  // Does not use SPI

void* tft = NULL ;

bool dsp_begin()
{
  return false ;
}

void dsp_fillRect ( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color )
{} ;                                                        // Fill a rectange


#define TFTSECS 1
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
} ;

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
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void displayvolume()
{
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Dummy routine for this type of display.                                                         *
//**************************************************************************************************
void displaytime ( const char* str, uint16_t color )
{
}


