//***************************************************************************************************
//                                   O L E D . H                                                    *
//***************************************************************************************************
// Header for driver for SSD1306/SSD1309/SH1106 display                                             *
//***************************************************************************************************
// 25-02-2021, ES: Correct bug, isSH1106 was always set.                                            *
//***************************************************************************************************
#ifndef OLED_H
#define OLED_H
#include <Arduino.h>
#include <string.h>

#ifdef OLED1309                           // Set type acoording to definition
  #define OLEDTYP 1309
#endif
#ifdef OLED1106
  #define OLEDTYP 1106
#endif
#ifdef OLED1306
  #define OLEDTYP 1306
#endif


#define OLED_I2C_ADDRESS   0x3C
#define SCREEN_WIDTH        128                    // OLED display width, in pixels
#define SCREEN_HEIGHT        64                    // OLED display height, in pixels
#define DISPLAYTYPE      "OLED"
#define INIPARS     ini_block.tft_sda_pin, ini_block.tft_scl_pin, OLEDTYP  // Parameters for dsp_begin
#define TIMEPOS             -52                    // Position (column) of time in topline relative to end

// Color definitions.  OLED only have 2 colors.
#define BLACK   0
#define BLUE    0xFF
#define RED     0xFF
#define GREEN   0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   RED | GREEN | BLUE

#define DEFTXTSIZ   1                              // Default text size

#define OLED_NPAG    8                             // Number of pages (text lines)

// Data to display.  There are TFTSECS sections
struct page_struct
{
  uint8_t   page[128] ;                            // Buffer for one page (8 pixels heigh)
  bool      dirty ;                                // True if modified
} ;

struct scrseg_struct                               // For screen segments
{
  bool     update_req ;                            // Request update of screen
  uint16_t color ;                                 // Textcolor
  uint16_t y ;                                     // Begin of segment row
  uint16_t height ;                                // Height of segment
  String   str ;                                   // String to be displayed
} ;

class OLED
{
  public:
    OLED    ( int8_t sda, int8_t scl ) ;              // Constructor
    void      clear() ;                               // Clear buffer
    void      display() ;                             // Display buffer
    void      print ( char c ) ;                      // Print a character
    void      print ( const char* str ) ;             // Print a string
    void      setCursor ( uint8_t x, uint8_t y ) ;    // Position the cursor
    void      fillRect ( uint8_t x, uint8_t y,        // Fill a rectangle
                         uint8_t w, uint8_t h,
                         uint8_t color ) ;
  private:
    struct page_struct*     ssdbuf = NULL ;
    const  uint8_t*         font ;                    // Font to use
    uint8_t                 xchar = 0 ;               // Current cursor position (text)
    uint8_t                 ychar = 0 ;               // Current cursor position (text)
    void                    openI2Cchan() ;           // Open I2C channel
    void                    closeI2Cchan() ;          // Close I2C channel
    void                    wrI2Cchan ( uint8_t b ) ; // Send 1 byte to I2C buffer
} ;

#define TFTSECS   4
#define tftdata   OLED_tftdata

extern OLED*         tft ;                            // Object for display
extern scrseg_struct OLED_tftdata[TFTSECS] ;          // Screen divided in segments

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation()                                          // Use standard landscape format
#define dsp_setTextSize(a)                                         // Set the text size
#define dsp_setTextColor(a)
#define dsp_print(a)            tft->print ( a )                   // Print a string 
#define dsp_println(a)          tft->print ( a ) ; \
                                tft->print ( '\n' )                // Print string plus newline
#define dsp_setCursor(a,b)      tft->setCursor ( a, b )            // Position the cursor
#define dsp_fillRect(a,b,c,d,e) tft->fillRect ( a, b, c, d, e )    // Fill a rectangle
#define dsp_erase()             tft->clear()                       // Clear the screen
#define dsp_getwidth()          128                                // Get width of screen
#define dsp_getheight()         64                                 // Get height of screen
#define dsp_update(a)           tft->display()                     // Updates to the physical screen
#define dsp_usesSPI()           false                              // Does not use SPI
#define dsp_begin               oled_dsp_begin                     // Init driver
#define displayvolume           oled_displayvolume
#define displaybattery          oled_displaybattery
#define displaytime             oled_displaytime

void oled_displaybattery ( uint16_t bat0, uint16_t bat100, uint16_t adcval ) ;
void oled_displaytime ( const char* str, uint16_t color = 0xFFFF ) ;
bool oled_dsp_begin ( int8_t sda_pin, int8_t scl_pin, uint16_t olt ) ;
void oled_displayvolume  ( uint8_t vol ) ;

