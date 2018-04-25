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
#define dsp_fillRect(a,b,c,d,e)                                    // Fill a rectange
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)                              
#define dsp_setCursor(a,b)                                         // Position the cursor
#define dsp_erase()                                                // Clear the screen
#define dsp_getwidth()      0                                      // Get width of screen
#define dsp_getheight()     0                                      // Get height of screen
#define dsp_update()                                               // Updates to the physical screen

bool dsp_begin()
{
  return false ;
}

void* tft = NULL ;

#define TFTSECS 1
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   0,  8, "" },                            // 1 top line
} ;


