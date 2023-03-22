//***************************************************************************************************
// config.h                                                                                         *
//***************************************************************************************************
//
#define NAME "ESP32-Radio"                                // Define name of the radio, also AP SSID

//#define SDCARD                                            // For SD card support (reading MP3-files)

// Define (just one) type of MP3/AAC decoder
#define DEC_VS1053                                        // Hardware decoder for MP3, AAC, OGG
//#define DEC_VS1003                                      // Hardware decoder for MP3
//#define DEC_HELIX                                       // Software decoder for MP3, AAC. I2S output

// Define (just one) type of display.  See documentation.
#define BLUETFT                                           // Works also for RED TFT 128x160
//define ST7789                                           // 240x240 TFT
//#define OLED1306                                        // 64x128 I2C OLED SSD1306
//#define OLED1309                                        // 64x128 I2C OLED SSD1309
//#define OLED1106                                        // 64x128 I2C OLED SH1106
//#define DUMMYTFT                                        // Dummy display
//#define LCD1602I2C                                      // LCD 1602 display with I2C backpack
//#define LCD2004I2C                                      // LCD 2004 display with I2C backpack
//#define ILI9341                                         // ILI9341 240*320
//#define NEXTION                                         // Nextion display. Uses UART 2 (pin 16 and 17)
//

// Define ZIPPYB5 if a ZIPPY B5 Side Switch is used instead of a rotary switch
//#define ZIPPYB5