// Control byte
#define OLED_CONTROL_BYTE_CMD_SINGLE    0x80
#define OLED_CONTROL_BYTE_CMD_STREAM    0x00
#define OLED_CONTROL_BYTE_DATA_STREAM   0x40

// Fundamental commands (pg.28)
#define OLED_CMD_SET_CONTRAST           0x81    // follow with 0x7F
#define OLED_CMD_DISPLAY_RAM            0xA4
#define OLED_CMD_DISPLAY_ALLON          0xA5
#define OLED_CMD_DISPLAY_NORMAL         0xA6
#define OLED_CMD_DISPLAY_INVERTED       0xA7
#define OLED_CMD_DISPLAY_OFF            0xAE
#define OLED_CMD_DISPLAY_ON             0xAF
#define OLED_CMD_SCROLL_OFF             0x2E

// Addressing Command Table (pg.30)
#define OLED_CMD_SET_MEMORY_ADDR_MODE   0x20    // follow with 0x00 = HORZ mode = Behave like a KS108 graphic LCD
#define OLED_CMD_SET_COLUMN_RANGE       0x21    // can be used only in HORZ/VERT mode - follow with 0x00 and 0x7F = COL127
#define OLED_CMD_SET_PAGE_RANGE         0x22    // can be used only in HORZ/VERT mode - follow with 0x00 and 0x07 = PAGE7
#define OLED_CMD_SET_PAGE_START_ADDR    0xB0    // Set page start address for page addressing mode
// Hardware Config (pg.31)
#define OLED_CMD_SET_DISPLAY_START_LINE 0x40
#define OLED_CMD_SET_SEGMENT_REMAP      0xA1    
#define OLED_CMD_SET_MUX_RATIO          0xA8    // follow with 0x3F = 64 MUX
#define OLED_CMD_SET_COM_SCAN_MODE      0xC8    // Mirror Y
#define OLED_CMD_SET_DISPLAY_OFFSET     0xD3    // follow with 0x00
#define OLED_CMD_SET_COM_PIN_MAP        0xDA    // follow with 0x12
#define OLED_CMD_NOP                    0xE3    // NOP

// Timing and Driving Scheme (pg.32)
#define OLED_CMD_SET_DISPLAY_CLK_DIV    0xD5    // follow with 0x80
#define OLED_CMD_SET_PRECHARGE          0xD9    // follow with 0xF1
#define OLED_CMD_SET_VCOMH_DESELCT      0xDB    // follow with 0x30

// Charge Pump (pg.62)
#define OLED_CMD_SET_CHARGE_PUMP        0x8D    // follow with 0x14

