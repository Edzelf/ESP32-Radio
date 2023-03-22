//***************************************************************************************************
// config.h                                                                                         *
//***************************************************************************************************
// Configuration definition for your particular set-up.                                             *
//***************************************************************************************************
//
//#define NAME "ESP32-Radio"                              // Define name of the radio, also AP SSID
                                                          // Default is "ESP32-Radio"

//#define SDCARD                                            // Experimental: For SD card support (reading MP3-files)

//#define ETHERNET                                        // For wired Ethernet (WT32-ETH-01 or similar)

#define FIXEDWIFI "LAPTOP-ED/wifiwifi"                    // Add a fixed SSID to the list (WiFi only)

// Define (just one) type of MP3/AAC decoder
#define DEC_VS1053                                        // Hardware decoder for MP3, AAC, OGG
//#define DEC_VS1003                                      // Hardware decoder for MP3
//#define DEC_HELIX                                       // Software decoder for MP3, AAC. I2S output
//#define DEC_HELIX_AI                                    // Software decoder for AI Audio kit (AC101)
//#define DEC_HELIX_INT                                   // Software decoder for MP3, AAC. DAC output
                                                          // Needs platform = espressif32@3.1.0 !!!!

// Define (just one) type of display.  See documentation.
#define BLUETFT                                         // Works also for RED TFT 128x160
//#define ST7789                                          // 240x240 TFT
//#define OLED1306                                        // 64x128 I2C OLED SSD1306
//#define OLED1309                                        // 64x128 I2C OLED SSD1309
//#define OLED1106                                        // 64x128 I2C OLED SH1106
//#define DUMMYTFT                                        // Dummy display
//#define LCD1602I2C                                      // LCD 1602 display with I2C backpack
//#define LCD2004I2C                                      // LCD 2004 display with I2C backpack
//#define ILI9341                                         // ILI9341 240*320
//#define NEXTION                                         // Nextion display
//

// Define ZIPPYB5 if a ZIPPY B5 Side Switch is used instead of a rotary switch
///#define ZIPPYB5

// End of configuration parameters.