const uint8_t OLEDFONTWIDTH = 5 ;
const uint8_t OLEDfont[] =
            {
              0x00, 0x00, 0x00, 0x00, 0x00, // SPACE
              0x00, 0x00, 0x5F, 0x00, 0x00, // !
              0x00, 0x03, 0x00, 0x03, 0x00, // "
              0x14, 0x3E, 0x14, 0x3E, 0x14, // #
              0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
              0x43, 0x33, 0x08, 0x66, 0x61, // %
              0x36, 0x49, 0x55, 0x22, 0x50, // &
              0x00, 0x05, 0x03, 0x00, 0x00, // '
              0x00, 0x1C, 0x22, 0x41, 0x00, // (
              0x00, 0x41, 0x22, 0x1C, 0x00, // )
              0x14, 0x08, 0x3E, 0x08, 0x14, // *
              0x08, 0x08, 0x3E, 0x08, 0x08, // +
              0x00, 0xA0, 0x60, 0x00, 0x00, // ,
              0x08, 0x08, 0x08, 0x08, 0x08, // -
              0x00, 0x60, 0x60, 0x00, 0x00, // .
              0x20, 0x10, 0x08, 0x04, 0x02, // /
            
              0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
              0x00, 0x04, 0x02, 0x7F, 0x00, // 1
              0x42, 0x61, 0x51, 0x49, 0x46, // 2
              0x22, 0x41, 0x49, 0x49, 0x36, // 3
              0x18, 0x14, 0x12, 0x7F, 0x10, // 4
              0x27, 0x45, 0x45, 0x45, 0x39, // 5
              0x3E, 0x49, 0x49, 0x49, 0x32, // 6
              0x01, 0x01, 0x71, 0x09, 0x07, // 7
              0x36, 0x49, 0x49, 0x49, 0x36, // 8
              0x26, 0x49, 0x49, 0x49, 0x3E, // 9
              0x00, 0x36, 0x36, 0x00, 0x00, // :
              0x00, 0x56, 0x36, 0x00, 0x00, // ;
              0x08, 0x14, 0x22, 0x41, 0x00, // <
              0x14, 0x14, 0x14, 0x14, 0x14, // =
              0x00, 0x41, 0x22, 0x14, 0x08, // >
              0x02, 0x01, 0x51, 0x09, 0x06, // ?
            
              0x3E, 0x41, 0x59, 0x55, 0x5E, // @
              0x7E, 0x09, 0x09, 0x09, 0x7E, // A
              0x7F, 0x49, 0x49, 0x49, 0x36, // B
              0x3E, 0x41, 0x41, 0x41, 0x22, // C
              0x7F, 0x41, 0x41, 0x41, 0x3E, // D
              0x7F, 0x49, 0x49, 0x49, 0x41, // E
              0x7F, 0x09, 0x09, 0x09, 0x01, // F
              0x3E, 0x41, 0x41, 0x49, 0x3A, // G
              0x7F, 0x08, 0x08, 0x08, 0x7F, // H
              0x00, 0x41, 0x7F, 0x41, 0x00, // I
              0x30, 0x40, 0x40, 0x40, 0x3F, // J
              0x7F, 0x08, 0x14, 0x22, 0x41, // K
              0x7F, 0x40, 0x40, 0x40, 0x40, // L
              0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
              0x7F, 0x02, 0x04, 0x08, 0x7F, // N
              0x3E, 0x41, 0x41, 0x41, 0x3E, // O
            
              0x7F, 0x09, 0x09, 0x09, 0x06, // P
              0x1E, 0x21, 0x21, 0x21, 0x5E, // Q
              0x7F, 0x09, 0x09, 0x09, 0x76, // R
              0x26, 0x49, 0x49, 0x49, 0x32, // S
              0x01, 0x01, 0x7F, 0x01, 0x01, // T
              0x3F, 0x40, 0x40, 0x40, 0x3F, // U
              0x1F, 0x20, 0x40, 0x20, 0x1F, // V
              0x7F, 0x20, 0x10, 0x20, 0x7F, // W
              0x41, 0x22, 0x1C, 0x22, 0x41, // X
              0x07, 0x08, 0x70, 0x08, 0x07, // Y
              0x61, 0x51, 0x49, 0x45, 0x43, // Z
              0x00, 0x7F, 0x41, 0x00, 0x00, // [
              0x02, 0x04, 0x08, 0x10, 0x20, // backslash
              0x00, 0x00, 0x41, 0x7F, 0x00, // ]
              0x04, 0x02, 0x01, 0x02, 0x04, // ^
              0x40, 0x40, 0x40, 0x40, 0x40, // _
            
              0x00, 0x01, 0x02, 0x04, 0x00, // `
              0x20, 0x54, 0x54, 0x54, 0x78, // a
              0x7F, 0x44, 0x44, 0x44, 0x38, // b
              0x38, 0x44, 0x44, 0x44, 0x44, // c
              0x38, 0x44, 0x44, 0x44, 0x7F, // d
              0x38, 0x54, 0x54, 0x54, 0x18, // e
              0x04, 0x04, 0x7E, 0x05, 0x05, // f
              0x18, 0xA4, 0xA4, 0xA4, 0x78, // g
              0x7F, 0x08, 0x04, 0x04, 0x78, // h
              0x00, 0x44, 0x7D, 0x40, 0x00, // i
              0x20, 0x40, 0x44, 0x3D, 0x00, // j
              0x7F, 0x10, 0x28, 0x44, 0x00, // k
              0x00, 0x41, 0x7F, 0x40, 0x00, // l
              0x7C, 0x04, 0x78, 0x04, 0x78, // m
              0x7C, 0x08, 0x04, 0x04, 0x78, // n
              0x38, 0x44, 0x44, 0x44, 0x38, // o
            
              0xFC, 0x24, 0x24, 0x24, 0x18, // p
              0x18, 0x24, 0x24, 0x24, 0xFC, // q
              0x00, 0x7C, 0x08, 0x04, 0x04, // r
              0x48, 0x54, 0x54, 0x54, 0x20, // s
              0x04, 0x04, 0x3F, 0x44, 0x44, // t
              0x3C, 0x40, 0x40, 0x20, 0x7C, // u
              0x1C, 0x20, 0x40, 0x20, 0x1C, // v
              0x3C, 0x40, 0x30, 0x40, 0x3C, // w
              0x44, 0x28, 0x10, 0x28, 0x44, // x
              0x0C, 0x50, 0x50, 0x50, 0x3C, // y
              0x44, 0x64, 0x54, 0x4C, 0x44, // z
              0x00, 0x08, 0x36, 0x41, 0x41, // {
              0x00, 0x00, 0x7F, 0x00, 0x00, // |
              0x41, 0x41, 0x36, 0x08, 0x00, // }
              0x02, 0x01, 0x02, 0x04, 0x02  // ~
            } ;
#endif