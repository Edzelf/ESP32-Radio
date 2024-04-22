//***************************************************************************************************
//*  ESP32_Radio V2 -- Webradio receiver for ESP32, VS1053 MP3 module and optional display.         *
//*                    By Ed Smallenburg.                                                           *
//***************************************************************************************************
// ESP32 libraries used:  See platformio.ini
// A library for the VS1053 (for ESP32) is not available (or not easy to find).  Therefore
// a class for this module is derived from the maniacbug library and integrated in this sketch.
// The Helix codecs for MP3 and AAC are taken from Wolle (schreibfaul1), see:
// https://github.com/schreibfaul1/ESP32-audioI2S
//
// See http://www.internet-radio.com for suitable stations.  Add the stations of your choice
// to the preferences through the webinterface.
// You may also use the "search" page of the webinterface to find stations.
//
// Brief description of the program:
// First a suitable WiFi network is found and a connection is made.
// Then a connection will be made to a shoutcast server.  The server starts with some
// info in the header in readable ascii, ending with a double CRLF, like:
//  icy-name:Classic Rock Florida - SHE Radio
//  icy-genre:Classic Rock 60s 70s 80s Oldies Miami South Florida
//  icy-url:http://www.ClassicRockFLorida.com
//  content-type:audio/mpeg
//  icy-pub:1
//  icy-metaint:32768          - Metadata after 32768 bytes of MP3-data
//  icy-br:128                 - in kb/sec (for Ogg this is like "icy-br=Quality 2"
//
// After de double CRLF is received, the server starts sending mp3- or Ogg-data.  For mp3, this
// data may contain metadata (non mp3) after every "metaint" mp3 bytes.
// The metadata is empty in most cases, but if any is available the content will be
// presented on the TFT.
// Pushing an input button causes the player to execute a programmable command.
//
// The display used is a Chinese 1.8 color TFT module 128 x 160 pixels.
// Now there is room for 26 characters per line and 16 lines.
// Software will work without installing the display.
// Other displays are also supported. See documentation.
// For configuration of the WiFi network(s): see the global data section further on.
//
// The VSPI interface is used for VS1053, TFT and SD.
//
// Wiring. Note that this is just an example.  Pins (except 18, 19 and 23 of the SPI interface)
// can be configured in the config page of the web interface.
//
// ESP32dev Signal  Wired to LCD        Wired to VS1053      AI Audio board    Wired to the rest
// -------- ------  --------------      -------------------  ---------------   ------------------------
// GPIO32           -                   pin 1 XDCS           I2C Clock
// GPIO33           -                   -                    I2C Data
// GPIO5            -                   pin 2 XCS            KEY 6             -
// GPIO4            -                   pin 4 DREQ           AMPLIFIER_ENABLE  -
// GPIO2            pin 3 D/C or A0     -                    SPI_MISO          -
// GPIO16   RXD2    -                   -                                      TX of NEXTION (if in use)
// GPIO17   TXD2    -                   -                                      RX of NEXTION (if in use)
// GPIO18   SCK     pin 5 CLK or SCK    pin 5 SCK            KEY 5             -
// GPIO19   MISO    -                   pin 7 MISO           KEY 3             -
// GPIO23   MOSI    pin 4 DIN or SDA    pin 6 MOSI           KEY 4             -
// GPIO15           pin 2 CS            -                    SPI_MOSI          -
// GPIO3    RXD0    -                   -                                      Reserved serial input
// GPIO1    TXD0    -                   -                                      Reserved serial output
// GPIO34   -       -                   -                    SD detect         Optional pull-up resistor
// GPIO35   -       -                   -                                      Infrared receiver VS1838B
// GPIO25   -       -                   -                    I2S DSIN          Rotary encoder CLK
// GPIO26   -       -                   -                    I2S LRC           Rotary encoder DT
// GPIO27   -       -                   -                    I2S BCLK          Rotary encoder SW
// GPIO13   -       -                   -                    SD card CS        -
// GPIO14   -       -                   -                    SPI_SCK           -
// GPIO36   -       -                   -                    KEY 1             -
// GPIO13   -       -                   -                    KEY 2             -
// GPIO19   -       -                   -                    KEY 3             -
// GPIO23   -       -                   -                    KEY 4             -
// GPIO18   -       -                   -                    KEY 5             -
// GPIO05   -       -                   -                    KEY 6             -
// -------  ------  ---------------     -------------------                    ----------------
// GND      -       pin 8 GND           pin 8 GND                              Power supply GND
// VCC 5 V  -       pin 7 BL            -                                      Power supply
// VCC 5 V  -       pin 6 VCC           pin 9 5V                               Power supply
// EN       -       pin 1 RST           pin 3 XRST                             -
//
//  History:
//   Date     Author        Remarks
// ----------  --  ------------------------------------------------------------------
// 06-08-2021, ES: Copy from version 1.
// 06-08-2021, ES: Use SPIFFS and Async webserver.
// 23-08-2021, ES: Version with software MP3/AAC decoders.
// 05-10-2021, ES: Fixed internal DAC output, fixed OTA update.
// 06-10-2021, ES: Fixed AP mode.
// 10-02-2022, ES: Included ST7789 display.
// 11-02-2022, ES: SD card implementation.
// 26-03-2022, ES: Fixed NEXTION bug.
// 12-04-2022, ES: Fixed dataqueue bug (NEXT function).
// 13-04-2022, ES: Fixed redirect bug (preset was reset), fixed playlist.
// 14-04-2022, ES: Added posibility for a fixed WiFi network.
// 15-04-2022, ES: Redesigned station selection.
// 25-04-2022, ES: Support for WT32-ETH01 (wired Ethernet).
// 13-05-2022, ES: Correction I2S settings.
// 15-05-2022, ES: Correction mp3 list for web interface.
// 17-11-2022, ES: Support of AI Audio kit V2.1.
// 22-11-2022, ES: Fixed memory leak.
// 28-04-2023, ES: Correct "Request station:port failed!"
// 10-05-2023, ES: SD card files stored on the SDcard.
// 19-05-2023, ES: Mute and unmute with 2 buttons or commands.
// 22-05-2023, ES: Use internal mutex for SPI bus.
// 16-06-2023, ES: Add sleep commmeand.
// 09-10-2023, ES: Reduce GPIO errors by checking GPIO pins.
// 14-12-2023, ES: Add mqtt trigger to refresh all items.
// 16-01-2024, ES: Disable brownout.
// 16-02-2024, ES: SPDIFF output (experimental).
// 19-02-2024, ES: Fixed mono stream, correct handling of reset command.

//
// Define the version number, the format used is the HTTP standard.
#define VERSION     "Wed, 06 Mar 2024 16:00:00 GMT"
//
#include <Arduino.h>                                      // Standard include for Platformio Arduino projects
#include "soc/soc.h"                                      // For brown-out detector setting
#include "soc/rtc_cntl_reg.h"                             // For brown-out detector settingtest
//#include <esp_log.h>
#include <WiFi.h>
#include "config.h"                                       // Specify display type, decoder type
#include <nvs.h>                                          // Access to NVS
#include <PubSubClient.h>                                 // MTTQ access
#ifdef ETHERNET
  #include <ETH.h>                                        // Definitions for Ethernet controller
  //#include <AsyncWebServer_WT32_ETH01.h>
  #define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN              // External clock from crystal oscillator
  #define ETH_TYPE        ETH_PHY_LAN8720                 // Type of controller
  #define ETH_ADDR        1                               // I2C address of Ethernet PHY
#else
  #include <WiFiMulti.h>                                  // Handle multiple WiFi networks
  WiFiMulti wifiMulti ;                                   // Object for WiFiMulti
#endif
#include <ESPAsyncWebServer.h>                            // For Async Web server
#include <ESPmDNS.h>                                      // For multicast DNS
#include <time.h>                                         // Time functions
#include <SPI.h>                                          // For SPI handling
#ifdef ENABLEOTA
  #include <ArduinoOTA.h>                                 // Over the air updates
#endif
#include <freertos/queue.h>                               // FreeRtos queue support
#include <freertos/task.h>                                // FreeRtos task handling
#include <esp_task_wdt.h>
#include <driver/adc.h>
#include <base64.h>                                       // For Basic authentication
#include <SPIFFS.h>                                       // Filesystem
#include "utils.h"                                        // Some handy utilities
#if defined(DEC_HELIX_SPDIF) || defined(DEC_HELIX_INT) || defined(DEC_HELIX_AI)
  #define DEC_HELIX
#endif
#if defined(DEC_HELIX_AI)                                 // AI Audio kit board?
  #include <AC101.h>
  #define GPIO_PA_EN 21                                   // GPIO for enabling amplifier
  AC101 dac ;                                             // AC101 controls
#endif
#if defined(DEC_HELIX)
  #include <driver/i2s.h>                                 // Driver for I2S output
  #include "mp3_decoder.h"                                // Yes, include libhelix_HMP3DECODER
  #include "aac_decoder.h"                                // and libhelix_HAACDECODER
  #include "helixfuncs.h"                                 // Helix functions
#else
  #include "VS1053.h"                                     // Driver for VS1053
#endif
#define MAXKEYS           200                             // Max. number of NVS keys in table
#define FSIF              true                            // Format SPIFFS if not existing
#define QSIZ              400                             // Number of entries in the MP3 stream queue
#define NVSBUFSIZE        150                             // Max size of a string in NVS
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Note that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#ifndef NAME                                              // Name may be defined in config.h
  #define NAME            "ESP32-Radio"
#endif
#define MAXPRESETS        200                             // Max number of presets in preferences
#define MAXMQTTCONNECTS   5                               // Maximum number of MQTT reconnects before give-up
#define METASIZ           1024                            // Size of metaline buffer
#define BL_TIME           45                              // Time-out [sec] for blanking TFT display (BL pin)
//
// Subscription topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
// by the the mqttprefix in the preferences.  The next definition will yield the topic
// "ESP32Radio/command" if mqttprefix is "ESP32Radio".
#define MQTT_SUBTOPIC     "command"                      // Command to receive from MQTT
//
#define heapspace heap_caps_get_largest_free_block ( MALLOC_CAP_8BIT )

//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void        tftlog ( const char *str, bool newline = false ) ;
bool        showstreamtitle ( const char* ml, bool full = false ) ;
void        handlebyte_ch ( uint8_t b ) ;
void        handleCmd()  ;
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void        chomp ( String &str ) ;
String      nvsgetstr ( const char* key ) ;
bool        nvssearch ( const char* key ) ;
void        sdfuncs() ;
void        stop_mp3client () ;
void        tftset ( uint16_t inx, const char *str ) ;
void        tftset ( uint16_t inx, String& str ) ;
void        playtask ( void* parameter ) ;                 // Task to play the stream on VS1053 or HELIX decoder
void        displayinfo ( uint16_t inx ) ;
void        gettime() ;
void        reservepin ( int8_t rpinnr ) ;
uint32_t    ssconv ( const uint8_t* bytes ) ;
void        scan_content_length ( const char* metalinebf ) ;
void        handle_notfound  ( AsyncWebServerRequest *request ) ;
void        handle_getprefs  ( AsyncWebServerRequest *request ) ;
void        handle_saveprefs ( AsyncWebServerRequest *request ) ;
void        handle_getdefs   ( AsyncWebServerRequest *request ) ;
void        handle_settings  ( AsyncWebServerRequest *request ) ;
void        handle_mp3list   ( AsyncWebServerRequest *request ) ;
void        handle_reset     ( AsyncWebServerRequest *request ) ;
bool        readhostfrompref ( int16_t preset, String* host, String* hsym = NULL ) ;



//**************************************************************************************************
// Several structs and enums.                                                                      *
//**************************************************************************************************
//

enum qdata_type { QDATA, QSTARTSONG, QSTOPSONG,       // datatyp in qdata_struct,
                  QSTOPTASK } ;
struct qdata_struct                                   // Data in queue for playtask (dataqueue)
{
  qdata_type                          datatyp ;       // Identifier
  __attribute__((aligned(4))) uint8_t buf[32] ;       // Buffer for chunk of mp3 data
} ;

struct ini_struct
{
  String         mqttbroker ;                         // The name of the MQTT broker server
  String         mqttprefix ;                         // Prefix to use for topics
  uint16_t       mqttport ;                           // Port, default 1883
  String         mqttuser ;                           // User for MQTT authentication
  String         mqttpasswd ;                         // Password for MQTT authentication
  uint8_t        reqvol ;                             // Requested volume
  uint8_t        rtone[4] ;                           // Requested bass/treble settings
  String         clk_server ;                         // Server to be used for time of day clock
  int8_t         clk_offset ;                         // Offset in hours with respect to UTC
  int8_t         clk_dst ;                            // Number of hours shift during DST
  int8_t         ir_pin ;                             // GPIO connected to output of IR decoder
  int8_t         enc_clk_pin ;                        // GPIO connected to CLK of rotary encoder
  int8_t         enc_dt_pin ;                         // GPIO connected to DT of rotary encoder
  int8_t         enc_sw_pin ;                         // GPIO connected to SW of rotary encoder / ZIPPY B5
  int8_t         enc_up_pin ;                         // GPIO connected to UP of ZIPPY B5 side switch
  int8_t         enc_dwn_pin ;                        // GPIO connected to DOWN of ZIPPY B5 side switch
  int8_t         tft_cs_pin ;                         // GPIO connected to CS of TFT screen
  int8_t         tft_dc_pin ;                         // GPIO connected to D/C or A0 of TFT screen
  int8_t         tft_scl_pin ;                        // GPIO connected to SCL of i2c TFT screen
  int8_t         tft_sda_pin ;                        // GPIO connected to SDA of I2C TFT screen
  int8_t         tft_bl_pin ;                         // GPIO to activate BL of display
  int8_t         tft_blx_pin ;                        // GPIO to activate BL of display (inversed logic)
  int8_t         nxt_rx_pin ;                         // GPIO for input from NEXTION
  int8_t         nxt_tx_pin ;                         // GPIO for output to NEXTION
  int8_t         sd_cs_pin ;                          // GPIO connected to CS of SD card
  int8_t         sd_detect_pin ;                      // GPIO connected to SC card detect (LOW is inserted)
  int8_t         vs_cs_pin ;                          // GPIO connected to CS of VS1053
  int8_t         vs_dcs_pin ;                         // GPIO connected to DCS of VS1053
  int8_t         vs_dreq_pin ;                        // GPIO connected to DREQ of VS1053
  int8_t         shutdown_pin ;                       // GPIO to shut down the amplifier
  int8_t         shutdownx_pin ;                      // GPIO to shut down the amplifier (inversed logic)
  int8_t         spi_sck_pin ;                        // GPIO connected to SPI SCK pin
  int8_t         spi_miso_pin ;                       // GPIO connected to SPI MISO pin
  int8_t         spi_mosi_pin ;                       // GPIO connected to SPI MOSI pin
  int8_t         i2s_bck_pin ;                        // GPIO Pin number for I2S "BCK"
  int8_t         i2s_lck_pin ;                        // GPIO Pin number for I2S "L(R)CK"
  int8_t         i2s_din_pin ;                        // GPIO Pin number for I2S "DIN"
  int8_t         i2s_spdif_pin ;                      // GPIO Pin number for SPDIF output
  int8_t         eth_mdc_pin ;                        // GPIO Pin number for Ethernet controller MDC
  int8_t         eth_mdio_pin ;                       // GPIO Pin number for Ethernet controller MDIO
  int8_t         eth_power_pin ;                      // GPIO Pin number for Ethernet controller POWER
  uint16_t       bat0 ;                               // ADC value for 0 percent battery charge
  uint16_t       bat100 ;                             // ADC value for 100 percent battery charge
} ;

struct WifiInfo_t                                     // For list with WiFi info
{
  char * ssid ;                                       // SSID for an entry
  char * passphrase ;                                 // Passphrase for an entry
} ;

// Preset info
enum station_state_t { ST_PRESET, ST_REDIRECT,        // Possible preset status
                       ST_PLAYLIST, ST_STATION } ;
struct preset_info_t
{
    int16_t            preset ;                       // Preset to play
    int16_t            highest_preset ;               // Highest possible preset
    station_state_t    station_state ;                // Station state
    int16_t            playlistnr ;                   // Index in playlist
    int16_t            highest_playlistnr ;           // Highest possible preset
    String             playlisthost ;                 // Host with playlist
    String             host ;                         // Resulting host
    String             hsym ;                         // Symbolic name (comment after name)
} ;

const char* TAG = "main" ;                            // For debug lines


//**************************************************************************************************
// Global data section.                                                                            *
//**************************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is              *
// saved in the preferences by the webinterface.  On restart the new data will                     *
// de read from these preferences.                                                                 *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.                       *
//**************************************************************************************************

enum datamode_t { INIT = 0x1, HEADER = 0x2, DATA = 0x4,      // State for datastream
                  METADATA = 0x8, PLAYLISTINIT = 0x10,
                  PLAYLISTHEADER = 0x20, PLAYLISTDATA = 0x40,
                  STOPREQD = 0x80, STOPPED = 0x100
                } ;

// Global variables
preset_info_t        presetinfo ;                        // Info about the current or new station
ini_struct           ini_block ;                         // Holds configurable data
AsyncWebServer       cmdserver ( 80 ) ;                  // Instance of embedded webserver, port 80
AsyncClient*         mp3client = NULL ;                  // An instance of the mp3 client
WiFiClient           wmqttclient ;                       // An instance for mqtt
PubSubClient         mqttclient ( wmqttclient ) ;        // Client for MQTT subscriber
TaskHandle_t         maintask ;                          // Taskhandle for main task
TaskHandle_t         xplaytask ;                         // Task handle for playtask
TaskHandle_t         xsdtask ;                           // Task handle for SD task
hw_timer_t*          timer = NULL ;                      // For timer
char                 timetxt[9] ;                        // Converted timeinfo
const qdata_struct   stopcmd = {QSTOPSONG} ;             // Command for radio/SD
const qdata_struct   startcmd = {QSTARTSONG} ;           // Command for radio/SD
QueueHandle_t        radioqueue = 0 ;                    // Queue for icecast commands
QueueHandle_t        dataqueue = 0 ;                     // Queue for mp3 datastream
QueueHandle_t        sdqueue = 0 ;                       // For commands to sdfuncs
qdata_struct         outchunk ;                          // Data to queue
qdata_struct         inchunk ;                           // Data from queue
uint8_t*             outqp = outchunk.buf ;              // Pointer to buffer in outchunk
uint32_t             totalcount = 0 ;                    // Counter mp3 data
datamode_t           datamode ;                          // State of datastream
int                  metacount ;                         // Number of bytes in metadata
int                  datacount ;                         // Counter databytes before metadata
RTC_NOINIT_ATTR char metalinebf[METASIZ + 1] ;           // Buffer for metaline/ID3 tags
RTC_NOINIT_ATTR char cmd[130] ;                          // Command from MQTT or Serial
int16_t              metalinebfx ;                       // Index for metalinebf
String               icystreamtitle ;                    // Streamtitle from metadata
String               icyname ;                           // Icecast station name
String               audio_ct ;                          // Content-type, like "audio/aacp"
String               ipaddress ;                         // Own IP-address
int                  bitrate ;                           // Bitrate in kb/sec
int                  mbitrate ;                          // Measured bitrate
int                  metaint = 0 ;                       // Number of databytes between metadata
bool                 reqtone = false ;                   // New tone setting requested
bool                 muteflag = false ;                  // Mute output
bool                 resetreq = false ;                  // Request to reset the ESP32
bool                 testreq = false ;                   // Request to print test info
bool                 sleepreq = false ;                  // Request for deep sleep
bool                 eth_connected = false ;             // Ethernet connected or not
bool                 NetworkFound = false ;              // True if WiFi network connected
bool                 mqtt_on = false ;                   // MQTT in use
uint16_t             mqttcount = 0 ;                     // Counter MAXMQTTCONNECTS
int8_t               playingstat = 0 ;                   // 1 if radio is playing (for MQTT)
int16_t              playlist_num = 0 ;                  // Nonzero for selection from playlist
bool                 chunked = false ;                   // Station provides chunked transfer
int                  chunkcount = 0 ;                    // Counter for chunked transfer
uint16_t             ir_value = 0 ;                      // IR code
uint32_t             ir_0 = 550 ;                        // Average duration of an IR short pulse
uint32_t             ir_1 = 1650 ;                       // Average duration of an IR long pulse
struct tm            timeinfo ;                          // Will be filled by NTP server
bool                 time_req = false ;                  // Set time requested
uint16_t             adcvalraw ;                         // ADC value (raw)
uint16_t             adcval ;                            // ADC value (battery voltage, averaged)
uint32_t             clength ;                           // Content length found in http header
uint16_t             bltimer = 0 ;                       // Backlight time-out counter
bool                 dsp_ok = false ;                    // Display okay or not
int                  ir_intcount = 0 ;                   // For test IR interrupts
bool                 spftrigger = false ;                // To trigger execution of special functions
const char*          fixedwifi = "" ;                    // Used for FIXEDWIFI option
File                 SPIFFSfile ;                        /// File handle for SPIFFS file

std::vector<WifiInfo_t> wifilist ;                       // List with wifi_xx info

// nvs stuff
const esp_partition_t*  nvs ;                                     // Pointer to partition struct
esp_err_t               nvserr ;                                  // Error code from nvs functions
uint32_t                nvshandle = 0 ;                           // Handle for nvs access
RTC_NOINIT_ATTR char    nvskeys[MAXKEYS][NVS_KEY_NAME_MAX_SIZE] ; // Space for NVS keys

// Rotary encoder stuff
#define sv DRAM_ATTR static volatile
sv uint16_t       clickcount = 0 ;                       // Incremented per encoder click
sv int16_t        rotationcount = 0 ;                    // Current position of rotary switch
sv uint16_t       enc_inactivity = 0 ;                   // Time inactive
sv bool           singleclick = false ;                  // True if single click detected
sv bool           doubleclick = false ;                  // True if double click detected
sv bool           tripleclick = false ;                  // True if triple click detected
sv bool           longclick = false ;                    // True if longclick detected
enum enc_menu_t { VOLUME, PRESET, TRACK } ;              // State for rotary encoder menu
enc_menu_t        enc_menu_mode = VOLUME ;               // Default is VOLUME mode
//
struct progpin_struct                                    // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
                                                         // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
} ;

progpin_struct   progpin[] =                             // Input pins and programmed function
{
  {  0, false, false,  "", false },
  //{  1, true,  false,  "", false },                    // Reserved for TX Serial output
  {  2, false, false,  "", false },
  //{  3, true,  false,  "", false },                    // Reserved for RX Serial input
  {  4, false, false,  "", false },
  {  5, false, false,  "", false },
  //{  6, true,  false,  "", false },                    // Reserved for FLASH SCK
  //{  7, true,  false,  "", false },                    // Reserved for FLASH D0
  //{  8, true,  false,  "", false },                    // Reserved for FLASH D1
  //{  9, true,  false,  "", false },                    // Reserved for FLASH D2
  //{ 10, true,  false,  "", false },                    // Reserved for FLASH D3
  //{ 11, true,  false,  "", false },                    // Reserved for FLASH CMD
  { 12, false, false,  "", false },
  { 13, false, false,  "", false },
  { 14, false, false,  "", false },
  { 15, false, false,  "", false },
  { 16, false, false,  "", false },                      // May be UART 2 RX for Nextion
  { 17, false, false,  "", false },                      // May be UART 2 TX for Nextion
  { 18, false, false,  "", false },                      // Default for SPI CLK
  { 19, false, false,  "", false },                      // Default for SPI MISO
  //{ 20, true,  false,  "", false },                    // Not exposed on DEV board
  { 21, false, false,  "", false },                      // Also Wire SDA
  { 22, false, false,  "", false },                      // Also Wire SCL
  { 23, false, false,  "", false },                      // Default for SPI MOSI
  //{ 24, true,  false,  "", false },                    // Not exposed on DEV board
  { 25, false, false,  "", false },                      // DAC output / I2S output
  { 26, false, false,  "", false },                      // DAC output / I2S output
  { 27, false, false,  "", false },                      // I2S output
  //{ 28, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 29, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 30, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 31, true,  false,  "", false },                    // Not exposed on DEV board
  { 32, false, false,  "", false },
  { 33, false, false,  "", false },
  { 34, false, false,  "", false },                      // Note, no internal pull-up
  { 35, false, false,  "", false },                      // Note, no internal pull-up
  //{ 36, true,  false,  "", false },                    // Reserved for ADC battery level
  { 39, false,  false,  "", false },                     // Note, no internal pull-up
  { -1, false, false,  "", false }                       // End of list
} ;

struct touchpin_struct                                   // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number GPIO
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
                                                         // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
  int16_t        count ;                                 // Counter number of times low level
} ;
touchpin_struct   touchpin[] =                           // Touch pins and programmed function
{
  {   4, false, false, "", false, 0 },                   // TOUCH0
  {   0, true,  false, "", false, 0 },                   // TOUCH1, reserved for BOOT button
  {   2, false, false, "", false, 0 },                   // TOUCH2
  {  15, false, false, "", false, 0 },                   // TOUCH3
  {  13, false, false, "", false, 0 },                   // TOUCH4
  {  12, false, false, "", false, 0 },                   // TOUCH5
  {  14, false, false, "", false, 0 },                   // TOUCH6
  {  27, false, false, "", false, 0 },                   // TOUCH7
  {  33, false, false, "", false, 0 },                   // TOUCH8
  {  32, false, false, "", false, 0 },                   // TOUCH9
  {  -1, false, false, "", false, 0 }                    // End of list
  // End of table
} ;


//**************************************************************************************************
// End of global data section.                                                                     *
//**************************************************************************************************




//**************************************************************************************************
//                                     M Q T T P U B _ C L A S S                                   *
//**************************************************************************************************
// ID's for the items to publish to MQTT.  Is index in amqttpub[]
enum { MQTT_IP,     MQTT_ICYNAME, MQTT_STREAMTITLE, MQTT_NOWPLAYING,
       MQTT_PRESET, MQTT_VOLUME, MQTT_PLAYING, MQTT_PLAYLISTPOS
     } ;
enum { MQSTRING, MQINT8, MQINT16 } ;                     // Type of variable to publish

class mqttpubc                                           // For MQTT publishing
{
    struct mqttpub_struct
    {
      const char*    topic ;                             // Topic as partial string (without prefix)
      uint8_t        type ;                              // Type of payload
      void*          payload ;                           // Payload for this topic
      bool           topictrigger ;                      // Set to true to trigger MQTT publish
    } ;
    // Publication topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
    // by the the mqttprefix in the preferences.
  protected:
    mqttpub_struct amqttpub[9] =                         // Definitions of various MQTT topic to publish
    { // Index is equal to enum above
      { "ip",              MQSTRING, &ipaddress,             false }, // Definition for MQTT_IP
      { "icy/name",        MQSTRING, &icyname,               false }, // Definition for MQTT_ICYNAME
      { "icy/streamtitle", MQSTRING, &icystreamtitle,        false }, // Definition for MQTT_STREAMTITLE
      { "nowplaying",      MQSTRING, &ipaddress,             false }, // Definition for MQTT_NOWPLAYING
      { "preset" ,         MQINT8,   &presetinfo.preset,     false }, // Definition for MQTT_PRESET
      { "volume" ,         MQINT8,   &ini_block.reqvol,      false }, // Definition for MQTT_VOLUME
      { "playing",         MQINT8,   &playingstat,           false }, // Definition for MQTT_PLAYING
      { "playlist/pos",    MQINT16,  &presetinfo.playlistnr, false }, // Definition for MQTT_PLAYLISTPOS
      { NULL,              0,        NULL,                   false }  // End of definitions
    } ;
  public:
    void          trigger ( uint8_t item ) ;                      // Trigger publishing for one item
    void          triggerall () ;                                 // Trigger all items
    void          publishtopic() ;                                // Publish triggerer items
} ;


//**************************************************************************************************
// MQTTPUB  class implementation.                                                                  *
//**************************************************************************************************

//**************************************************************************************************
//                                            T R I G G E R                                        *
//**************************************************************************************************
// Set request for an item to publish to MQTT.                                                     *
//**************************************************************************************************
void mqttpubc::trigger ( uint8_t item )                    // Trigger publishig for one item
{
  amqttpub[item].topictrigger = true ;                     // Request re-publish for an item
}

//**************************************************************************************************
//                                       T R I G G E R A L L                                       *
//**************************************************************************************************
// Set request for all items to publish to MQTT.                                                   *
//**************************************************************************************************
void mqttpubc::triggerall()                                // Trigger publishig for one item
{
  int item = 0 ;                                           // Item to refresh

  while ( amqttpub[item].topic )                           // Cycle through the list of items
  {
    trigger ( item++ ) ;                                   // Trigger this item and select next
  }
}

//**************************************************************************************************
//                                     P U B L I S H T O P I C                                     *
//**************************************************************************************************
// Publish a topic to MQTT broker.                                                                 *
//**************************************************************************************************
void mqttpubc::publishtopic()
{
  int         i = 0 ;                                         // Loop control
  char        topic[80] ;                                     // Topic to send
  const char* payload ;                                       // Points to payload
  char        intvar[10] ;                                    // Space for integer parameter

  while ( amqttpub[i].topic )
  {
    if ( amqttpub[i].topictrigger )                           // Topic ready to send?
    {
      amqttpub[i].topictrigger = false ;                      // Success or not: clear trigger
      sprintf ( topic, "%s/%s", ini_block.mqttprefix.c_str(),
                amqttpub[i].topic ) ;                         // Add prefix to topic
      switch ( amqttpub[i].type )                             // Select conversion method
      {
        case MQSTRING :
          payload = ((String*)amqttpub[i].payload)->c_str() ;
          //payload = pstr->c_str() ;                           // Get pointer to payload
          break ;
        case MQINT8 :
          sprintf ( intvar, "%d",
                    *(int8_t*)amqttpub[i].payload ) ;         // Convert to array of char
          payload = intvar ;                                  // Point to this array
          break ;
        case MQINT16 :
          sprintf ( intvar, "%d",
                    *(int16_t*)amqttpub[i].payload ) ;        // Convert to array of char
          payload = intvar ;                                  // Point to this array
          break ;
        default :
          continue ;                                          // Unknown data type
      }
      ESP_LOGI ( TAG, "Publish to topic %s : %s",             // Show for debug
                 topic, payload ) ;
      if ( !mqttclient.publish ( topic, payload ) )           // Publish!
      {
        ESP_LOGE ( TAG, "MQTT publish failed!" ) ;            // Failed
      }
      return ;                                                // Do the rest later
    }
    i++ ;                                                     // Next entry
  }
}

mqttpubc         mqttpub ;                                    // Instance for mqttpubc

//
// Include software for the right display
#ifdef BLUETFT
 #include "bluetft.h"                                        // For ILI9163C or ST7735S 128x160 display
#endif
#ifdef ST7789
 #include "ST7789.h"                                         // For ST7789 240x240 display
#endif
#ifdef ILI9341
 #include "ILI9341.h"                                        // For ILI9341 320x240 display
#endif
#ifdef OLED1306
 #include "oled.h"                                           // For OLED I2C SD1306 64x128 display
#endif
#ifdef OLED1309
 #include "oled.h"                                           // For OLED I2C SD1309 64x128 display
#endif
#ifdef OLED1106
 #include "oled.h"                                           // For OLED I2C SH1106 64x128 display
#endif
#ifdef LCD1602I2C
 #include "LCD1602.h"                                        // For LCD 1602 display (I2C)
#endif
#ifdef LCD2004I2C
 #include "LCD2004.h"                                        // For LCD 2004 display (I2C)
#endif
#ifdef DUMMYTFT
 #include "dummytft.h"                                       // For Dummy display
#endif
#ifdef NEXTION
 #include "NEXTION.h"                                        // For NEXTION display
#endif

// Include software for SD card.  Will include dummy if "SDCARD" is not defined
#include "SDcard.h"                                         // For SD card interface

//**************************************************************************************************
//                                  M Y Q U E U E S E N D                                          *
//**************************************************************************************************
// Send to queue if existing.                                                                      *
//**************************************************************************************************
void myQueueSend ( QueueHandle_t q, const void* msg, int waittime = 0 )
{
  if ( q )                                                // Check if we have a legal queue
  {
    xQueueSend ( q, msg, waittime ) ;                     // Queue okay, send to it
  }
}


//**************************************************************************************************
//                                           B L S E T                                             *
//**************************************************************************************************
// Enable or disable the TFT backlight if configured.                                              *
// May be called from interrupt level.                                                             *
//**************************************************************************************************
void IRAM_ATTR blset ( bool enable )
{
  if ( ini_block.tft_bl_pin >= 0 )                       // Backlight for TFT control?
  {
    digitalWrite ( ini_block.tft_bl_pin, enable ) ;      // Enable/disable backlight
  }
  if ( ini_block.tft_blx_pin >= 0 )                      // Backlight for TFT (inversed logic) control?
  {
    digitalWrite ( ini_block.tft_blx_pin, !enable ) ;    // Enable/disable backlight
  }
  if ( enable )
  {
    bltimer = 0 ;                                        // Reset counter backlight time-out
  }
}


//**************************************************************************************************
//                                      N V S O P E N                                              *
//**************************************************************************************************
// Open Preferences with my-app namespace. Each application module, library, etc.                  *
// has to use namespace name to prevent key name collisions. We will open storage in               *
// RW-mode (second parameter has to be false).                                                     *
//**************************************************************************************************
void nvsopen()
{
  if ( ! nvshandle )                                         // Opened already?
  {
    nvserr = nvs_open ( NAME, NVS_READWRITE, &nvshandle ) ;  // No, open nvs
    if ( nvserr )
    {
      ESP_LOGE ( TAG, "nvs_open failed!" ) ;
    }
  }
}


//**************************************************************************************************
//                                      N V S C L E A R                                            *
//**************************************************************************************************
// Clear all preferences.                                                                          *
//**************************************************************************************************
esp_err_t nvsclear()
{
  nvsopen() ;                                         // Be sure to open nvs
  return nvs_erase_all ( nvshandle ) ;                // Clear all keys
}


//**************************************************************************************************
//                                      N V S G E T S T R                                          *
//**************************************************************************************************
// Read a string from nvs.                                                                         *
//**************************************************************************************************
String nvsgetstr ( const char* key )
{
  static char   nvs_buf[NVSBUFSIZE] ;       // Buffer for contents
  size_t        len = NVSBUFSIZE ;          // Max length of the string, later real length

  nvsopen() ;                               // Be sure to open nvs
  nvs_buf[0] = '\0' ;                       // Return empty string on error
  nvserr = nvs_get_str ( nvshandle, key, nvs_buf, &len ) ;
  if ( nvserr )
  {
    ESP_LOGE ( TAG, "nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
               nvserr, key, strlen ( key), len ) ;
    ESP_LOGE ( TAG, "Contents: %s", nvs_buf ) ;
  }
  return String ( nvs_buf ) ;
}


//**************************************************************************************************
//                                      N V S S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in nvs.  Length is limited to allow easy read-back.                        *
// No writing if no change.                                                                        *
//**************************************************************************************************
esp_err_t nvssetstr ( const char* key, String val )
{
  String curcont ;                                         // Current contents
  bool   wflag = true  ;                                   // Assume update or new key

  //ESP_LOGI ( TAG, "Setstring for %s: %s", key, val.c_str() ) ;
  if ( val.length() >= NVSBUFSIZE )                        // Limit length of string to store
  {
    ESP_LOGE ( TAG, "nvssetstr length failed!" ) ;
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE ;
  }
  if ( nvssearch ( key ) )                                 // Already in nvs?
  {
    curcont = nvsgetstr ( key ) ;                          // Read current value
    wflag = ( curcont != val ) ;                           // Value change?
  }
  if ( wflag )                                             // Update or new?
  {
    //ESP_LOGI ( TAG, "nvssetstr update value" ) ;
    nvserr = nvs_set_str ( nvshandle, key, val.c_str() ) ; // Store key and value
    if ( nvserr )                                          // Check error
    {
      ESP_LOGE ( TAG, "nvssetstr failed!" ) ;
    }
  }
  return nvserr ;
}


//**************************************************************************************************
//                                      N V S S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in nvs.                                                                     *
//**************************************************************************************************
bool nvssearch ( const char* key )
{
  size_t        len = NVSBUFSIZE ;                      // Length of the string

  nvsopen() ;                                           // Be sure to open nvs
  nvserr = nvs_get_str ( nvshandle, key, NULL, &len ) ; // Get length of contents
  return ( nvserr == ESP_OK ) ;                         // Return true if found
}


//**************************************************************************************************
//                                      Q U E U E T O P T                                          *
//**************************************************************************************************
// Queue a special function for the play task.                                                     *
// These are high priority messages like stop or start.  So we make sure the message fits.         *
//**************************************************************************************************
void queueToPt ( qdata_type func )
{
  qdata_struct     specchunk ;                            // Special function to queue

  while ( xQueueReceive ( dataqueue, &specchunk, 0 ) ) ;  // Empty the queue
  specchunk.datatyp = func ;                              // Put function in datatyp
  xQueueSendToFront ( dataqueue, &specchunk, 200 ) ;      // Send to queue (First Out)
  vTaskDelay ( 1 ) ;                                      // Give Play task time to react
}


//**************************************************************************************************
//                                      T F T S E T                                                *
//**************************************************************************************************
// Request to display a segment on TFT.  Version for char* and String parameter.                   *
//**************************************************************************************************
void tftset ( uint16_t inx, const char *str )
{
  if ( inx < TFTSECS )                                  // Segment available on display
  {
    if ( str )                                          // String specified?
    {
      tftdata[inx].str = String ( str ) ;               // Yes, set string
    }
    tftdata[inx].update_req = true ;                    // and request flag
  }
}

void tftset ( uint16_t inx, String& str )
{
  if ( inx < TFTSECS )                                  // Segment available on display
  {
    tftdata[inx].str = str ;                            // Set string
    tftdata[inx].update_req = true ;                    // and request flag
  }
}


//**************************************************************************************************
//                                          U P D A T E N R                                        *
//**************************************************************************************************
// Used by nextPreset because update for preset and playlist number is about the same.             *
// Modify the pnr, handle over- and underflow. handle relative setting.                            *
//**************************************************************************************************
bool updateNr ( int16_t* pnr, int16_t maxnr, int16_t nr, bool relative )
{
  bool res = true ;                                           // Assume positive result

  //ESP_LOGI ( TAG, "updateNr %d <= %d to %d, relative is %d",
  //           *pnr, maxnr, nr, relative ) ;
  if ( relative )                                             // Relative to pnr?
  {
    *pnr += nr ;                                              // Yes, compute new pnr
  }
  else
  {
    *pnr = nr ;                                               // Not relative, set direct pnr
  }
  if ( *pnr < 0 )                                             // Check result
  {
    res = false ;                                             // Negative result, set bad result
    *pnr = maxnr ;                                            // and wrap
  }
  if ( *pnr > maxnr )                                         // Check if result beyond max
  {
    res = false ;                                             // Too high, set bad result
    *pnr = 0 ;                                                // and wrap
  }
  //ESP_LOGI ( TAG, "updateNr result is %d", *pnr ) ;
  return res ;                                                // Return the result
}


//**************************************************************************************************
//                                          N E X T P R E S E T                                    *
//**************************************************************************************************
// Set the preset for the next station.  May be relative.                                          *
//**************************************************************************************************
bool nextPreset ( int16_t pnr, bool relative = false )
{
  //ESP_LOGI ( TAG, "nextpreset called with pnr = %d", pnr ) ;
  if ( ( presetinfo.station_state == ST_STATION ) ||           // In station mode?
       ( presetinfo.station_state == ST_REDIRECT ) )           // or redirect mode?
  {
    presetinfo.station_state = ST_PRESET ;                     // No "next" in station/redirect mode
  }
  if ( presetinfo.station_state == ST_PLAYLIST )               // In playlist mode?
  {
    if ( ! updateNr ( &presetinfo.playlistnr,                  // Yes, next index possible?
                      presetinfo.highest_playlistnr,
                      pnr, relative ) )
    {
      presetinfo.station_state = ST_PRESET ;                   // No, end playlist mode
    }
  }
  if ( presetinfo.station_state == ST_PRESET )                 // In preset mode?
  {
    updateNr ( &presetinfo.preset,                             // Select next preset
               presetinfo.highest_preset,
               pnr, relative ) ;
    if ( ! readhostfrompref ( presetinfo.preset,              // Set host
                              &presetinfo.host,
                              &presetinfo.hsym ) )
    {
      return false ;
    }
    ESP_LOGI ( TAG, "nextPreset is %d", presetinfo.preset ) ;
  }
  return true ;
}


//**************************************************************************************************
//                                          T I M E R 1 0 S E C                                    *
//**************************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                                       *
// If totalcount has not been changed, there is a problem and playing will stop.                   *
// Note that calling timely procedures within this routine or in called functions will             *
// cause a crash!                                                                                  *
//**************************************************************************************************
void IRAM_ATTR timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed for change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  uint32_t        bytesplayed ;                   // Bytes send to MP3 converter

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    bytesplayed = totalcount - oldtotalcount ;    // Number of bytes played in the 10 seconds
    oldtotalcount = totalcount ;                  // Save for comparison in next cycle
    if ( bytesplayed == 0 )                       // Still playing?
    {
      if ( morethanonce > 10 )                    // No! Happened too many times?
      {
        resetreq = true ;                         // Yes, restart
      }
      //if ( datamode & ( PLAYLISTDATA |          // In playlist mode?
      //                  PLAYLISTINIT |
      //                  PLAYLISTHEADER ) )
      //{
      //  playlist_num = 0 ;                      // Yes, end of playlist
      //}
      //if ( ( morethanonce > 0 ) ||              // Happened more than once?
      //     ( playlist_num > 0 ) )               // Or playlist active?
      if ( morethanonce > 0 )                     // Happened more than once?
      {
        datamode = STOPREQD ;                     // Stop player
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      //                                          // Data has been send to MP3 decoder
      // Bitrate in kbits/s is bytesplayed / 10 / 1000 * 8
      mbitrate = ( bytesplayed + 625 ) / 1250 ;   // Measured bitrate, rounded
      morethanonce = 0 ;                          // Data seen, reset failcounter
    }
  }
}


//**************************************************************************************************
//                                          T I M E R 1 0 0                                        *
//**************************************************************************************************
// Called every 100 msec on interrupt level, so must be in IRAM and no lengthy operations          *
// allowed.                                                                                        *
//**************************************************************************************************
void IRAM_ATTR timer100()
{
  sv int16_t   count10sec = 0 ;                   // Counter for activatie 10 seconds process
  sv int16_t   eqcount = 0 ;                      // Counter for equal number of clicks
  sv int16_t   oldclickcount = 0 ;                // To detect difference

  spftrigger = true ;                             // Activate spfuncs
  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
  if ( ( count10sec % 10 ) == 0 )                 // One second over?
  {
    if ( ++timeinfo.tm_sec >= 60 )                // Yes, update number of seconds
    {
      timeinfo.tm_sec = 0 ;                       // Wrap after 60 seconds
      if ( ++timeinfo.tm_min >= 60 )
      {
        timeinfo.tm_min = 0 ;                     // Wrap after 60 minutes
        if ( ++timeinfo.tm_hour >= 24 )
        {
          timeinfo.tm_hour = 0 ;                  // Wrap after 24 hours
        }
      }
    }
    time_req = true ;                             // Yes, show current time request
    if ( ++bltimer == BL_TIME )                   // Time to blank the TFT screen?
    {
      bltimer = 0 ;                               // Yes, reset counter
      blset ( false ) ;                           // Disable TFT (backlight)
    }
  }
  // Handle rotary encoder. Inactivity counter will be reset by encoder interrupt
  if ( enc_inactivity < 36000 )                   // Count inactivity time, but limit to 36000
  {
    enc_inactivity++ ;
  }
  // Now detection of single/double click of rotary encoder switch or ZIPPY B5
  if ( clickcount )                               // Any click?
  {
    if ( oldclickcount == clickcount )            // Yes, stable situation?
    {
      if ( ++eqcount == 6 )                       // Long time stable?
      {
        eqcount = 0 ;
        if ( clickcount > 2 )                     // Triple click?
        {
          tripleclick = true ;                    // Yes, set result
        }
        else if ( clickcount == 2 )               // Double click?
        {
          doubleclick = true ;                    // Yes, set result
        }
        else
        {
          singleclick = true ;                    // Just one click seen
        }
        clickcount = 0 ;                          // Reset number of clicks
      }
    }
    else
    {
      oldclickcount = clickcount ;                // To detect change
      eqcount = 0 ;                               // Not stable, reset count
    }
  }
}


//**************************************************************************************************
//                                          I S R _ I R                                            *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer.         *
// Input is complete after 65 level changes.                                                       *
// Only the last 32 level changes are significant and will be handed over to common data.          *
//**************************************************************************************************
void IRAM_ATTR isr_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  sv uint32_t      ir_locvalue = 0 ;                 // IR code
  sv int           ir_loccount = 0 ;                 // Length of code
  uint32_t         t1, intval ;                      // Current time and interval since last change
  uint32_t         mask_in = 2 ;                     // Mask input for conversion
  uint16_t         mask_out = 1 ;                    // Mask output for conversion

  ir_intcount++ ;                                    // Test IR input.
  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare
  if ( ( intval > 300 ) && ( intval < 800 ) )        // Short pulse?
  {
    ir_locvalue = ir_locvalue << 1 ;                 // Shift in a "zero" bit
    ir_loccount++ ;                                  // Count number of received bits
    ir_0 = ( ir_0 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
  }
  else if ( ( intval > 1400 ) && ( intval < 1900 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
    ir_1 = ( ir_1 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
  }
  else if ( ir_loccount == 65 )                      // Value is correct after 65 level changes
  {
    while ( mask_in )                                // Convert 32 bits to 16 bits
    {
      if ( ir_locvalue & mask_in )                   // Bit set in pattern?
      {
        ir_value |= mask_out ;                       // Set set bit in result
      }
      mask_in <<= 2 ;                                // Shift input mask 2 positions
      mask_out <<= 1 ;                               // Shift output mask 1 position
    }
    ir_loccount = 0 ;                                // Ready for next input
  }
  else
  {
    ir_locvalue = 0 ;                                // Reset decoding
    ir_loccount = 0 ;
  }
}


//**************************************************************************************************
//                                          I S R _ E N C _ S W I T C H                            *
//**************************************************************************************************
// Interrupts received from rotary encoder switch or ZIPPY B5.                                     *
//**************************************************************************************************
void IRAM_ATTR isr_enc_switch()
{
  sv uint32_t     oldtime = 0 ;                            // Time in millis previous interrupt
  sv bool         sw_state ;                               // True is pushed (LOW)
  bool            newstate ;                               // Current state of input signal
  uint32_t        newtime ;                                // Current timestamp
  uint32_t        dtime ;                                  // Time difference with previous interrupt

  newstate = ( digitalRead ( ini_block.enc_sw_pin ) == LOW ) ;
  newtime = xTaskGetTickCount() ;                          // Time of last interrupt
  dtime = ( newtime - oldtime ) & 0xFFFF ;                 // Compute delta
  if ( dtime < 50 )                                        // Debounce
  {
    return ;                                               // Ignore bouncing
  }
  if ( newstate != sw_state )                              // State changed?
  {
    oldtime = newtime ;                                    // Time of change for next compare
    sw_state = newstate ;                                  // Yes, set current (new) state
    if ( !sw_state )                                       // SW released?
    {
      if ( ( dtime ) > 2000 )                              // More than 2 second?
      {
        longclick = true ;                                 // Yes, register longclick
        clickcount = 0 ;                                   // Forget normal count
      }
      else
      {
        clickcount++ ;                                     // Yes, click detected
      }
      enc_inactivity = 0 ;                                 // Not inactive anymore
    }
  }
}

#ifdef ZIPPYB5
  //**************************************************************************************************
  //                                          I S R _ E N C _ T U R N                                *
  //**************************************************************************************************
  // Interrupts received from ZIPPY B5 side switch (clk signal) knob turn.                           *
  //**************************************************************************************************
  void IRAM_ATTR isr_enc_turn()
  {
    sv uint32_t     trig_time = 0 ;                               // For debounce
    uint32_t        new_time ;                                    // New time

    new_time = xTaskGetTickCount() ;                              // Get time
    if ( new_time <= trig_time )                                  // For debounce
    {
      return ;                                                    // No action
    }
    trig_time = new_time + 300 ;                                  // Dead time 
    if ( digitalRead ( ini_block.enc_up_pin ) == LOW )            // Up pin activated?
    {
      rotationcount++ ;
    }
    else if ( digitalRead ( ini_block.enc_dwn_pin ) == LOW )      // Down pin activated?
    {
      rotationcount-- ;
    }
    enc_inactivity = 0 ;                                          // Mark activity
  }
#else
  //**************************************************************************************************
  //                                          I S R _ E N C _ T U R N                                *
  //**************************************************************************************************
  // Interrupts received from rotary encoder (clk signal) knob turn.                                 *
  // The encoder is a Manchester coded device, the outcomes (-1,0,1) of all the previous state and   *
  // actual state are stored in the enc_states[].                                                    *
  // Full_status is a 4 bit variable, the upper 2 bits are the previous encoder values, the lower    *
  // ones are the actual ones.                                                                       *
  // 4 bits cover all the possible previous and actual states of the 2 PINs, so this variable is     *
  // the index enc_states[].                                                                         *
  // No debouncing is needed, because only the valid states produce values different from 0.         *
  // Rotation is 4 if position is moved from one fixed position to the next, so it is devided by 4.  *
  //**************************************************************************************************
  void IRAM_ATTR isr_enc_turn()
  {
    sv uint32_t     old_state = 0x0001 ;                          // Previous state
    sv int16_t      locrotcount = 0 ;                             // Local rotation count
    uint8_t         act_state = 0 ;                               // The current state of the 2 PINs
    uint8_t         inx ;                                         // Index in enc_state
    sv const int8_t enc_states [] =                               // Table must be in DRAM (iram safe)
    { 0,                    // 00 -> 00
      -1,                   // 00 -> 01                           // dt goes HIGH
      1,                    // 00 -> 10
      0,                    // 00 -> 11
      1,                    // 01 -> 00                           // dt goes LOW
      0,                    // 01 -> 01
      0,                    // 01 -> 10
      -1,                   // 01 -> 11                           // clk goes HIGH
      -1,                   // 10 -> 00                           // clk goes LOW
      0,                    // 10 -> 01
      0,                    // 10 -> 10
      1,                    // 10 -> 11                           // dt goes HIGH
      0,                    // 11 -> 00
      1,                    // 11 -> 01                           // clk goes LOW
      -1,                   // 11 -> 10                           // dt goes HIGH
      0                     // 11 -> 11
    } ;
    // Read current state of CLK, DT pin. Result is a 2 bit binary number: 00, 01, 10 or 11.
    act_state = ( digitalRead ( ini_block.enc_clk_pin ) << 1 ) +
                digitalRead ( ini_block.enc_dt_pin ) ;
    inx = ( old_state << 2 ) + act_state ;                        // Form index in enc_states
    locrotcount += enc_states[inx] ;                              // Get delta: 0, +1 or -1
    if ( locrotcount == 4 )
    {
      rotationcount++ ;                                           // Divide by 4
      locrotcount = 0 ;
    }
    else if ( locrotcount == -4 )
    {
      rotationcount-- ;                                           // Divide by 4
      locrotcount = 0 ;
    }
    old_state = act_state ;                                       // Remember current status
    enc_inactivity = 0 ;                                          // Mark activity
  }
#endif

//**************************************************************************************************
//                                S H O W S T R E A M T I T L E                                    *
//**************************************************************************************************
// Show artist and songtitle if present in metadata.                                               *
// Show always if full=true.                                                                       *
// Returns true if title has changed.                                                              *
//**************************************************************************************************
bool showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  static char       oldstreamtitle[150] ;        // Previous title, for compare
  char              streamtitle[150] ;           // Streamtitle from metadata

  if ( strstr ( ml, "StreamTitle=" ) )
  {
    ESP_LOGI ( TAG, "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    ESP_LOGI ( TAG, "%s", ml ) ;
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if ( ( p2 = strstr ( ml, ";" ) ) )          // Search for end of title
    {
      if ( *p1 == '\'' )                        // Surrounded by quotes?
      {
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy ( streamtitle, p1, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else if ( full )
  {
    // Info probably from playlist
    strncpy ( streamtitle, ml, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else
  {
    icystreamtitle = "" ;                       // Unknown type
    return false ;                              // Do not show
  }
  // Save for status request from browser and for MQTT
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    p2 = p1 + 3 ;                               // 2nd part of text at this position
    #ifdef NEXTION
      *p1++ = '\\' ;                            // Found: replace 3 characters by "\r"
      *p1++ = 'r' ;                             // Found: replace 3 characters by "\r"
    #else
      *p1++ = '\n' ;                            // Found: replace 3 characters by newline
    #endif
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  if ( strcmp ( oldstreamtitle,                 // Change in tiltlke?
                streamtitle ) != 0 )
  {
    tftset ( 1, streamtitle ) ;                 // Yes, set screen segment text middle part
    return true ;                               // Return true if tiitle has changed
  }
  return false ;
}


//**************************************************************************************************
//                                    S E T D A T A M O D E                                        *
//**************************************************************************************************
// Change the datamode and show in debug for testing.                                              *
//**************************************************************************************************
void setdatamode ( datamode_t newmode )
{
  //ESP_LOGI ( TAG, "Change datamode from 0x%03X to 0x%03X",
  //           (int)datamode, (int)newmode ) ;
  datamode = newmode ;
}


//**************************************************************************************************
//                                    S T O P _ M P 3 C L I E N T                                  *
//**************************************************************************************************
// Disconnect from the server.                                                                     *
//**************************************************************************************************
void stop_mp3client ()
{
  queueToPt ( QSTOPSONG ) ;                        // Queue a request to stop the song
  while ( mp3client && mp3client->connected() )    // Client active and connected?
  {
    ESP_LOGI ( TAG, "Stopping client" ) ;          // Yes, stop connection to host
    mp3client->close() ;                           // Close connection
    mp3client->abort() ;                           // Necesary to prevent crash
    vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;
  }
}


//**************************************************************************************************
//                                    C O N N E C T T O H O S T                                    *
//**************************************************************************************************
// Connect to the Internet radio server specified by presetinfo and send the GET request.          *
//**************************************************************************************************
bool connecttohost()
{
  int         inx ;                                  // Position of ":" in hostname
  uint16_t    port = 80 ;                            // Port number for host
  String      extension = "/" ;                      // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext ;                            // Host without extension and portnumber
  String      auth  ;                                // For basic authentication
  char        getreq[500] ;                          // GET command for MP3 host
  int         retrycount = 0 ;                       // Count for connect
  size_t      len ;                                  // Length of GET request
  bool        res = false ;                          // Function result, assume bad result

  stop_mp3client() ;                                 // Disconnect if still connected
  chomp ( presetinfo.host ) ;                        // Do some filtering
  hostwoext = presetinfo.host ;                      // Assume host does not have extension
  ESP_LOGI ( TAG, "Connect to host %s",
             presetinfo.host.c_str() ) ;
  tftset ( 0, NAME ) ;                               // Set screen segment text top line
  tftset ( 1, "" ) ;                                 // Clear song and artist
  displaytime ( "" ) ;                               // Clear time on TFT screen
  setdatamode ( INIT ) ;                             // Start default in INIT mode
  chunked = false ;                                  // Assume not chunked
  if ( presetinfo.host.endsWith ( ".m3u" ) )         // Is it an m3u playlist?
  {
    presetinfo.station_state = ST_PLAYLIST ;         // Yes, change station state
    presetinfo.playlisthost = presetinfo.host ;      // Save copy of playlist URL
    setdatamode ( PLAYLISTINIT ) ;                   // Yes, start in PLAYLIST mode
    ESP_LOGI ( TAG, "Playlist request, entry %d",
               presetinfo.playlistnr ) ;
  }
  // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
  inx = presetinfo.host.indexOf ( "/" ) ;            // Search for begin of extension
  if ( inx > 0 )                                     // Is there an extension?
  {
    extension = hostwoext.substring ( inx ) ;        // Yes, change the default
    hostwoext = hostwoext.substring ( 0, inx ) ;     // Host without extension
  }
  // In the host there may be a portnumber
  inx = hostwoext.indexOf ( ":" ) ;                  // Search for separator
  if ( inx >= 0 )                                    // Portnumber available?
  {
    port = hostwoext.substring ( inx + 1 ).toInt() ; // Get portnumber as integer
    hostwoext = hostwoext.substring ( 0, inx ) ;     // Host without portnumber
  }
  //ESP_LOGI ( TAG, "Connect to %s on port %d, extension %s",
  //           hostwoext.c_str(), port, extension.c_str() ) ;
  if ( mp3client->connect ( hostwoext.c_str(), port ) )
  {
    if ( nvssearch ( "basicauth" ) )                 // Does "basicauth" exists?
    {
      auth = nvsgetstr ( "basicauth" ) ;             // Use basic authentication?
      if ( auth != "" )                              // Should be user:passwd
      { 
         auth = base64::encode ( auth.c_str() ) ;    // Encode
         auth = String ( "Authorization: Basic " ) +
                auth + String ( "\r\n" ) ;
      }
    }
    while ( mp3client->disconnected() )              // Wait for connect
    {
      if ( retrycount++ > 50 )                       // For max 5 seconds
      {
        mp3client->stop() ;                          // No connect, stop
        break ;                                      //
      }
      vTaskDelay ( 100 / portTICK_PERIOD_MS ) ;
    }
    if ( mp3client->connected() )
    {
      sprintf ( getreq, "GET %s HTTP/1.0\r\n"
                        "Host: %s\r\n"
                        "Icy-MetaData: 1\r\n"
                        "%s"                              // Auth
                        "Connection: close\r\n\r\n",      // Close when finished
                extension.c_str(),
                hostwoext.c_str(),
                auth.c_str() ) ;
      ESP_LOGI ( TAG, "send GET command" ) ;
      if ( mp3client->canSend() )
      {
        len = strlen ( getreq ) ;                         // Length of string to send
        res = mp3client->write ( getreq, len ) == len  ;  // Send GET request, set result
      }
    }
  }
  else
  {
    ESP_LOGE ( TAG, "Request %s failed!",                    // Report error
               presetinfo.host.c_str() ) ;
  }
  return res ;
}


//**************************************************************************************************
//                                      S S C O N V                                                *
//**************************************************************************************************
// Convert an array with 4 "synchsafe integers" to a number.                                       *
// There are 7 bits used per byte.                                                                 *
//**************************************************************************************************
uint32_t ssconv ( const uint8_t* bytes )
{
  uint32_t res = 0 ;                                      // Result of conversion
  uint8_t  i ;                                            // Counter number of bytes to convert

  for ( i = 0 ; i < 4 ; i++ )                             // Handle 4 bytes
  {
    res = res * 128 + bytes[i] ;                          // Convert next 7 bits
  }
  return res ;                                            // Return the result
}

#ifdef ETHERNET
//**************************************************************************************************
//                                      E T H E V E N T                                            *
//**************************************************************************************************
// Will be executed on ethernet driver events.                                                     *
//**************************************************************************************************
void EthEvent ( WiFiEvent_t event )
{
  const char* fd = "" ;                               // Full duplex or not as a string

  switch ( event )                                    // What event?
  {
    case ARDUINO_EVENT_ETH_START :
      ESP_LOGI ( TAG, "ETH Started" ) ;               // Driver started
      ETH.setHostname ( NAME ) ;                      // Set the eth hostname now
      break ;
    case ARDUINO_EVENT_ETH_CONNECTED :
      ESP_LOGI ( TAG, "ETH cable connected" ) ;       // We have a connection
      break ;
    case ARDUINO_EVENT_ETH_GOT_IP :
      if ( ETH.fullDuplex() )                         // IP received from DHCP
      {
        fd = ", FULL_DUPLEX" ;                        // It is full duplex
      }
      ipaddress = ETH.localIP().toString() ;          // Remember for display
      ESP_LOGI ( TAG, "IPv4: %s, %d Mbps%s",          // Show status
                 ipaddress.c_str(),
                 ETH.linkSpeed(),
                 fd ) ;
      eth_connected = true ;                          // Set global flag: connection OK
      break ;
    case ARDUINO_EVENT_ETH_DISCONNECTED :
      ESP_LOGI ( TAG, "ETH cable disconnected" ) ;    // We have a disconnection
      eth_connected = false ;                         // Clear the global flag
      break ;
    case ARDUINO_EVENT_ETH_STOP :
      ESP_LOGI ( TAG, "ETH Stopped" ) ;
      eth_connected = false ;
      break ;
    default :
      ESP_LOGI ( TAG, "ETH event %d", (int)event ) ;  // Unknown event
      break ;
  }
}

//**************************************************************************************************
//                                       C O N N E C T E T H                                       *
//**************************************************************************************************
// Connect to Ethernet.                                                                            *
// If connection fails, the function returns false.                                                *
//**************************************************************************************************
bool connectETH()
{
  const char* pIP ;                                     // Pointer to IP address
  bool        res ;                                     // Result of connect
  int         tries = 0 ;                               // Counter for wait time
  
  ESP_LOGI ( TAG, "ETH pins %d, %d and %d",
             ini_block.eth_power_pin,
             ini_block.eth_mdc_pin,
             ini_block.eth_mdio_pin ) ;
  res =  ETH.begin ( ETH_ADDR, ini_block.eth_power_pin, // Start Ethernet driver
                     ini_block.eth_mdc_pin,
                     ini_block.eth_mdio_pin,
                     ETH_TYPE, ETH_CLK_MODE ) ;
  while ( res & ( ! eth_connected ) )                   // Wait for connect
  {
    vTaskDelay ( 1000 / portTICK_PERIOD_MS ) ;
    if ( tries++ == 10 )                                // Limit wait time
    {
      res = false ;                                     // No luck
    }
  }
  pIP = ipaddress.c_str() ;                             // As c-string
  ESP_LOGI ( TAG, "IP = %s", pIP ) ;
  tftlog ( "IP = " ) ;                                  // Show IP
  tftlog ( pIP, true ) ;
  #ifdef NEXTION
    dsp_println ( "\f" ) ;                              // Select new page if NEXTION 
  #endif
  return res ;                                          // Return result of connection
}

#else
//**************************************************************************************************
//                                       C O N N E C T W I F I                                     *
//**************************************************************************************************
// Connect to WiFi using the SSID's available in wifiMulti.                                        *
// If only one AP if found in preferences (i.e. wifi_00) the connection is made without            *
// using wifiMulti.                                                                                *
// If connection fails, an AP is created and the function returns false.                           *
//**************************************************************************************************
bool connectwifi()
{
  bool        localAP = false ;                         // True if only local AP is left
  const char* pIP ;                                     // Pointer to IP address
  WifiInfo_t  winfo ;                                   // Entry from wifilist

  WiFi.mode ( WIFI_STA ) ;                              // This ESP is a station
  WiFi.disconnect ( true ) ;                            // After restart the router could
  WiFi.softAPdisconnect ( true ) ;                      // still keep the old connection
  vTaskDelay ( 1000 / portTICK_PERIOD_MS ) ;            // Silly things to start connection
  WiFi.mode ( WIFI_STA ) ;
  vTaskDelay ( 1000 / portTICK_PERIOD_MS ) ;
  if ( wifilist.size()  )                               // Any AP defined?
  {
    if ( wifilist.size() == 1 )                         // Just one AP defined in preferences?
    {
      winfo = wifilist[0] ;                             // Get this entry
      ESP_LOGI ( TAG, "Try WiFi \"%s\"", winfo.ssid ) ; // Message to show during WiFi connect
      WiFi.begin ( winfo.ssid, winfo.passphrase ) ;     // Connect to single SSID found in wifi_xx
    }
    else                                                // More AP to try
    {
      wifiMulti.run() ;                                 // Connect to best network
    }
    if (  WiFi.waitForConnectResult() != WL_CONNECTED ) // Try to connect
    {
      localAP = true ;                                  // Error, setup own AP
    }
  }
  else
  {
    localAP = true ;                                    // Not even a single AP defined
  }
  if ( localAP )                                        // Must setup local AP?
  {
    ESP_LOGI ( TAG, "WiFi Failed!  Trying to setup AP with"
               " name %s and password %s.",
               NAME, NAME ) ;
    WiFi.disconnect ( true ) ;                          // After restart the router could
    WiFi.softAPdisconnect ( true ) ;                    // still keep the old connection
    if ( ! WiFi.softAP ( NAME, NAME ) )                 // This ESP will be an AP
    {
      ESP_LOGE ( TAG, "AP failed" ) ;                   // Setup of AP failed
    }
    ipaddress = String ( "192.168.4.1" ) ;              // Fixed IP address
  }
  else
  {
    tftlog ( "SSID = " ) ;                              // Show SSID on display
    tftlog ( WiFi.SSID().c_str(), true ) ;
    ESP_LOGI ( TAG, "SSID = %s",                        // Format string with SSID connected to
               WiFi.SSID().c_str() ) ;
    ipaddress = WiFi.localIP().toString() ;             // Form IP address
  }
  pIP = ipaddress.c_str() ;                             // As c-string
  ESP_LOGI ( TAG, "IP = %s", pIP ) ;
  tftlog ( "IP = " ) ;                                  // Show IP
  tftlog ( pIP, true ) ;
  #ifdef NEXTION
    vTaskDelay ( 2000 / portTICK_PERIOD_MS ) ;          // Show for some time
    dsp_println ( "\f" ) ;                              // Select new page if NEXTION 
  #endif
  return ( localAP == false ) ;                         // Return result of connection
}
#endif

#ifdef ENABLEOTA
//**************************************************************************************************
//                                           O T A S T A R T                                       *
//**************************************************************************************************
// Update via WiFi/Ethernet has been started by Arduino IDE or PlatformIO.                         *
//**************************************************************************************************
void otastart()
{
  const char* p = "OTA update Started" ;

  ESP_LOGI ( TAG, "%s", p ) ;                      // Show event for debug
  tftset ( 2, p ) ;                                // Set screen segment bottom part
  mp3client->abort() ;                             // Stop client
  timerAlarmDisable ( timer ) ;                    // Disable the timer
  disableCore0WDT() ;                              // Disable watchdog core 0
  disableCore1WDT() ;                              // Disable watchdog core 1
  queueToPt ( QSTOPTASK ) ;                        // Queue a request to stop the song
}


//**************************************************************************************************
//                                           O T A E R R O R                                       *
//**************************************************************************************************
// Update via WiFi has an error.                                                                   *
//**************************************************************************************************
void otaerror ( ota_error_t error)
{
  ESP_LOGE ( TAG, "OTA error %d", error ) ;
  tftset ( 2, "OTA error!" ) ;                        // Set screen segment bottom part
}
#endif                                                // ENABLEOTA


//**************************************************************************************************
//                                  R E A D H O S T F R O M P R E F                                *
//**************************************************************************************************
// Read the mp3 host from the preferences specified by the parameter.                              *
// The host will be returned.                                                                      *
// We search for "preset_x" or "preset_xx" or "preset_xxx".                                        *
//**************************************************************************************************
bool readhostfrompref ( int16_t preset, String* host, String* hsym )
{
  char           tkey[12] ;                            // Key as an array of char
  int            inx ;                                 // Position of comment in preset

  sprintf ( tkey, "preset_%d", preset ) ;              // Form the search key
  if ( !nvssearch ( tkey ) )                           // Does _x[x[x]] exists?
  {
    sprintf ( tkey, "preset_%03d", preset ) ;          // Form new search key
    if ( !nvssearch ( tkey ) )                         // Does _xxx exists?
    {
      sprintf ( tkey, "preset_%02d", preset ) ;        // Form new search key
    }
    if ( !nvssearch ( tkey ) )                         // Does _xx exists?
    {
      *host = String ( "" ) ;                          // Not found
      *hsym = *host ;                                  // Symbolic name also unknown
      return false ;
    }
  }
  // Get the contents
  *host = nvsgetstr ( tkey ) ;                         // Get the station
  if ( hsym )                                          // Symbolic name parameter wanted?
  {
    *hsym = *host ;                                    // Symbolic name defailt equal to preset
    // See if comment if available.  Otherwise the preset itself.
    inx = hsym->indexOf ( "#" ) ;                      // Get position of "#"
    if ( inx > 0 )                                     // Hash sign present?
    {
      hsym->remove ( 0, inx + 1 ) ;                    // Yes, remove non-comment part
    }
    chomp ( *hsym ) ;                                  // Remove garbage from description
  }
  return true ;
}


//**************************************************************************************************
//                                       R E A D P R O G B U T T O N S                             *
//**************************************************************************************************
// Read the preferences for the programmable input pins and the touch pins.                        *
//**************************************************************************************************
void readprogbuttons()
{
  char        mykey[20] ;                                   // For numerated key
  int8_t      pinnr ;                                       // GPIO pinnumber to fill
  int         i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry

  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan for all programmable pins
  {
    //ESP_LOGI ( TAG, "Check programmable GPIO_%02d",pinnr ) ;
    sprintf ( mykey, "gpio_%02d", pinnr ) ;                 // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      //ESP_LOGI ( TAG, "Pin in NVS" ) ;
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !progpin[i].reserved )                         // Do not use reserved pins
        {
          progpin[i].avail = true ;                         // This one is active now
          progpin[i].command = val ;                        // Set command
          ESP_LOGI ( TAG, "gpio_%02d will execute %s",      // Show result
                     pinnr, val.c_str() ) ;
        }
      }
    }
  }
  // Now for the touch pins 0..9, identified by their GPIO pin number
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan for all programmable pins
  {
    sprintf ( mykey, "touch_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !touchpin[i].reserved )                        // Do not use reserved pins
        {
          touchpin[i].avail = true ;                        // This one is active now
          touchpin[i].command = val ;                       // Set command
          //pinMode ( touchpin[i].gpio,  INPUT ) ;          // Free floating input
          ESP_LOGI ( TAG, "touch_%02d will execute %s",     // Show result
                     i, val.c_str() ) ;
          ESP_LOGI ( TAG, "Level is now %d",
                     touchRead ( pinnr ) ) ;                // Sample the pin
        }
        else
        {
          ESP_LOGE ( TAG, "touch_%02d pin (GPIO%02d) is reserved for I/O!",
                     i, pinnr ) ;
        }
      }
    }
  }
}


//**************************************************************************************************
//                                       R E S E R V E P I N                                       *
//**************************************************************************************************
// Set I/O pin to "reserved".                                                                      *
// The pin will be unavailable for a programmable function.                                        *
//**************************************************************************************************
void reservepin ( int8_t rpinnr )
{
  uint8_t i = 0 ;                                           // Index in progpin/touchpin array
  int8_t  pin ;                                             // Pin number in progpin array

  while ( ( pin = progpin[i].gpio ) >= 0 )                  // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      if ( progpin[i].reserved )                            // Already reserved?
      {
        ESP_LOGE ( TAG, "Pin %d is already reserved!", rpinnr ) ;
      }
      //ESP_LOGE ( TAG, "GPIO%02d unavailabe for 'gpio_'-command",
      //           pin ) ;
      progpin[i].reserved = true ;                          // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
  // Also reserve touchpin numbers
  i = 0 ;
  while ( ( pin = touchpin[i].gpio ) >= 0 )                 // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      //ESP_LOGI ( TAG, "GPIO%02d unavailabe for touch command",
      //           pin ) ;
      touchpin[i].reserved = true ;                         // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
}


//**************************************************************************************************
//                                       R E A D I O P R E F S                                     *
//**************************************************************************************************
// Scan the preferences for IO-pin definitions.                                                    *
//**************************************************************************************************
void readIOprefs()
{
  struct iosetting
  {
    const char* gname ;                                   // Name in preferences
    int8_t*     gnr ;                                     // Address of target GPIO pin number
    int8_t      pdefault ;                                // Default pin
  };
  struct iosetting klist[] = {                            // List of I/O related keys
      { "pin_ir",        &ini_block.ir_pin,           -1 },
      { "pin_enc_clk",   &ini_block.enc_clk_pin,      -1 }, // Rotary encoder CLK
      { "pin_enc_dt",    &ini_block.enc_dt_pin,       -1 }, // Rotary encoder DT
      { "pin_enc_up",    &ini_block.enc_up_pin,       -1 }, // ZIPPY B5 side switch up
      { "pin_enc_dwn",   &ini_block.enc_dwn_pin,      -1 }, // ZIPPY B5 side switch down
      { "pin_enc_sw",    &ini_block.enc_sw_pin,       -1 },
      { "pin_tft_cs",    &ini_block.tft_cs_pin,       -1 }, // Display SPI version
      { "pin_tft_dc",    &ini_block.tft_dc_pin,       -1 }, // Display SPI version
      { "pin_tft_scl",   &ini_block.tft_scl_pin,      -1 }, // Display I2C version
      { "pin_tft_sda",   &ini_block.tft_sda_pin,      -1 }, // Display I2C version
      { "pin_tft_bl",    &ini_block.tft_bl_pin,       -1 }, // Display backlight
      { "pin_tft_blx",   &ini_block.tft_blx_pin,      -1 }, // Display backlight (inversed logic)
      { "pin_nxt_rx",    &ini_block.nxt_rx_pin,       -1 }, // NEXTION input pin
      { "pin_nxt_tx",    &ini_block.nxt_tx_pin,       -1 }, // NEXTION output pin
      { "pin_sd_cs",     &ini_block.sd_cs_pin,        -1 }, // SD card select
      { "pin_sd_detect", &ini_block.sd_detect_pin,    -1 }, // SD card detect
    #if defined(DEC_VS1053) || defined(DEC_VS1003)
      { "pin_vs_cs",     &ini_block.vs_cs_pin,        -1 }, // VS1053 pins
      { "pin_vs_dcs",    &ini_block.vs_dcs_pin,       -1 },
      { "pin_vs_dreq",   &ini_block.vs_dreq_pin,      -1 },
    #endif
      { "pin_shutdown",  &ini_block.shutdown_pin,     -1 }, // Amplifier shut-down pin
      { "pin_shutdownx", &ini_block.shutdownx_pin,    -1 }, // Amplifier shut-down pin (inversed logic)
    #ifdef DEC_HELIX
     #ifndef DEC_HELIX_INT
      #ifdef DEC_HELIX_SPDIF
      { "pin_i2s_spdif", &ini_block.i2s_spdif_pin,    -1 },
      #else
      { "pin_i2s_bck",   &ini_block.i2s_bck_pin,      -1 }, // I2S interface pins
      { "pin_i2s_lck",   &ini_block.i2s_lck_pin,      -1 },
      { "pin_i2s_din",   &ini_block.i2s_din_pin,      -1 },
      #endif
     #endif
    #endif
    #ifdef ETHERNET
      { "pin_spi_sck",   &ini_block.spi_sck_pin,      -1 },
      { "pin_spi_miso",  &ini_block.spi_miso_pin,     -1 },
      { "pin_spi_mosi",  &ini_block.spi_mosi_pin,     -1 },
      { "pin_eth_mdc",   &ini_block.eth_mdc_pin,      23 },
      { "pin_eth_mdio",  &ini_block.eth_mdio_pin,     18 },
      { "pin_eth_power", &ini_block.eth_power_pin,    16 },
    #else
      { "pin_spi_sck",   &ini_block.spi_sck_pin,      18 }, // Note: different for AI Audio kit (14)
      { "pin_spi_miso",  &ini_block.spi_miso_pin,     19 }, // Note: different for AI Audio kit (2)
      { "pin_spi_mosi",  &ini_block.spi_mosi_pin,     23 }, // Note: different for AI Audio kit (15)
    #endif
      { NULL,            NULL,                        0  }  // End of list
  } ;
  int         i ;                                         // Loop control
  int         count = 0 ;                                 // Number of keys found
  String      val ;                                       // Contents of preference entry
  int8_t      ival ;                                      // Value converted to integer
  int8_t*     p ;                                         // Points to variable

  for ( i = 0 ; klist[i].gname ; i++ )                    // Loop trough all I/O related keys
  {
    p = klist[i].gnr ;                                    // Point to target variable
    ival = klist[i].pdefault ;                            // Assume pin number to be the default
    if ( nvssearch ( klist[i].gname ) )                   // Does it exist?
    {
      val = nvsgetstr ( klist[i].gname ) ;                // Read value of key
      if ( val.length() )                                 // Parameter in preference?
      {
        count++ ;                                         // Yes, count number of filled keys
        ival = val.toInt() ;                              // Convert value to integer pinnumber
        reservepin ( ival ) ;                             // Set pin to "reserved"
      }
    }
    *p = ival ;                                           // Set pinnumber in ini_block
    if ( ival >= 0 )                                      // Only show configured pins
    {
      ESP_LOGI ( TAG, "'%-13s' set to %d",      // Show result
                 klist[i].gname,
                 ival ) ;
    }
  }
}


//**************************************************************************************************
//                                       R E A D P R E F S                                         *
//**************************************************************************************************
// Read the preferences and interpret the commands.                                                *
// If output == true, the key / value pairs are returned to the caller as a String.                *
//**************************************************************************************************
String readprefs ( bool output )
{
  uint16_t    i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry
  String      cmd ;                                         // Command for analyzCmd
  String      outstr = "" ;                                 // Outputstring
  char*       key ;                                         // Point to nvskeys[i]
  uint16_t    last2char = 0 ;                               // To detect paragraphs
  int         presetnr ;                                    // Preset number
 
  presetinfo.highest_preset = 0 ;                           // Number of presets may be shorter
  for ( i = 0 ; i < MAXKEYS ; i++ )                         // Loop trough all available keys
  {
    key = nvskeys[i] ;                                      // Examine next key
    //ESP_LOGI ( TAG, "Key[%d] is %s", i, key ) ;
    if ( *key == '\0' ) break ;                             // Stop on end of list
    val = nvsgetstr ( key ) ;                               // Read value of this key
    cmd = String ( key ) +                                  // Yes, form command
          String ( " = " ) +
          val ;
    if ( cmd.startsWith ( "preset_") )                      // Preset definition?
    {
      presetnr = atoi ( key + 7 ) ;                         // Yes, get preset number
      if ( presetnr > presetinfo.highest_preset )         
      {
        presetinfo.highest_preset = presetnr ;              // Found new max
      }
    }
    if ( output )
    {
      if ( ( i > 0 ) &&
           ( *(uint16_t*)key != last2char ) )               // New paragraph?
      {
        outstr += String ( "#\n" ) ;                        // Yes, add separator
      }
      last2char = *(uint16_t*)key ;                         // Save 2 chars for next compare
      outstr += String ( key ) +                            // Add to outstr
                String ( " = " ) +
                val +
                String ( "\n" ) ;                           // Add newline
    }
    else
    {
      analyzeCmd ( cmd.c_str() ) ;                          // Analyze it
    }
  }
  if ( i == 0 )                                             // Any key seen?
  {
    outstr = String ( "No preferences found.\n"
                      "Use defaults and edit options first.\n" ) ;
  }
  return outstr ;
}


//**************************************************************************************************
//                                    M Q T T R E C O N N E C T                                    *
//**************************************************************************************************
// Reconnect to broker.                                                                            *
//**************************************************************************************************
bool mqttreconnect()
{
  static uint32_t retrytime = 0 ;                         // Limit reconnect interval
  bool            res = false ;                           // Connect result
  char            clientid[20] ;                          // Client ID
  char            subtopic[60] ;                          // Topic to subscribe

  if ( ( millis() - retrytime ) < 5000 )                  // Don't try to frequently
  {
    return res ;
  }
  retrytime = millis() ;                                  // Set time of last try
  if ( mqttcount > MAXMQTTCONNECTS )                      // Tried too much?
  {
    mqtt_on = false ;                                     // Yes, switch off forever
    return res ;                                          // and quit
  }
  mqttcount++ ;                                           // Count the retries
  ESP_LOGI ( TAG, "(Re)connecting number %d to MQTT %s",  // Show some debug info
             mqttcount,
             ini_block.mqttbroker.c_str() ) ;
  sprintf ( clientid, "%s-%04d",                          // Generate client ID
            NAME, (int) random ( 10000 ) % 10000 ) ;
  res = mqttclient.connect ( clientid,                    // Connect to broker
                             ini_block.mqttuser.c_str(),
                             ini_block.mqttpasswd.c_str()
                           ) ;
  if ( res )
  {
    sprintf ( subtopic, "%s/%s",                          // Add prefix to subtopic
              ini_block.mqttprefix.c_str(),
              MQTT_SUBTOPIC ) ;
    res = mqttclient.subscribe ( subtopic ) ;             // Subscribe to MQTT
    if ( !res )
    {
      ESP_LOGE ( TAG, "MQTT subscribe failed!" ) ;        // Failure
    }
    mqttpub.trigger ( MQTT_IP ) ;                         // Publish own IP
  }
  else
  {
    ESP_LOGE ( TAG, "MQTT connection failed, rc=%d",
               mqttclient.state() ) ;

  }
  return res ;
}


//**************************************************************************************************
//                                    O N M Q T T M E S S A G E                                    *
//**************************************************************************************************
// Executed when a subscribed message is received.                                                 *
// Note that message is not delimited by a '\0'.                                                   *
// Note that cmd buffer is shared with serial input.                                               *
//**************************************************************************************************
void onMqttMessage ( char* topic, byte* payload, unsigned int len )
{
  const char*  reply ;                                // Result from analyzeCmd

  if ( strstr ( topic, MQTT_SUBTOPIC ) )              // Check on topic, maybe unnecessary
  {
    if ( len >= sizeof(cmd) )                         // Message may not be too long
    {
      len = sizeof(cmd) - 1 ;
    }
    strncpy ( cmd, (char*)payload, len ) ;            // Make copy of message
    cmd[len] = '\0' ;                                 // Take care of delimeter
    ESP_LOGI ( TAG, "MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd ) ;
    reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
    ESP_LOGI ( TAG, "%s", reply ) ;                   // Result for debugging
  }
}


//**************************************************************************************************
//                                     S C A N S E R I A L                                         *
//**************************************************************************************************
// Listen to commands on the Serial inputline.                                                     *
//**************************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  const char*   reply = "" ;                     // Reply string from analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        reply = analyzeCmd ( cmd ) ;             // Analyze command and handle it
        log_printf ( "%s", reply ) ;             // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}

#ifdef NEXTION 
//**************************************************************************************************
//                                     S C A N S E R I A L 2                                       *
//**************************************************************************************************
// Listen to commands on the 2nd Serial inputline (NEXTION).                                       *
//**************************************************************************************************
void scanserial2()
{
  static String  serialcmd ;                       // Command from Serial input
  char           c ;                               // Input character
  const char*    reply = "" ;                      // Reply string from analyzeCmd
  uint16_t       len ;                             // Length of input string
  static uint8_t ffcount = 0 ;                     // Counter for 3 tmes "0xFF"

  if ( nxtserial )                                 // NEXTION active?
  {
    while ( nxtserial->available() )               // Yes, any input seen?
    {
      c =  (char)nxtserial->read() ;               // Yes, read the next input character
      len = serialcmd.length() ;                   // Get the length of the current string
      if ( c == 0xFF )                             // End of command?
      {
        if ( ++ffcount < 3 )                       // 3 times FF?
        {
          continue ;                               // No, continue to read
        }
        ffcount = 0 ;                              // For next command
        if ( len )
        {
          strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
          ESP_LOGI ( TAG, "NEXTION command seen %02X %s",
                     cmd[0], cmd + 1 ) ;
          if ( cmd[0] == 0x70 )                    // Button pressed?
          { 
            reply = analyzeCmd ( cmd + 1 ) ;       // Analyze command and handle it
            ESP_LOGI ( TAG, "%s", reply ) ;        // Result for debugging
          }
          serialcmd = "" ;                         // Prepare for new command
        }
      }
      else if ( c >= ' ' )                         // Only accept useful characters
      {
        serialcmd += c ;                           // Add to the command
      }
      if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
      {
        serialcmd = "" ;                           // Too long, reset
      }
    }
  }
}
#else
#define scanserial2()                              // Empty version if no NEXTION
#endif


//**************************************************************************************************
//                                     S C A N D I G I T A L                                       *
//**************************************************************************************************
// Scan digital inputs.                                                                            *
//**************************************************************************************************
void  scandigital()
{
  static uint32_t oldmillis = 5000 ;                        // To compare with current time
  int             i ;                                       // Loop control
  int8_t          pinnr ;                                   // Pin number to check
  bool            level ;                                   // Input level
  const char*     reply ;                                   // Result of analyzeCmd
  int16_t         tlevel ;                                  // Level found by touch pin
  const int16_t   THRESHOLD = 30 ;                          // Threshold or touch pins

  if ( ( millis() - oldmillis ) < 100 )                     // Debounce
  {
    return ;
  }
  oldmillis = millis() ;                                    // 100 msec over
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan all inputs
  {
    if ( !progpin[i].avail || progpin[i].reserved )         // Skip unused and reserved pins
    {
      continue ;
    }
    level = ( digitalRead ( pinnr ) == HIGH ) ;             // Sample the pin
    if ( level != progpin[i].cur )                          // Change seen?
    {
      progpin[i].cur = level ;                              // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        ESP_LOGI ( TAG, "GPIO_%02d is now LOW, execute %s",
                   pinnr, progpin[i].command.c_str() ) ;
        reply = analyzeCmd ( progpin[i].command.c_str() ) ; // Analyze command and handle it
        ESP_LOGI ( TAG, "%s", reply ) ;                     // Result for debugging
      }
    }
  }
  // Now for the touch pins
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan all inputs
  {
    if ( !touchpin[i].avail || touchpin[i].reserved )       // Skip unused and reserved pins
    {
      continue ;
    }
    tlevel = ( touchRead ( pinnr ) ) ;                      // Sample the pin
    level = ( tlevel >= THRESHOLD ) ;                       // True if below threshold
    if ( level )                                            // Level HIGH?
    {
      touchpin[i].count = 0 ;                               // Reset count number of times
    }
    else
    {
      if ( ++touchpin[i].count < 3 )                        // Count number of times LOW
      {
        level = true ;                                      // Not long enough: handle as HIGH
      }
    }
    if ( level != touchpin[i].cur )                         // Change seen?
    {
      touchpin[i].cur = level ;                             // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        ESP_LOGI ( TAG, "TOUCH_%02d is now %d ( < %d ), execute %s",
                   pinnr, tlevel, THRESHOLD,
                   touchpin[i].command.c_str() ) ;
        reply = analyzeCmd ( touchpin[i].command.c_str() ); // Analyze command and handle it
        ESP_LOGI ( TAG, "%s", reply ) ;                     // Result for debugging
      }
    }
  }
}


//**************************************************************************************************
//                                     S C A N I R                                                 *
//**************************************************************************************************
// See if IR input is available.  Execute the programmed command.                                  *
//**************************************************************************************************
void scanIR()
{
  char        mykey[20] ;                                   // For numerated key
  String      val ;                                         // Contents of preference entry
  const char* reply ;                                       // Result of analyzeCmd

  if ( ir_value )                                           // Any input?
  {
    sprintf ( mykey, "ir_%04X", ir_value ) ;                // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      ESP_LOGI ( TAG, "IR code %04X received. Will execute %s",
                 ir_value, val.c_str() ) ;
      reply = analyzeCmd ( val.c_str() ) ;                  // Analyze command and handle it
      ESP_LOGI ( TAG, "%s", reply ) ;                       // Result for debugging
    }
    else
    {
      ESP_LOGI ( TAG, "IR code %04X received, but not found in preferences!  Timing %d/%d",
                 ir_value, ir_0, ir_1 ) ;
    }
    ir_value = 0 ;                                          // Reset IR code received
  }
}


#ifndef ETHERNET
//**************************************************************************************************
//                                           M K _ L S A N                                         *
//**************************************************************************************************
// Make al list of acceptable networks in preferences.                                             *
// Will be called only once by setup().                                                            *
// The result will be stored in wifilist.                                                          *
// Not that the last found SSID and password are kept in common data.  If only one SSID is         *
// defined, the connect is made without using wifiMulti.  In this case a connection will           *
// be made even if de SSID is hidden.                                                              *
//**************************************************************************************************
void  mk_lsan()
{
  int16_t     i ;                                        // Loop control
  char        key[10] ;                                  // For example: "wifi_03"
  String      buf ;                                      // "SSID/password"
  String      lssid, lpw ;                               // Last read SSID and password from nvs
  int         inx ;                                      // Place of "/"
  WifiInfo_t  winfo ;                                    // Element to store in list

  ESP_LOGI ( TAG, "Create list with acceptable WiFi networks" ) ;
  for ( i = -1 ; i < 100 ; i++ )                         // Examine FIXEDWIFI, wifi_00 .. wifi_99
  {
    buf = String ( "" ) ;                                // Clear buffer with ssid/passwd
    if ( i == -1 )                                       // Examine FIXEDWIFI if defined
    {
      if ( *fixedwifi )                                  // FIXEDWIFI set and not empty?
      {
        buf = String ( fixedwifi ) ;
      }
    }
    else
    {
      sprintf ( key, "wifi_%02d", i ) ;                  // Form key in preferences
      if ( nvssearch ( key  ) )                          // Does it exists?
      {
        buf = nvsgetstr ( key ) ;                        // Get the contents, like "ssid/password"
      }
    }
    inx = buf.indexOf ( "/" ) ;                          // Find separator between ssid and password
    if ( inx > 0 )                                       // Separator found?
    {
      lpw = buf.substring ( inx + 1 ) ;                  // Isolate password
      lssid = buf.substring ( 0, inx ) ;                 // Holds SSID now
      winfo.ssid = (char*)lssid.c_str() ;                       // Set ssid of new element for wifilist
      winfo.passphrase = (char*)lpw.c_str() ;
      wifilist.push_back ( winfo ) ;                     // Add to list
      wifiMulti.addAP ( winfo.ssid,                      // Add to wifi acceptable network list
                        winfo.passphrase ) ;
    }
  }
}
#endif

//**************************************************************************************************
//                                     G E T R A D I O S T A T U S                                 *
//**************************************************************************************************
// Return preset-, tone- and volume status.                                                        *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
String getradiostatus()
{
  return String ( "preset=" ) +                          // Add preset setting
         String ( presetinfo.host ) +
         String ( "\nvolume=" ) +                        // Add volume setting
         String ( ini_block.reqvol ) +
         String ( "\ntoneha=" ) +                        // Add tone setting HA
         String ( ini_block.rtone[0] ) +
         String ( "\ntonehf=" ) +                        // Add tone setting HF
         String ( ini_block.rtone[1] ) +
         String ( "\ntonela=" ) +                        // Add tone setting LA
         String ( ini_block.rtone[2] ) +
         String ( "\ntonelf=" ) +                        // Add tone setting LF
         String ( ini_block.rtone[3] ) ;
}


//**************************************************************************************************
//                                           T F T L O G                                           *
//**************************************************************************************************
// Log to display.                                                                                 *
//**************************************************************************************************
void tftlog ( const char *str, bool newline )
{
  if ( dsp_ok )                                        // TFT configured?
  {
    dsp_print ( str ) ;                                // Yes, show error on TFT
    if ( newline )
    {
      dsp_print ( "\n" ) ;
    }
    dsp_update ( true ) ;                              // To physical screen
  }
}


//**************************************************************************************************
//                            B U B B L E S O R T K E Y S                                          *
//**************************************************************************************************
// Bubblesort the nvskeys.                                                                         *
//**************************************************************************************************
void bubbleSortKeys ( uint16_t n )
{
  uint16_t i, j ;                                             // Indexes in nvskeys
  char     tmpstr[NVS_KEY_NAME_MAX_SIZE] ;                    // Temp. storage for a key

  for ( i = 0 ; i < n - 1 ; i++ )                             // Examine all keys
  {
    for ( j = 0 ; j < n - i - 1 ; j++ )                       // Compare to next keys
    {
      if ( strcmp ( nvskeys[j], nvskeys[j + 1] ) > 0 )        // Next key out of order?
      {
        strcpy ( tmpstr, nvskeys[j] ) ;                       // Save current key a while
        strcpy ( nvskeys[j], nvskeys[j + 1] ) ;               // Replace current with next key
        strcpy ( nvskeys[j + 1], tmpstr ) ;                   // Replace next with saved current
      }
    }
  }
}


//**************************************************************************************************
//                                      F I L L K E Y L I S T                                      *
//**************************************************************************************************
// File the list of all relevant keys in NVS.                                                      *
// The keys will be sorted.                                                                        *
//**************************************************************************************************
void fillkeylist()
{
  nvs_iterator_t   it ;                                         // Iterator for NVS
  nvs_entry_info_t info ;                                       // Info in entry
  uint16_t         nvsinx = 0 ;                                 // Index in nvskey table

  it = nvs_entry_find ( "nvs", NAME, NVS_TYPE_ANY ) ;           // Get first entry
  while ( it )
  {
    nvs_entry_info ( it, &info ) ;                              // Get info on this entry
    ESP_LOGI ( TAG, "%s::%s type=%d",
               info.namespace_name, info.key, info.type ) ;
    if ( info.type == NVS_TYPE_STR )                            // Only string are used
    {
      strcpy ( nvskeys[nvsinx], info.key ) ;                    // Save key in table
      if ( ++nvsinx == MAXKEYS )
      {
        nvsinx-- ;                                              // Prevent excessive index
      }
    }
    it = nvs_entry_next ( it ) ;
  }
  nvs_release_iterator ( it ) ;                                 // Release resource
  nvskeys[nvsinx][0] = '\0' ;                                   // Empty key at the end
  ESP_LOGI ( TAG, "Read %d keys from NVS", nvsinx ) ;
  bubbleSortKeys ( nvsinx ) ;                                   // Sort the keys
}


//**************************************************************************************************
//                                       H A N D L E D A T A                                       *
//**************************************************************************************************
// Event callback on received data from MP3/AAC host.                                              *
// It is assumed that the first data buffer contains the full header.                              *
// Normally, the data length is 1436 bytes.                                                        *
//**************************************************************************************************
void handleData ( void* arg, AsyncClient* client, void *data, size_t len )
{
  uint8_t* p = (uint8_t*)data ;                         // Treat as an array of bytes

  // ESP_LOGI ( TAG, "Data received, %d bytes", len ) ;
  while ( len-- )
  {
    handlebyte_ch ( *p++ ) ;                            // Handle next byte
  }
}


//**************************************************************************************************
//                                  O N T I M E O U T                                              *
//**************************************************************************************************
// Event callback on p3 host connect time-out.                                                     *
//**************************************************************************************************
void onTimeout ( void* arg, AsyncClient* client, uint32_t t )
{
  ESP_LOGE ( TAG, "MP3 client connect time-out!" ) ;
}


//**************************************************************************************************
//                                           O N E R R O R                                         *
//**************************************************************************************************
// Event callback on MP3 host connect error.                                                       *
//**************************************************************************************************
void onError ( void* arg, AsyncClient* client, err_t a )
{
  ESP_LOGI ( TAG, "MP3 host error %s", client->errorToString ( a ) ) ;
}


//**************************************************************************************************
//                                           O N C O N N E C T                                     *
//**************************************************************************************************
// Event callback on MP3 host connect.                                                             *
//**************************************************************************************************
void onConnect ( void* arg, AsyncClient* client )
{
  ESP_LOGI ( TAG, "Connected to host at %s on port %d",
             client->remoteIP().toString().c_str(),
             client->remotePort() ) ;
}


//**************************************************************************************************
//                                     O N D I S C O N N E C T                                     *
//**************************************************************************************************
// Event callback on MP3 host disconnect.                                                          *
//**************************************************************************************************
void onDisConnect ( void* arg, AsyncClient* client )
{
  ESP_LOGI ( TAG, "Host disconnected" ) ;
}


//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
// Setup for the program.                                                                          *
//**************************************************************************************************
void setup()
{
  int                        i ;                          // Loop control
  int                        pinnr ;                      // Input pinnumber
  const char*                p ;
  byte                       mac[6] ;                     // WiFi mac address
  char                       tmpstr[20] ;                 // For version and Mac address
  esp_partition_iterator_t   pi ;                         // Iterator for find
  const esp_partition_t*     ps ;                         // Pointer to partition struct

  maintask = xTaskGetCurrentTaskHandle() ;                // My taskhandle
  outchunk.datatyp = QDATA ;                              // This chunk dedicated to QDATA
  vTaskDelay ( 3000 / portTICK_PERIOD_MS ) ;              // Wait for PlatformIO monitor to start
  Serial.begin ( 115200 ) ;                               // For debug
  WRITE_PERI_REG ( RTC_CNTL_BROWN_OUT_REG, 0 ) ;          // Disable brownout detector
  log_printf ( "\n" ) ;
  // Print some memory and sketch info
  log_printf ( "Starting ESP32-radio running on CPU %d at %d MHz.\n",
             xPortGetCoreID(),
             ESP.getCpuFreqMHz() ) ;
  ESP_LOGI ( TAG, "Version %s.  Free memory %d",
             VERSION,
             heapspace ) ;                                // Normally about 100 kB
  ESP_LOGI ( TAG, "Display type is %s", DISPLAYTYPE ) ;   // Report display option
  
  if ( !SPIFFS.begin ( FSIF ) )                           // Mount and test SPIFFS
  {
    ESP_LOGE ( TAG, "SPIFFS Mount Error!" ) ;             // A pity...
  }
  else
  {
    ESP_LOGI ( TAG, "SPIFFS is okay, space %d, used %d",  // Show available SPIFFS space
               SPIFFS.totalBytes(),
               SPIFFS.usedBytes() ) ;
    File index_html = SPIFFS.open ( "/index.html",        // Try to read from SPIFFS file
                                    FILE_READ ) ;
    if ( index_html )                                     // Open success?
    {
      index_html.close() ;                                // Yes, close file
    }
    else
    {
      ESP_LOGE ( TAG, "Web interface incomplete!" ) ;          // No, show warning, upload data to SPIFFS
    }
  }
  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,     // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,   // All data partitions
                            NULL ) ;
  while ( pi )
  {
    ps = esp_partition_get ( pi ) ;                       // Get partition struct
    ESP_LOGI ( TAG, "Found partition '%-8s' "             // Show partition
               "at offset 0x%06X "
               "with size %8d",
               ps->label, ps->address, ps->size ) ;
    if ( strcmp ( ps->label, "nvs" ) == 0 )              // Is this the NVS partition?
    {
      nvs = ps ;                                         // Yes, remember NVS partition
    }
    pi = esp_partition_next ( pi ) ;                     // Find next
  }
  #ifdef FIXEDWIFI                                       // Set fixedwifi if defined
    fixedwifi = FIXEDWIFI ;
  #endif
  if ( nvs == NULL )
  {
    ESP_LOGE ( TAG, "Partition NVS not found!" ) ;       // Very unlikely...
    while ( true ) ;                                     // Impossible to continue
  }
  fillkeylist() ;                                        // Fill keynames with all keys
  memset ( &ini_block, 0, sizeof(ini_block) ) ;          // Init ini_block
  ini_block.mqttport = 1883 ;                            // Default port for MQTT
  ini_block.mqttprefix = "" ;                            // No prefix for MQTT topics seen yet
  ini_block.clk_server = "pool.ntp.org" ;                // Default server for NTP
  ini_block.clk_offset = 1 ;                             // Default Amsterdam time zone
  ini_block.clk_dst = 1 ;                                // DST is +1 hour
  ini_block.bat0 = 2600 ;                                // Battery ADC level for 0 percent
  ini_block.bat100 = 2950 ;                              // Battery ADC level for 100 percent
  readIOprefs() ;                                        // Read pins used for SPI, TFT, VS1053, IR,
                                                         // Rotary encoder
  for ( i = 0 ; (pinnr = progpin[i].gpio) >= 0 ; i++ )   // Check programmable input pins
  {
    pinMode ( pinnr, INPUT_PULLUP ) ;                    // Input for control button
    vTaskDelay ( 10 / portTICK_PERIOD_MS ) ;
    // Check if pull-up active
    if ( ( progpin[i].cur = digitalRead ( pinnr ) ) == HIGH )
    {
      p = "HIGH" ;
    }
    else
    {
      p = "LOW, probably no PULL-UP" ;                   // No Pull-up
    }
    ESP_LOGI ( TAG, "GPIO%d is %s", pinnr, p ) ;
  }
  readprogbuttons() ;                                    // Program the free input pins
  if ( ini_block.spi_sck_pin >= 0 )
  {
    SPI.begin ( ini_block.spi_sck_pin,                   // Init VSPI bus with default or modified pins
                ini_block.spi_miso_pin,
                ini_block.spi_mosi_pin ) ;
  }
  if ( ini_block.ir_pin >= 0 )
  {
    ESP_LOGI ( TAG, "Enable pin %d for IR",
               ini_block.ir_pin ) ;
    pinMode ( ini_block.ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ini_block.ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_IR, CHANGE ) ;
  }
  ESP_LOGI ( TAG, "Start %s display", DISPLAYTYPE ) ;
  dsp_ok = dsp_begin ( INIPARS ) ;                       // Init display
  if ( dsp_ok )                                          // Init okay?
  {
    dsp_erase() ;                                        // Clear screen
    dsp_setRotation() ;                                  // Usse landscape format
    dsp_setTextSize ( DEFTXTSIZ ) ;                      // Small character font
    dsp_setTextColor ( GREY ) ;                          // Info in grey
    dsp_setCursor ( 0, 0 ) ;                             // Top of screen
    dsp_println ( "Starting......" ) ;
    strncpy ( tmpstr, VERSION, 16 ) ;                    // Limit version length
    dsp_println ( tmpstr ) ;
    dsp_println ( "By Ed Smallenburg" ) ;
    dsp_update ( enc_menu_mode == VOLUME ) ;             // Show on physical screen if needed
  }
  else
  {
    ESP_LOGE ( TAG, "Display not activated" ) ;          // Display init error
  }
  if ( ini_block.tft_bl_pin >= 0 )                       // Backlight for TFT control?
  {
    pinMode ( ini_block.tft_bl_pin, OUTPUT ) ;           // Yes, enable output
  }
  if ( ini_block.tft_blx_pin >= 0 )                      // Backlight for TFT (inversed logic) control?
  {
    pinMode ( ini_block.tft_blx_pin, OUTPUT ) ;          // Yes, enable output
  }
  blset ( true ) ;                                       // Enable backlight (if configured)
  #ifndef ETHERNET
    mk_lsan() ;                                          // Make a list of acceptable networks
                                                         // in preferences.
    WiFi.disconnect() ;                                  // After restart router could still
    vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;            //   keep old connection
    WiFi.mode ( WIFI_STA ) ;                             // This ESP is a station
    // WiFi.setSleep (false ) ;                          // should prevent _poll(): pcb is NULL""error"
    vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;            // ??
    WiFi.persistent ( false ) ;                          // Do not save SSID and password
  #endif
  readprefs ( false ) ;                                  // Read preferences
  radioqueue = xQueueCreate ( 10,                        // Create small queue for communication to radiofuncs
                             sizeof ( qdata_type ) ) ;
  dataqueue = xQueueCreate  ( QSIZ,                      // Create queue for data communication
                             sizeof ( qdata_struct ) ) ;
  p = "Connect to network" ;                             // Show progress
  ESP_LOGI ( TAG, "%s", p ) ;
  tftlog ( p, true ) ;                                   // On TFT too
  #ifdef ETHERNET
    WiFi.onEvent ( EthEvent ) ;                          // Set actions on ETH events
    NetworkFound = connectETH() ;                        // Connect to Ethernet
  #else
    NetworkFound = connectwifi() ;                       // Connect to WiFi network
  #endif
  tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA,
                               NAME ) ;
  ESP_LOGI ( TAG, "Start web server" ) ;
  cmdserver.on ( "/getprefs",  handle_getprefs ) ;       // Handle get preferences
  cmdserver.on ( "/saveprefs", handle_saveprefs ) ;      // Handle save preferences
  cmdserver.on ( "/getdefs",   handle_getdefs ) ;        // Handle get default config
  cmdserver.on ( "/settings",  handle_settings ) ;       // Handle strings like presets/volume,...
  cmdserver.on ( "/mp3list",   handle_mp3list ) ;        // Handle request for list of tracks
  cmdserver.on ( "/reset",     handle_reset ) ;          // Handle reset command
  cmdserver.onNotFound ( handle_notfound ) ;             // For handling a simple page/file and parameters
  cmdserver.begin() ;                                    // Start http server
  if ( NetworkFound )                                    // OTA and MQTT only if Wifi network found
  {
    ESP_LOGI ( TAG, "Network found. Starting clients" ) ;
    mp3client = new AsyncClient ;                        // Create client for Shoutcast connection
    mp3client->onData ( &handleData ) ;                  // Set callback on received mp3 data
    mp3client->onConnect ( &onConnect ) ;                // Set callback on connect
    mp3client->onDisconnect ( &onDisConnect ) ;          // Set callback on disconnect
    mp3client->onError ( &onError ) ;                    // Set callback on error
    mp3client->onTimeout ( &onTimeout ) ;                // Set callback on time-out
    mqtt_on = ( ini_block.mqttbroker.length() > 0 ) &&   // Use MQTT if broker specified
              ( ini_block.mqttbroker != "none" ) ;
    #ifdef ENABLEOTA
      ArduinoOTA.setHostname ( NAME ) ;                  // Set the hostname
      ArduinoOTA.onStart ( otastart ) ;
      ArduinoOTA.onError ( otaerror ) ;
      ArduinoOTA.begin() ;                               // Allow update over the air
    #endif
    if ( mqtt_on )                                       // Broker specified?
    {
      if ( ( ini_block.mqttprefix.length() == 0 ) ||     // No prefix?
           ( ini_block.mqttprefix == "none" ) )
      {
        WiFi.macAddress ( mac ) ;                        // Get mac-adress
        sprintf ( tmpstr, "P%02X%02X%02X%02X",           // Generate string from last part
                  mac[3], mac[2],
                  mac[1], mac[0] ) ;
        ini_block.mqttprefix = String ( tmpstr ) ;       // Save for further use
      }
      ESP_LOGI ( TAG, "MQTT uses prefix %s", ini_block.mqttprefix.c_str() ) ;
      ESP_LOGI ( TAG, "Init MQTT" ) ;
      mqttclient.setServer(ini_block.mqttbroker.c_str(), // Specify the broker
                           ini_block.mqttport ) ;        // And the port
      mqttclient.setCallback ( onMqttMessage ) ;         // Set callback on receive
    }
    if ( MDNS.begin ( NAME ) )                           // Start MDNS transponder
    {
      ESP_LOGI ( TAG, "MDNS responder started" ) ;
    }
    else
    {
      ESP_LOGE ( TAG, "Error setting up MDNS responder!" ) ;
    }
  }
  timer = timerBegin ( 0, 80, true ) ;                   // User 1st timer with prescaler 80
  timerAttachInterrupt ( timer, &timer100, false ) ;     // Call timer100() on timer alarm
  timerAlarmWrite ( timer, 100000, true ) ;              // Alarm every 100 msec
  timerAlarmEnable ( timer ) ;                           // Enable the timer
  vTaskDelay ( 1000 / portTICK_PERIOD_MS ) ;             // Show IP for a while
  configTime ( ini_block.clk_offset * 3600,
               ini_block.clk_dst * 3600,
               ini_block.clk_server.c_str() ) ;          // GMT offset, daylight offset in seconds
  timeinfo.tm_year = 0 ;                                 // Set TOD to illegal
  // Init settings for rotary switch (if existing).
  #ifdef ZIPPYB5
    if ( ( ini_block.enc_up_pin + ini_block.enc_dwn_pin + ini_block.enc_sw_pin ) > 2 )
    {
      attachInterrupt ( ini_block.enc_up_pin,  isr_enc_turn,   CHANGE ) ;
      attachInterrupt ( ini_block.enc_dwn_pin, isr_enc_turn,   CHANGE ) ;
      attachInterrupt ( ini_block.enc_sw_pin,  isr_enc_switch, CHANGE ) ;
      ESP_LOGI ( TAG, "ZIPPY side switch is enabled" ) ;
    }
    else
    {
      ESP_LOGI ( TAG, "ZIPPY side switch is disabled (%d/%d/%d)",
                ini_block.enc_up_pin,
                ini_block.enc_dwn_pin,
                ini_block.enc_sw_pin ) ;
    }
  #else
    if ( ( ini_block.enc_clk_pin + ini_block.enc_dt_pin + ini_block.enc_sw_pin ) > 2 )
    {
      attachInterrupt ( ini_block.enc_clk_pin, isr_enc_turn,   CHANGE ) ;
      attachInterrupt ( ini_block.enc_dt_pin,  isr_enc_turn,   CHANGE ) ;
      attachInterrupt ( ini_block.enc_sw_pin,  isr_enc_switch, CHANGE ) ;
      ESP_LOGI ( TAG, "Rotary encoder is enabled" ) ;
    }
    else
    {
      ESP_LOGI ( TAG, "Rotary encoder is disabled (%d/%d/%d)",
                ini_block.enc_clk_pin,
                ini_block.enc_dt_pin,
                ini_block.enc_sw_pin ) ;
    }
  #endif
  if ( NetworkFound )
  {
    gettime() ;                                           // Sync time
  }
  adc1_config_width ( ADC_WIDTH_12Bit ) ;
  adc1_config_channel_atten ( ADC1_CHANNEL_0, ADC_ATTEN_DB_11 ) ;  // VP/GPIO36
  xTaskCreatePinnedToCore (
    playtask,                                             // Task to play data in dataqueue.
    "Playtask",                                           // Name of task.
    2100,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    2,                                                    // priority of the task
    &xplaytask,                                           // Task handle to keep track of created task
    0 ) ;                                                 // Run on CPU 0
  vTaskDelay ( 100 / portTICK_PERIOD_MS ) ;               // Allow playtask to start
#ifdef SDCARD
  sdqueue = xQueueCreate ( 10,                            // Create small queue for communication to sdfuncs
                           sizeof ( qdata_type ) ) ;
  xTaskCreatePinnedToCore (
    SDtask,                                               // Task to get filenames from SD card
    "SDtask",                                             // Name of task.
    4000,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    2,                                                    // priority of the task
    &xsdtask,                                             // Task handle to keep track of created task
    0 ) ;                                                 // Run on CPU 0
#endif
  singleclick = false ;                                   // Might be fantom click
  if ( dsp_ok )                                           // Is display okay?
  {
    vTaskDelay ( 2000 / portTICK_PERIOD_MS ) ;            // Yes, allow user to read display text
    dsp_erase() ;                                         // Clear screen
  }
  tftset ( 0, NAME ) ;                                    // Set screen segment text top line
  presetinfo.station_state = ST_PRESET ;                  // Start in preset mode
  if ( nextPreset ( nvsgetstr ( "preset" ).toInt(),       // Restore last preset
       false  ) )
  {
    if ( NetworkFound )                                     // Start with preset if network available
    {
      myQueueSend ( radioqueue, &startcmd ) ;               // Start player in radio mode
    }
  }
}


//**************************************************************************************************
//                                        W R I T E P R E F S                                      *
//**************************************************************************************************
// Update the preferences.  Called from the web interface.                                         *
// Parameter is a string with multiple HTTP key/value pairs.                                       *
//**************************************************************************************************
void writeprefs ( AsyncWebServerRequest *request )
{
  int        numargs ;                            // Number of arguments
  int        i ;                                  // Index in arguments
  String     key ;                                // Name of parameter i
  String     contents ;                           // Value of parameter i

  //timerAlarmDisable ( timer ) ;                 // Disable the timer
  nvsclear() ;                                    // Remove all preferences
  numargs = request->params() ;                   // Haal aantal parameters
  ESP_LOGI ( TAG, "writeprefs numargs is %d",
             numargs ) ;
  for ( i = 0 ; i < numargs ; i++ )               // Scan de parameters
  {
    key = request->argName ( i ) ;                // Get name (key)
    contents = request->arg ( i ) ;               // Get value
    chomp ( key ) ;                               // Remove leading/trailing spaces
    chomp ( contents ) ;                          // Remove leading/trailing spaces
    contents.replace ( "($)", "#" ) ;             // Replace comment separator
    if ( key.isEmpty() || contents.isEmpty() )    // Skip empty keys (comment line)
    {
      continue ;
    }
    if ( key == "version" )                       // Skip de "version" parameter
    {
      continue ;
    }
    ESP_LOGI ( TAG, "Handle POST %s = %s",
               key.c_str(), contents.c_str() ) ;  // Toon POST parameter
    nvssetstr ( key.c_str(), contents ) ;         // Save new pair
  }
  nvs_commit( nvshandle ) ;
  //timerAlarmEnable ( timer ) ;                  // Enable the timer
  fillkeylist() ;                                 // Update list with keys
}


#ifdef SDCARD
//**************************************************************************************************
//                                        C B  _ M P 3 L I S T                                     *
//**************************************************************************************************
// Callback function for handle_mp3list, will be called for every chunk to send to client.         *
// If no more data is available, this function will return 0.                                      *
//**************************************************************************************************
size_t cb_mp3list ( uint8_t *buffer, size_t maxLen, size_t index )
{
  static int         i ;                              // Index in track list
  static const char* path ;                           // Pointer in file path
  size_t             len = 0 ;                        // Number of bytes filled in buffer
  char*              p = (char*)buffer ;              // Treat as pointer to aray of char
  static bool        eolSeen ;                        // Remember if End Of List

  if ( index == 0 )                                   // First call for this page?
  {
    i = 0 ;                                           // Yes, set index (track number)
    path = getFirstSDFileName() ;                     // Force read of next path
    eolSeen = ( path == nullptr ) ;                   // Any file?
  }
  while ( ( maxLen > len ) && ( ! eolSeen ) )         // Space for another char from path?
  {
    if ( *path )                                      // End of path?
    {
      *p++ = *path++ ;                                // No, add another character to send buffer
      len++ ;                                         // Update total length
    }
    else
    {
      // End of path
      if ( i )                                        // At least one path in output?
      {
        *p++ = '\n' ;                                 // Yes, add separator
        len++ ;                                       // Update total length
      }
      path = getSDFileName ( i++ ) ;                  // Get next path from list
      if ( i >= SD_filecount )                        // No more files?
      {
        eolSeen = true ;                              // Yes, stop
        break ;
      }
    }
  }
  // We arrive here if output buffer is completely full or end of tracklist is reached
  return len ;                                        // Return filled length of buffer
}


//**************************************************************************************************
//                                    H A N D L E _ M P L I S T                                    *
//**************************************************************************************************
// Called from mp3play page to list all the MP3 tracks.                                            *
// It will handle the chunks for the client.  The buffer is filled by the callback routine.        *
//**************************************************************************************************
void handle_mp3list ( AsyncWebServerRequest *request )
{
  AsyncWebServerResponse *response ;

  response = request->beginChunkedResponse ( "text/plain", cb_mp3list ) ;
  response->addHeader ( "Server", NAME ) ;
  request->send ( response ) ;
}
#else
//**************************************************************************************************
//                                    H A N D L E _ M P L I S T                                    *
//**************************************************************************************************
// Dummy version.                                                                                  *
//**************************************************************************************************
void handle_mp3list ( AsyncWebServerRequest *request )
{
  request->send ( 200, "text/plain", "<empty>" ) ;
}
#endif


//**************************************************************************************************
//                                    H A N D L E _ G E T P R E F S                                *
//**************************************************************************************************
// Called from config page to display configuration data.                                          *
//**************************************************************************************************
void handle_getprefs ( AsyncWebServerRequest *request )
{
  String prefs ;
  
  //if ( datamode != STOPPED )                         // Still playing?
  //{
  //  setdatamode (  STOPREQD ) ;                      // Stop playing
  //}
  prefs = readprefs ( true ) ;                         // Read preference values
  request->send ( 200, "text/plain", prefs ) ;         // Send the reply
}


//**************************************************************************************************
//                                    H A N D L E _ S A V E P R E F S                              *
//**************************************************************************************************
// Called from config page to save configuration data.                                             *
//**************************************************************************************************
void handle_saveprefs ( AsyncWebServerRequest *request )
{
  const char* reply = "Config saved" ;                 // Default reply

  writeprefs ( request ) ;                             // Write to NVS
  request->send ( 200, "text/plain", reply ) ;         // Send the reply
}


//**************************************************************************************************
//                                    H A N D L E _ G E T D E F S                                  *
//**************************************************************************************************
// Called from config page to load default configuration.                                          *
//**************************************************************************************************
void handle_getdefs ( AsyncWebServerRequest *request )
{
  String ct ;                                         // Content type
  const char* path ;                                  // File with default settings

  path = "/defaultprefs.txt" ;                        // Set file name
  if ( SPIFFS.exists ( path ) )                       // Does it exist in SPIFFS?
  {
    ct = getContentType ( path ) ;                    // Yes, get content type
    request->send ( SPIFFS, path, ct ) ;              // Send to client
  }
  else
  {
    request->send ( 200, "text/plain",                // No send empty preferences
                    "<empty>" ) ;
  }
}


//**************************************************************************************************
//                                    H A N D L E _ S E T T I N G S                                *
//**************************************************************************************************
// Called from index page to load settings like presets, volume. tone....                          *
//**************************************************************************************************
void handle_settings ( AsyncWebServerRequest *request )
{
  String              val = String() ;                   // Result to send
  String              statstr ;                          // Station string
  String              hsym ;                             // Symbolic station name from comment part
  int16_t             i ;                                // Loop control, preset number

  for ( i = 0 ; i < MAXPRESETS ; i++ )                   // Max number of presets
  {
    readhostfrompref ( i, &statstr, &hsym ) ;            // Get the preset from NVS
    if ( statstr != "" )                                 // Preset available?
    {
      // Show just comment if available.  Otherwise the preset itself.
      if ( hsym != "" )                                  // hsym set?
      {
        statstr = hsym ;                                 // Yes, use it
      }
      chomp ( statstr ) ;                                // Remove garbage from description
      //ESP_LOGI ( TAG, "statstr is %s", statstr.c_str() ) ;
      val += String ( "preset_" ) +
             String ( i ) +
             String ( "=" ) +
             statstr +
             String ( "\n" ) ;                           // Add delimeter
    }
  }
  #ifdef DEC_HELIX
    val += String ( "decoder=helix\n" ) ;                // Add decoder type for helix (no volume buttons)
  #endif
  val += getradiostatus() +                              // Add radio setting
         String ( "\n\n" ) ;                             // End of reply
  request->send ( 200, "text/plain", val ) ;             // Send preferences
}


//**************************************************************************************************
//                                  H A N D L E S A V E R E Q                                      *
//**************************************************************************************************
// Handle save volume/preset/tone.  This will save current settings every 10 minutes to            *
// the preferences.  On the next restart these values will be loaded.                              *
// Note that saving prefences will only take place if contents has changed.                        *
//**************************************************************************************************
void handleSaveReq()
{
  static uint32_t savetime = 0 ;                          // Limit save to once per 10 minutes

  if ( ( millis() - savetime ) < 600000 )                 // 600 sec is 10 minutes
  {
    return ;
  }
  savetime = millis() ;                                   // Set time of last save
  nvssetstr ( "preset", String ( presetinfo.preset  ) ) ; // Save current preset
  nvssetstr ( "volume", String ( ini_block.reqvol   ) ) ; // Save current volue
  nvssetstr ( "toneha", String ( ini_block.rtone[0] ) ) ; // Save current toneha
  nvssetstr ( "tonehf", String ( ini_block.rtone[1] ) ) ; // Save current tonehf
  nvssetstr ( "tonela", String ( ini_block.rtone[2] ) ) ; // Save current tonela
  nvssetstr ( "tonelf", String ( ini_block.rtone[3] ) ) ; // Save current tonelf
}


//**************************************************************************************************
//                                    H A N D L E _ R E S E T                                      *
//**************************************************************************************************
// Called from config page to reset the radio.                                                     *
//**************************************************************************************************
void handle_reset ( AsyncWebServerRequest *request )
{
  request->send ( 200, "text/plain", "Command accepted"  ) ;         // Send the reply
  resetreq = true ;                                                  // Set the reset request
}


//**************************************************************************************************
//                                      H A N D L E I P P U B                                      *
//**************************************************************************************************
// Handle publish op IP to MQTT.  This will happen every 10 minutes.                               *
//**************************************************************************************************
void handleIpPub()
{
  static uint32_t pubtime = 300000 ;                       // Limit save to once per 10 minutes

  if ( ( millis() - pubtime ) < 600000 )                   // 600 sec is 10 minutes
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  mqttpub.trigger ( MQTT_IP ) ;                            // Request re-publish IP
}


//**************************************************************************************************
//                                      H A N D L E V O L P U B                                    *
//**************************************************************************************************
// Handle publish of Volume to MQTT.  This will happen max every 10 seconds.                       *
//**************************************************************************************************
void handleVolPub()
{
  static uint32_t pubtime = 10000 ;                        // Limit save to once per 10 seconds
  static uint8_t  oldvol = -1 ;                            // For comparison

  if ( ( millis() - pubtime ) < 10000 )                    // 10 seconds
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  if ( ini_block.reqvol != oldvol )                        // Volume change?
  {
    mqttpub.trigger ( MQTT_VOLUME ) ;                      // Request publish VOLUME
    oldvol = ini_block.reqvol ;                            // Remember publishe volume
  }
}


//**************************************************************************************************
//                                           C H K _ E N C                                         *
//**************************************************************************************************
// See if rotary encoder is activated and perform its functions.                                   *
//**************************************************************************************************
void chk_enc()
{
  static int16_t enc_preset ;                                 // Selected preset
  String         tmp, tmp2 ;                                  // Temporary strings

  if ( enc_menu_mode != VOLUME )                              // In default mode?
  {
    if ( enc_inactivity > 50 )                                // No, more than 5 seconds inactive
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;                                // Return to VOLUME mode
      ESP_LOGI ( TAG, "Encoder mode back to VOLUME" ) ;
    }
  }
  if ( singleclick || doubleclick ||                          // Any activity?
       tripleclick || longclick ||
       ( rotationcount != 0 ) )
  {
    blset ( true ) ;                                          // Yes, activate display if needed
  }
  else
  {
    return ;                                                  // No, nothing to do
  }
  if ( tripleclick )                                          // First handle triple click
  {
    ESP_LOGI ( TAG, "Triple click" ) ;
    tripleclick = false ;                                     // Reset flag
    #ifdef SDCARD
      if ( SD_filecount )                                     // Tracks on SD?
      {
        enc_menu_mode = TRACK ;                               // Swich to TRACK mode
        ESP_LOGI ( TAG, "Encoder mode set to TRACK" ) ;
        tftset ( 3, "Turn to select track\n"                  // Show current option
                    "Press to confirm" ) ;
        getSDFileName ( +1 ) ;                                // Start with next file on SD
      }
      else
      {
        ESP_LOGI ( TAG, "No tracks on SD" ) ;
      }
    #endif
  }
  if ( doubleclick )                                          // Handle the doubleclick
  {
    ESP_LOGI ( TAG, "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;                                  // Swich to PRESET mode
    ESP_LOGI ( TAG, "Encoder mode set to PRESET" ) ;
    tftset ( 3, "Turn to select station\n"                    // Show current option
                "Press to confirm" ) ;
    enc_preset = presetinfo.preset ;                          // Start with current preset
    updateNr ( &enc_preset, presetinfo.highest_preset,        // plus 1
               1, true ) ;
  }
  if ( singleclick )
  {
    ESP_LOGI ( TAG, "Single click" ) ;
    singleclick = false ;
    switch ( enc_menu_mode )                                  // Which mode (VOLUME, PRESET)?
    {
      case VOLUME :
        if ( muteflag )
        {
          tftset ( 2, icyname ) ;                             // Restore screen segment bottom part
        }
        else
        {
          tftset ( 3, "Mute" ) ;
        }
        muteflag = !muteflag ;                                // Mute/unmute
        ESP_LOGI ( TAG, "Mute set to %d", muteflag ) ;
        break ;
      case PRESET :
        nextPreset ( enc_preset ) ;                           // Make a definite choice
        enc_menu_mode = VOLUME ;                              // Back to default mode
        myQueueSend ( radioqueue, &startcmd ) ;               // Signal radiofuncs()
        tftset ( 2, icyname ) ;                               // Restore screen segment bottom part
        break ;
    #ifdef SDCARD
      case TRACK :
        myQueueSend ( sdqueue, &startcmd ) ;                  // Signal SDfuncs()
        enc_menu_mode = VOLUME ;                              // Back to default mode
        tftset ( 2, icyname ) ;                               // Restore screen segment bottom part
        break ;
    #endif
      default :
        break ;
    }
  }
  if ( longclick )                                            // Check for long click
  {
    ESP_LOGI ( TAG, "Long click") ;
    myQueueSend ( sdqueue, &stopcmd ) ;                       // Stop player
    myQueueSend ( radioqueue, &stopcmd ) ;                    // Stop player
    //if ( datamode != STOPPED )
    //{
    //  setdatamode ( STOPREQD ) ;                            // Request STOP, do not touch longclick flag
    //}
    //else
    //{
    longclick = false ;                                       // Reset condition
    #ifdef SDCARD
      if ( SD_filecount )
      {
        getSDFileName ( -1 ) ;                                // Random choice
        myQueueSend ( sdqueue, &startcmd ) ;                  // Start random track
      }
    #endif
  }
  if ( rotationcount == 0 )                                   // Any rotation?
  {
    return ;                                                  // No, return
  }
  //ESP_LOGI ( TAG, "Rotation count %d", rotationcount ) ;
  switch ( enc_menu_mode )                                    // Which mode (VOLUME, PRESET, TRACK)?
  {
    case VOLUME :
      if ( ! muteflag )                                       // Do not handle if muted
      {
        rotationcount *= 4 ;                                  // Step by 4 percent
        if ( ( ini_block.reqvol + rotationcount ) < 0 )       // Limit volume
        {
          ini_block.reqvol = 0 ;                              // Limit to normal values
        }
        else if ( ( ini_block.reqvol + rotationcount ) > 100 )
        {
          ini_block.reqvol = 100 ;                            // Limit to normal values
        }
        else
        {
          ini_block.reqvol += rotationcount ;
        }
      }
      break ;
    case PRESET :
      if ( ( enc_preset + rotationcount ) < 0 )               // Negative not allowed
      {
        enc_preset = 0 ;                                      // Stay at 0
      }
      else
      {
        enc_preset += rotationcount ;                         // Next preset
      }
      readhostfrompref ( enc_preset, &tmp, &tmp2 ) ;          // Get host spec and possible comment
      if ( tmp == "" )                                        // End of presets?
      {
        enc_preset = 0 ;                                      // Yes, wrap
        readhostfrompref ( enc_preset, &tmp, &tmp2 ) ;        // Get host spec and possible comment
      }
      ESP_LOGI ( TAG, "Preset is %d", enc_preset ) ;
      // Show just comment if available.  Otherwise the preset itself.
      if ( tmp2 != "" )                                       // Symbolic name present?
      {
        tmp = tmp2 ;                                          // Yes, use it
      }
      chomp ( tmp ) ;                                         // Remove garbage from description
      tftset ( 3, tmp ) ;                                     // Set screen segment bottom part
      break ;
#ifdef SDCARD
    case TRACK :
      if ( rotationcount > 0 )
      {
        getSDFileName ( SD_curindex + rotationcount ) ;      // Select next file on SD
        ESP_LOGI ( TAG, "Select track %s",                   // Show for debug
                    getCurrentSDFileName() ) ;
        tftset ( 3, getCurrentShortSDFileName() ) ;          // Set screen segment bottom part
      }
      break ;
#endif
    default :
      break ;
  }
  rotationcount = 0 ;                                         // Reset
}

//**************************************************************************************************
//                                     S P F U N C S                                               *
//**************************************************************************************************
// Handles display of text, time and volume on TFT.                                                *
// Handles ADC meassurements.                                                                      *
//**************************************************************************************************
void spfuncs()
{
  if ( spftrigger )                                             // Will be set every 100 msec
  {
    spftrigger = false ;                                        // Reset trigger
    if ( dsp_ok )                                               // Posible to update TFT?
    {
      for ( uint16_t i = 0 ; i < TFTSECS ; i++ )                // Yes, handle all sections
      {
        if ( tftdata[i].update_req )                            // Refresh requested?
        {
          displayinfo ( i ) ;                                   // Yes, do the refresh
          dsp_update ( enc_menu_mode == VOLUME ) ;              // Updates to the screen
          tftdata[i].update_req = false ;                       // Reset request
          break ;                                               // Just handle 1 request
        }
      }
      dsp_update ( enc_menu_mode == VOLUME ) ;                  // Be sure to paint physical screen
    }
    if ( muteflag )                                             // Mute or not?
    {
      player_setVolume ( 0 ) ;                                  // Mute
    }
    else
    {
      player_setVolume ( ini_block.reqvol ) ;                   // Unmute
    }
    if ( reqtone )                                              // Request to change tone?
    {
      reqtone = false ;
      player_setTone ( ini_block.rtone ) ;                      // Set SCI_BASS to requested value
    }
    if ( time_req )                                             // Time to refresh timetxt?
    {
      if ( NetworkFound )                                       // Yes, time available?
      {
        gettime() ;                                             // Yes, get the current time
      }
      time_req = false ;                                        // Yes, clear request
      displaytime ( timetxt ) ;                                 // Write to TFT screen
      displayvolume ( player_getVolume() ) ;                    // Show volume on display
      displaybattery ( ini_block.bat0, ini_block.bat100,        // Show battery charge on display
                       adcval ) ;
    }
    if ( mqtt_on )
    {
      if ( !mqttclient.connected() )                            // See if connected
      {
        mqttreconnect() ;                                       // No, reconnect
      }
      else
      {
        mqttpub.publishtopic() ;                                // Check if any publishing to do
      }
    }
    adcvalraw = adc1_get_raw ( ADC1_CHANNEL_0 ) ;
    adcval = ( 15 * adcval +                                    // Read ADC and do some filtering
               adcvalraw ) / 16 ;
  }
}


//**************************************************************************************************
//                                     R A D I O F U N C S                                         *
//**************************************************************************************************
// Handles commands for the connection to a icecast server.                                        *
// Commands are received in the input queue.                                                       *
// Data from the server is handle by the handleData() function.                                    *
//**************************************************************************************************
void radiofuncs()
{
  qdata_type   radiocmd ;                                         // Command from radioqueue
  static bool  connected = false ;                                // Connected to host or not

  if ( xQueueReceive ( radioqueue, &radiocmd, 0 ) )               // New command in queue?
  {
    ESP_LOGI ( TAG, "Radiofuncs cmd is %d", radiocmd ) ;
    switch ( radiocmd )                                           // Yes, examine command
    {
      case QSTARTSONG:                                            // Start a new station?
        if ( sdqueue )                                            // SD card configured?
        {
          myQueueSend ( sdqueue, &stopcmd ) ;                     // Yes, send STOP to SD queue (First Out)
          sdfuncs() ;                                             // Allow sdfuncs to react
        }
        connected = connecttohost() ;                             // Connect to stream host
        mqttpub.trigger ( MQTT_PRESET ) ;                         // Request publishing to MQTT
        break ;
      case QSTOPSONG:                                             // Stop playing?
        if ( connected )                                          // Yes, are we stiil playing?
        {
          stop_mp3client() ;                                      // Yes, stop input stream
          connected = false ;                                     // Remember connection state
        }
      default:
        break ;
    }
  }
}


//**************************************************************************************************
//                                           L O O P                                               *
//**************************************************************************************************
// Main loop of the program.                                                                       *
//**************************************************************************************************
void loop()
{
  if ( resetreq )                                   // Reset requested?
  {
    vTaskDelay ( 1000 / portTICK_PERIOD_MS ) ;      // Yes, wait some time
    timerDetachInterrupt ( timer ) ;
    timerEnd ( timer ) ;
    ESP.restart() ;                                 // Reboot
  }
  if ( sleepreq )                                   // Request for deep sleep?
  {
    if ( dsp_ok )                                   // TFT configured?
    {
      dsp_erase() ;                                 // Yes, clear screen
      dsp_update ( true ) ;                         // To physical screen
    }
    esp_deep_sleep_start() ;                        // Yes, sleep until reset
    // No return here...
  }
  scanserial() ;                                    // Handle serial input
  scanserial2() ;                                   // Handle serial input from NEXTION (if active)
  scandigital() ;                                   // Scan digital inputs
  scanIR() ;                                        // See if IR input
  #ifdef ENABLEOTA
    ArduinoOTA.handle() ;                           // Check for OTA
  #endif
  if ( mqtt_on )                                    // Need to handle MQTT?
  {
    mqttclient.loop() ;                             // Handling of MQTT connection
  }
  handleSaveReq() ;                                 // See if time to save settings
  handleIpPub() ;                                   // See if time to publish IP
  handleVolPub() ;                                  // See if time to publish volume
  chk_enc() ;                                       // Check rotary encoder functions
  radiofuncs() ;                                    // Handle start/stop commands for icecast
  spfuncs() ;                                       // Handle special functions
  sdfuncs() ;                                       // Do SD card related functions
  if ( testreq )
  {
    const char* sformat = "Stack %-8s is %4d\n" ;
    testreq = false ;
    // heap_caps_print_heap_info ( MALLOC_CAP_8BIT ) ;
    log_printf ( sformat, pcTaskGetTaskName ( maintask ),
                 uxTaskGetStackHighWaterMark ( maintask ) ) ;
    log_printf ( sformat, pcTaskGetTaskName ( xplaytask ),
                 uxTaskGetStackHighWaterMark ( xplaytask ) ) ;
    #ifdef SDCARD
      log_printf ( sformat, pcTaskGetTaskName ( xsdtask ),
                 uxTaskGetStackHighWaterMark ( xsdtask ) ) ;
    #endif
    log_printf ( "ADC reading is %d, filtered %d\n", adcvalraw, adcval ) ;
    log_printf ( "%d IR interrupts seen\n", ir_intcount ) ;
    if ( pin_exists ( ini_block.sd_detect_pin ) )
    {
      if ( digitalRead ( ini_block.sd_detect_pin ) == LOW )
      {
        log_printf ( "SD card detected\n" ) ;
      }
    }
  }
  delay ( 10 ) ;
}


//**************************************************************************************************
//                             D E C O D E _ S P E C _ C H A R S                                   *
//**************************************************************************************************
// Decode special characters like "&#39;".                                                         *
//**************************************************************************************************
String decode_spec_chars ( String str )
{
  int    inx, inx2 ;                                // Indexes in string
  char   c ;                                        // Character from string
  char   val ;                                      // Converted character
  String res = str ;

  while ( ( inx = res.indexOf ( "&#" ) ) >= 0 )     // Start sequence in string?
  {
    inx2 = res.indexOf ( ";", inx ) ;               // Yes, search for stop character
    if ( inx2 < 0 )                                 // Stop character found
    {
      break ;                                       // Malformed string
    }
    res = str.substring ( 0, inx ) ;                // First part
    inx += 2 ;                                      // skip over start sequence
    val = 0 ;                                       // Init result of 
    while ( ( c = str[inx++] ) != ';' )             // Convert character
    {
      val = val * 10 + c - '0' ;
    }
    res += ( String ( val ) +                       // Add special char to string
             str.substring ( ++inx2 ) ) ;           // Add rest of string
  }
  return res ;
}


//**************************************************************************************************
//                                    C H K H D R L I N E                                          *
//**************************************************************************************************
// Check if a line in the header is a reasonable headerline.                                       *
// Normally it should contain something like "icy-xxxx:abcdef".                                    *
//**************************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 70 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}


//**************************************************************************************************
//                            S C A N _ C O N T E N T _ L E N G T H                                *
//**************************************************************************************************
// If the line contains content-length information: set clength (content length counter).          *
//**************************************************************************************************
void scan_content_length ( const char* metalinebf )
{
  if ( strstr ( metalinebf, "Content-Length" ) )        // Line contains content length
  {
    clength = atoi ( metalinebf + 15 ) ;                // Yes, set clength
    ESP_LOGI ( TAG, "Content-Length is %d", clength ) ; // Show for debugging purposes
  }
}


//**************************************************************************************************
//                                   H A N D L E B Y T E _ C H                                     *
//**************************************************************************************************
// Handle the next byte of data from server.                                                       *
// Chunked transfer encoding aware. Chunk extensions are not supported.                            *
//**************************************************************************************************
void handlebyte_ch ( uint8_t b )
{
  static int       chunksize = 0 ;                      // Chunkcount read from stream
  static uint16_t  playlistcnt ;                        // Counter to find right entry in playlist
  static int       LFcount ;                            // Detection of end of header
  static bool      ctseen = false ;                     // First line of header seen or not
  static bool      redirection = false ;                // Redirection or not

  if ( chunked &&
       ( datamode & ( DATA |                            // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                              // Expecting a new chunkcount?
    {
      if ( b == '\r' )                                  // Skip CR
      {
        return ;
      }
      else if ( b == '\n' )                             // LF ?
      {
        chunkcount = chunksize ;                        // Yes, set new count
        chunksize = 0 ;                                 // For next decode
        return ;
      }
      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper ( b ) - '0' ;                         // Be sure we have uppercase
      if ( b > 9 )
      {
        b = b - 7 ;                                     // Translate A..F to 10..15
      }
      chunksize = ( chunksize << 4 ) + b ;
      return  ;
    }
    chunkcount-- ;                                      // Update count to next chunksize block
  }
  if ( datamode == DATA )                               // Handle next byte of MP3/AAC/Ogg data
  {
    *outqp++ = b ;
    if ( outqp == ( outchunk.buf + sizeof(outchunk.buf) ) )       // Buffer full?
    {
      // Send data to playtask queue.  If the buffer cannot be placed within 200 ticks,
      // the queue is full, while the sender tries to send more.  The chunk will be dis-
      // carded it that case.
      if ( xQueueSend ( dataqueue, &outchunk, 200 ) != pdTRUE )  // Send to queue
      {
        ESP_LOGE ( TAG, "MP3 packet dropped!" ) ;
      }
      outqp = outchunk.buf ;                            // Item empty now
    }
    if ( metaint )                                      // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                           // End of datablock?
      {
        setdatamode ( METADATA ) ;
        metalinebfx = -1 ;                              // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == INIT )                               // Initialize for header receive
  {
    ctseen = false ;                                    // Contents type not seen yet
    redirection = false ;                               // No redirection found yet
    outqp = outchunk.buf ;                              // Item empty now
    metaint = 0 ;                                       // No metaint found
    LFcount = 0 ;                                       // For detection end of header
    bitrate = 0 ;                                       // Bitrate still unknown
    ESP_LOGI ( TAG, "Switch to HEADER" ) ;
    setdatamode ( HEADER ) ;                            // Handle header
    totalcount = 0 ;                                    // Reset totalcount
    metalinebfx = 0 ;                                   // No metadata yet
    metalinebf[0] = '\0' ;
  }
  if ( datamode == HEADER )                             // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                                // Ignore unprintable characters
         ( b == '\r' ) ||                               // Ignore CR
         ( b == '\0' ) )                                // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                               // Linefeed ?
    {
      LFcount++ ;                                       // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                  // Take care of delimiter
      if ( chkhdrline ( metalinebf ) )                  // Reasonable input?
      {
        ESP_LOGI ( TAG, "Headerline: %s",               // Show headerline
                   metalinebf ) ;
        String metaline = String ( metalinebf ) ;       // Convert to string
        String lcml = metaline ;                        // Use lower case for compare
        lcml.toLowerCase() ;
        if ( lcml.startsWith ( "location: " ) )         // Redirection?
        {
          metaline = metaline.substring ( 10 ) ;        // Yes, get new URL
          int hp = metaline.indexOf ( "://" ) ;         // Redirection with "http(s)://" ?
          if ( hp > 0 )
          {
            metaline = metaline.substring ( hp + 3 ) ;  // Yes, get new URL
          }
          presetinfo.station_state = ST_REDIRECT ;      // Set host already filled
          presetinfo.host= metaline ;
          redirection = true ;                          // Remember redirection
        }
        if ( lcml.startsWith ( "content-type" ) )       // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                               // Yes, remember seeing this
          audio_ct = metaline.substring ( 13 ) ;        // Set contentstype
          audio_ct.trim() ;
          //ESP_LOGI ( TAG, "%s seen", audio_ct.c_str() ) ;  // Like "audio/mpeg"
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;     // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                           // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                              // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ("icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;    // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;             // Get station name
          icyname = decode_spec_chars ( icyname ) ;     // Decode special characters in name
          icyname.trim() ;                              // Remove leading and trailing spaces
          if ( icyname.isEmpty() )                      // Empty name?
          {
            icyname = presetinfo.hsym ;                 // Yes, use symbolic name
          }
          tftset ( 2, icyname ) ;                       // Set screen segment bottom part
          mqttpub.trigger ( MQTT_ICYNAME ) ;            // Request publishing to MQTT
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                            // Remember chunked transfer mode
            chunkcount = 0 ;                            // Expect chunkcount in DATA
          }
        }
      }
      metalinebfx = 0 ;                                 // Reset this line
      if ( LFcount == 2 )                               // Double LF marks end of header?
      {
        if ( redirection )                              // Redirection?
        {
          ESP_LOGI ( TAG, "Redirect" ) ;                // Yes, show
          setdatamode ( INIT ) ;                        // Mode to INIT again
          myQueueSend ( radioqueue, &startcmd ) ;       // Restart with new found host
        }
        else if ( ctseen )                              // Content type seen?
        {
          ESP_LOGI ( TAG, "Switch to DATA, bitrate is " // Show bitrate
                    "%d kbps, metaint is %d",           // and metaint
                    bitrate, metaint ) ;
          setdatamode ( DATA ) ;                        // Expecting data now
          datacount = metaint ;                         // Number of bytes before first metadata
          queueToPt ( QSTARTSONG ) ;                    // Queue a request to start song
        }
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;             // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                     // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                     // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                           // Handle next byte of metadata
  {
    if ( metalinebfx < 0 )                              // First byte of metadata?
    {
      metalinebfx = 0 ;                                 // Prepare to store first character
      metacount = b * 16 + 1 ;                          // New count for metadata including length byte
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;             // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                     // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( --metacount == 0 )
    {
      metalinebf[metalinebfx] = '\0' ;                  // Make sure line is limited
      if ( strlen ( metalinebf ) )                      // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        if ( showstreamtitle ( metalinebf ) )           // Show artist and title if present in metadata
        {
          mqttpub.trigger ( MQTT_STREAMTITLE ) ;        // Title changed: Request publishing to MQTT
        }
      }
      if ( metalinebfx  > ( METASIZ - 10 ) )            // Unlikely metaline length?
      {
        ESP_LOGE ( TAG, "Metadata block too long!" ) ;  // Probably no metadata
        // Skipping all Metadata from now on.
        metaint = 0 ;
      }
      datacount = metaint ;                             // Reset data count
      //bufcnt = 0 ;                                    // Reset buffer count
      setdatamode ( DATA ) ;                            // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                       // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metalinebfx = 0 ;                                   // Prepare for new line
    LFcount = 0 ;                                       // For detection end of header
    setdatamode ( PLAYLISTHEADER ) ;                    // Handle playlist header
    playlistcnt = 0 ;                                   // Reset for compare
    totalcount = 0 ;                                    // Reset totalcount
    clength = 0xFFFFFFFF ;                              // Content-length unknown
    ESP_LOGI ( TAG, "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                     // Read header
  {
    if ( ( b > 0x7F ) ||                                // Ignore unprintable characters
         ( b == '\r' ) ||                               // Ignore CR
         ( b == '\0' ) )                                // Ignore NULL
    {
      return ;                                          // Quick return
    }
    else if ( b == '\n' )                               // Linefeed ?
    {
      LFcount++ ;                                       // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                  // Take care of delimeter
      ESP_LOGI ( TAG, "Playlistheader: %s",             // Show playlistheader
                 metalinebf ) ;
      scan_content_length ( metalinebf ) ;              // Check if it is a content-length line
      metalinebfx = 0 ;                                 // Ready for next line
      if ( LFcount == 2 )
      {
        ESP_LOGI ( TAG, "Switch to PLAYLISTDATA, "      // For debug
                   "search for entry %d",
                   presetinfo.playlistnr ) ;
        setdatamode ( PLAYLISTDATA ) ;                  // Expecting data now
        mqttpub.trigger ( MQTT_PLAYLISTPOS ) ;          // Playlistposition to MQTT
        return ;
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;             // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                     // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                     // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                       // Read next byte of .m3u file data
  {
    clength-- ;                                         // Decrease content length by 1
    if ( ( b > 0x7F ) ||                                // Ignore unprintable characters
         ( b == '\r' ) ||                               // Ignore CR
         ( b == '\0' ) )                                // Ignore NULL
    {
      // Yes, ignore
    }
    if ( b != '\n' )                                    // Linefeed?
    { // No, normal character in playlistdata,
      metalinebf[metalinebfx++] = (char)b ;             // add it to metaline
      if ( metalinebfx >= METASIZ )                     // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( ( b == '\n' ) ||                               // linefeed?
         ( clength == 0 ) )                             // Or end of playlist data contents
    {
      int inx ;                                         // Pointer in metaline
      metalinebf[metalinebfx] = '\0' ;                  // Take care of delimeter
      ESP_LOGI ( TAG, "Playlistdata: %s",               // Show playlist data
                 metalinebf ) ;
      if ( strlen ( metalinebf ) < 5 )                  // Skip short lines
      {
        metalinebfx = 0 ;                               // Flush line
        metalinebf[0] = '\0' ;
        return ;
      }
      String metaline = String ( metalinebf ) ;         // Convert to string
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )       // Info?
      {
        if ( presetinfo.playlistnr == playlistcnt )     // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;              // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            if ( showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) )
            {
              mqttpub.trigger ( MQTT_STREAMTITLE ) ;    // Title change: request publishing to MQTT
            }
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )                // Commentline?
      {
        metalinebfx = 0 ;                               // Yes, ignore
        return ;                                        // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.
      presetinfo.highest_playlistnr = playlistcnt ;
      ESP_LOGI ( TAG, "Entry %d in playlist found: %s", playlistcnt, metalinebf ) ;
      if ( presetinfo.playlistnr == playlistcnt )       // Is it the right one?
      {
        inx = metaline.indexOf ( "://" ) ;              // Search for "http(s)://"
        if ( inx >= 0 )                                 // Does URL contain "http://"?
        {
          metaline = metaline.substring ( inx + 3 ) ;   // Yes, remove it
        }
        presetinfo.host = metaline ;                    // Set host
        presetinfo.hsym = metaline ;                    // Do not know symbolic name
        presetinfo.station_state = ST_PLAYLIST ;        // Set playlist mode
        setdatamode ( INIT ) ;                          // Yes, mode to INIT again
        myQueueSend ( radioqueue, &startcmd ) ;         // Restart with new found host
      }
      metalinebfx = 0 ;                                 // Prepare for next line
      playlistcnt++ ;                                   // Next entry in playlist
    }
  }
}


//**************************************************************************************************
//                                  H A N D L E _ N O T F O U N D                                  *
//**************************************************************************************************
// If parameters are present: handle them.                                                         *
// Otherwise: transfer file from SPIFFS to webserver client.                                       *
//**************************************************************************************************
void handle_notfound ( AsyncWebServerRequest *request )
{
  String       ct = String ( "text/plain" ) ;         // Default content type
  String       path ;                                 // Filename for SPIFFS
  String       reply ;                                // Reply on not file request
  const char*  p ;                                    // Reply from analyzecmd
  int          numargs ;                              // Number of arguments
  int          i ;                                    // Index in arguments
  String       key ;                                  // Name of parameter i
  String       contents ;                             // Value of parameter i
  String       cmd ;                                  // Command to analyze
  String       sndstr = String() ;                    // String to send

  numargs = request->params() ;                       // Haal aantal parameters
  for ( i = 0 ; i < numargs ; i++ )                   // Scan de parameters
  {
    key = request->argName ( i ) ;                    // Get name (key)
    contents = request->arg ( i ) ;                   // Get value
    chomp ( key ) ;                                   // Remove leading/trailing spaces
    chomp ( contents ) ;                              // Remove leading/trailing spaces
    if ( key == "version" )                           // Skip "version" parameter
    {
      continue ;
    }
    cmd = key + String ( "=" ) + contents ;           // Format command to analyze
    //ESP_LOGI ( TAG, "Http command is %s", cmd.c_str() ) ;
    p = analyzeCmd ( cmd.c_str() ) ;                  // Analyze command
    sndstr += String ( p ) ;                          // Content of HTTP response follows the header
  }
  if ( ! sndstr.isEmpty() )                           // Any argument handled?
  {
    request->send ( 200, ct, sndstr ) ;               // Send reply
    return ;                                          // Quick return
  }
  // No parameters, it is just a request for a new page, style sheet or icon
  path = request->url() ;                             // Path for requested filename
  if ( path == String ( "/" ) )                       // Default is index.html
  {
    path = String ( "/index.html" ) ;                 // Select index.html
  }
  #ifndef SDCARD
    if ( path == "/mp3play.html" )                    // MP3 player page requested?
    {
      path = "/nomp3play.html" ;                      // Yes, select dummy mp3 player page
    }
  #endif
  if ( SPIFFS.exists ( path ) )                       // Does it exist in SPIFFS?
  {
    ct = getContentType ( path.c_str() ) ;            // Get content type
    request->send ( SPIFFS, path, ct ) ;              // Send to client
  }
  else
  {
    request->send ( 200, ct, String ( "sorry" ) ) ;   // Send reply
  }
}


//**************************************************************************************************
//                                         C H O M P                                               *
//**************************************************************************************************
// Do some filtering on de inputstring:                                                            *
//  - String comment part (starting with "#").                                                     *
//  - Strip trailing CR.                                                                           *
//  - Strip leading spaces.                                                                        *
//  - Strip trailing spaces.                                                                       *
//**************************************************************************************************
void chomp ( String &str )
{
  int   inx ;                                         // Index in de input string

  if ( ( inx = str.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    str.remove ( inx ) ;                              // Yes, remove
  }
  str.trim() ;                                        // Remove spaces and CR
}


//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Version for handling string with: <parameter>=<value>                                           *
//**************************************************************************************************
const char* analyzeCmd ( const char* str )
{
  char*        value ;                           // Points to value after equalsign in command
  const char*  res ;                             // Result of analyzeCmd

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    res = analyzeCmd ( str, value + 1 ) ;        // Analyze command and handle it
    *value = '=' ;                               // Restore equal sign
  }
  else
  {
    res = analyzeCmd ( str, "0" ) ;              // No value, assume zero
  }
  return res ;
}


//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, serial or MQTT.                         *
// par holds the parametername and val holds the value.                                            *
// "wifi_00" and "preset_00" may appear more than once, like wifi_01, wifi_02, etc.                *
// Examples with available parameters:                                                             *
//   preset     = 12                        // Select start preset to connect to                   *
//   track      = songname                  // Select MP3 track from SD card                       *
//   random                                 // Select random mP3 track                             *
//   preset_00  = <mp3 stream>              // Specify station for a preset 00-max *)              *
//   volume     = 95                        // Percentage between 0 and 100                        *
//   upvolume   = 2                         // Add percentage to current volume                    *
//   downvolume = 2                         // Subtract percentage from current volume             *
//   toneha     = <0..15>                   // Setting treble gain                                 *
//   tonehf     = <0..15>                   // Setting treble frequency                            *
//   tonela     = <0..15>                   // Setting bass gain                                   *
//   tonelf     = <0..15>                   // Setting treble frequency                            *
//   station    = <mp3 stream>              // Select new station (will not be saved)              *
//   station    = <URL>.mp3                 // Play standalone .mp3 file (not saved)               *
//   station    = <URL>.m3u                 // Select playlist (will not be saved)                 *
//   resume                                 // Resume playing                                      *
//   (un)mute                               // Mute/unmute the music                               *
//   sleep                                  // Go into deep sleep mode                             *
//   wifi_00    = mySSID/mypassword         // Set WiFi SSID and password *)                       *
//   mqttbroker = mybroker.com              // Set MQTT broker to use *)                           *
//   mqttprefix = XP93g                     // Set MQTT broker to use                              *
//   mqttport   = 1883                      // Set MQTT port to use, default 1883 *)               *
//   mqttuser   = myuser                    // Set MQTT user for authentication *)                 *
//   mqttpasswd = mypassword                // Set MQTT password for authentication *)             *
//   mqttrefresh                            // Refresh all MQTT items                              *
//   clk_server = pool.ntp.org              // Time server to be used *)                           *
//   clk_offset = <-11..+14>                // Offset with respect to UTC in hours *)              *
//   clk_dst    = <1..2>                    // Offset during daylight saving time in hours *)      *
//   settings                               // Returns setting like presets and tone               *
//   status                                 // Show current URL to play                            *
//   mp3list                                // Returns list with all MP3 files                     *
//   test                                   // For test purposes                                   *
//   reset                                  // Restart the ESP32                                   *
//   bat0       = 2318                      // ADC value for an empty battery                      *
//   bat100     = 2916                      // ADC value for a fully charged battery               *
//  Commands marked with "*)" are sensible during initialization only                              *
//**************************************************************************************************
const char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  String             tmpstr ;                         // Temporary storage of a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[180] ;                     // Reply to client, will be returned
  bool               relative = false ;               // Relative argument (+ or -)

  blset ( true ) ;                                    // Enable backlight of TFT
  strcpy ( reply, "Command accepted" ) ;              // Default reply
  argument = String ( par ) ;                         // Get the argument
  chomp ( argument ) ;                                // Remove comment and useless spaces
  if ( argument.length() == 0 )                       // Lege commandline (comment)?
  {
    return reply ;                                    // Ignore
  }
  argument.toLowerCase() ;                            // Force to lower case
  value = String ( val ) ;                            // Get the specified value
  chomp ( value ) ;                                   // Remove comment and extra spaces
  ivalue = value.toInt() ;                            // Also as an integer
  if ( ( relative = argument.startsWith ( "up" ) ) )  // + relative setting?
  {
    argument = argument.substring ( 2 ) ;             // Remove the "up"-part
  }
  else if ( ( relative = 
                 argument.startsWith ( "down" )  ) )  // - relative setting?
  {
    ivalue = -ivalue ;                                // But with negative value
    argument = argument.substring ( 4 ) ;             // Remove the "down"-part
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  else if ( argument == "volume" )                    // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = player_getVolume() +         // Up/down by 0.5 or more dB
                         ivalue ;
    }
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
    if ( ini_block.reqvol > 127 )                     // Wrapped around?
    {
      ini_block.reqvol = 0 ;                          // Yes, keep at zero
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
    muteflag = false ;                                // Stop possibly muting
    sprintf ( reply, "Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
  }
  else if ( argument.indexOf ( "mute" ) >= 0 )        // Mute/unmute request
  {
    muteflag = ( argument == "mute" ) ;               // Request volume to zero/normal
  }
  else if ( argument.startsWith ( "ir_" ) )           // Ir setting?
  { // Do not handle here
  }
  else if ( argument.startsWith ( "preset_" ) )       // Enumerated preset?
  {
    ivalue = argument.substring ( 7 ).toInt() ;       // Only look for max
  }
  else if ( argument == "preset" )                    // (UP/DOWN)Preset station?
  {
    nextPreset ( ivalue, relative ) ;                 // Yes, set new preset
    sprintf ( reply, "Preset is now %d",              // Reply new preset
              presetinfo.preset ) ;
    if ( NetworkFound )
    myQueueSend ( radioqueue, &startcmd ) ;           // Signal radiofuncs()
  }
#ifdef SDCARD
  else if ( argument == "track" )                     // MP3 track request?
  {
    if ( relative )                                   // Yes. "uptrack" has numeric value
    {
      getSDFileName ( SD_curindex + ivalue ) ;        // Select next file
    }
    else
    {
      setSDFileName ( value.c_str() ) ;               // Select new track by filename
    }
    myQueueSend ( sdqueue, &startcmd ) ;              // Signal SDfuncs()
  }
  else if ( argument == "random" )                    // Random MP3 track request?
  {
    getSDFileName ( -1 ) ;                            // Yes, select new random track
    ESP_LOGI ( TAG, "Random file is %s",              // Show filename
               getCurrentSDFileName() ) ;
    myQueueSend ( sdqueue, &startcmd ) ;              // Signal SDfuncs()
  }
#endif
  else if ( ( value.length() > 0 ) &&
            ( argument == "station" ) )               // Station in the form address:port
  {
    presetinfo.host = value ;                         // Save it for storage and selection later
    presetinfo.hsym = value ;                         // We do not know the symbolic name
    presetinfo.station_state = ST_STATION ;           // Set station mode
    myQueueSend ( radioqueue, &startcmd ) ;           // Signal radiofuncs()
    sprintf ( reply,
              "Select %s",                            // Format reply
              value.c_str() ) ;
    utf8ascii_ip ( reply ) ;                          // Remove possible strange characters
  }
  else if ( argument == "sleep" )                     // Sleep request?
  {
    sleepreq = true ;                                 // Yes, set request flag
  }
  else if ( argument == "status" )                    // Status request
  {
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
    }
    else
    {
      sprintf ( reply, "%s - %s", icyname.c_str(),
                icystreamtitle.c_str() ) ;            // Streamtitle from metadata
    }
  }
  else if ( argument == "reset" )                     // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument == "test" )                      // Test command
  {
    sprintf ( reply, "Free memory is %d/%d, "         // Get some info to display
              "chunks in queue %d, bitrate %d kbps\n",
              heapspace,
              ESP.getFreeHeap(),
              uxQueueMessagesWaiting ( dataqueue ),
              mbitrate ) ;
    testreq = true ;                                  // Request to print info in main program
  }
  // Commands for bass/treble control
  else if ( argument.startsWith ( "tone" ) )          // Tone command
  {
    if ( argument.indexOf ( "ha" ) > 0 )              // High amplitue? (for treble)
    {
      ini_block.rtone[0] = ivalue ;                   // Yes, prepare to set ST_AMPLITUDE
    }
    if ( argument.indexOf ( "hf" ) > 0 )              // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue ;                   // Yes, prepare to set ST_FREQLIMIT
    }
    if ( argument.indexOf ( "la" ) > 0 )              // Low amplitue? (for bass)
    {
      ini_block.rtone[2] = ivalue ;                   // Yes, prepare to set SB_AMPLITUDE
    }
    if ( argument.indexOf ( "lf" ) > 0 )              // High frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue ;                   // Yes, prepare to set SB_FREQLIMIT
    }
    reqtone = true ;                                  // Set change request
    sprintf ( reply, "Parameter for bass/treble %s set to %d",
              argument.c_str(), ivalue ) ;
  }
  else if ( argument == "rate" )                      // Rate command?
  {
    player_AdjustRate ( ivalue ) ;                    // Yes, adjust
  }
  else if ( argument.startsWith ( "mqtt" ) )          // Parameter fo MQTT?
  {
    strcpy ( reply, "MQTT broker parameter changed. Save and restart to have effect" ) ;
    if ( argument.indexOf ( "broker" ) > 0 )          // Broker specified?
    {
      ini_block.mqttbroker = value ;                  // Yes, set broker accordingly
    }
    else if ( argument.indexOf ( "prefix" ) > 0 )     // Port specified?
    {
      ini_block.mqttprefix = value ;                  // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "port" ) > 0 )       // Port specified?
    {
      ini_block.mqttport = ivalue ;                   // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "user" ) > 0 )       // User specified?
    {
      ini_block.mqttuser = value ;                    // Yes, set user accordingly
    }
    else if ( argument.indexOf ( "passwd" ) > 0 )     // Password specified?
    {
      ini_block.mqttpasswd = value.c_str() ;          // Yes, set broker password accordingly
    }
    else if ( argument.indexOf ( "refresh" ) > 0 )    // Refresh all items?
    {
      mqttpub.triggerall() ;                          // Yes, request to republish all items
    }
  }
  else if ( argument.startsWith ( "clk_" ) )          // TOD parameter?
  {
    if ( argument.indexOf ( "server" ) > 0 )          // Yes, NTP server spec?
    {
      ini_block.clk_server = value ;                  // Yes, set server
    }
    if ( argument.indexOf ( "offset" ) > 0 )          // Offset with respect to UTC spec?
    {
      ini_block.clk_offset = value.toInt() ;          // Yes, set offset
    }
    if ( argument.indexOf ( "dst" ) > 0 )             // Offset duringe DST spec?
    {
      ini_block.clk_dst = value.toInt() ;             // Yes, set DST offset
    }
  }
  else if ( argument.startsWith ( "bat" ) )           // Battery ADC value?
  {
    if ( argument.indexOf ( "100" ) )                 // 100 percent value?
    {
      ini_block.bat100 = ivalue ;                     // Yes, set it
    }
    else if ( argument.indexOf ( "0" ) )              // 0 percent value?
    {
      ini_block.bat0 = ivalue ;                       // Yes, set it
    }
  }
  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//**************************************************************************************************
//* Function that are called from spfunc().                                                        *
//* Note that some device dependent function are place in the *.h files.                           *
//**************************************************************************************************

//**************************************************************************************************
//                                      D I S P L A Y I N F O                                      *
//**************************************************************************************************
// Show a string on the LCD at a specified y-position (0..2) in a specified color.                 *
// The parameter is the index in tftdata[].                                                        *
//**************************************************************************************************
void displayinfo ( uint16_t inx )
{
  uint16_t       width = dsp_getwidth() ;                  // Normal number of colums
  scrseg_struct* p = &tftdata[inx] ;
  uint16_t len ;                                           // Length of string, later buffer length

  if ( inx == 0 )                                          // Topline is shorter
  {
    width += TIMEPOS ;                                     // Leave space for time
  }
  if ( dsp_ok )                                            // TFT active?
  {
    dsp_fillRect ( 0, p->y, width, p->height, BLACK ) ;    // Clear the space for new info
    if ( ( dsp_getheight() > 64 ) && ( p->y > 1 ) )        // Need and space for divider?
    {
      dsp_fillRect ( 0, p->y - 4, width, 1, GREEN ) ;      // Yes, show divider above text
    }
    len = p->str.length() ;                                // Required length of buffer
    if ( len++ )                                           // Check string length, set buffer length
    {
      char buf [ len ] ;                                   // Need some buffer space
      p->str.toCharArray ( buf, len ) ;                    // Make a local copy of the string
      utf8ascii_ip ( buf ) ;                               // Convert possible UTF8
      dsp_setTextColor ( p->color ) ;                      // Set the requested color
      dsp_setCursor ( 0, p->y ) ;                          // Prepare to show the info
      dsp_println ( buf ) ;                                // Show the string
    }
  }
}


//**************************************************************************************************
//                                         G E T T I M E                                           *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string.                                  *
// Will be called every second.                                                                    *
//**************************************************************************************************
void gettime()
{
  static int16_t delaycount = 0 ;                         // To reduce number of NTP requests
  static int16_t retrycount = 100 ;

  if ( --delaycount <= 0 )                                // Sync every few hours
  {
    delaycount = 7200 ;                                   // Reset counter
    if ( timeinfo.tm_year )                               // Legal time found?
    {
      ESP_LOGI ( TAG, "Sync TOD, old value is %s", timetxt ) ;
    }
    ESP_LOGI ( TAG, "Sync TOD" ) ;
    if ( !getLocalTime ( &timeinfo ) )                    // Read from NTP server
    {
      ESP_LOGW ( TAG, "Failed to obtain time!" ) ;        // Error
      timeinfo.tm_year = 0 ;                              // Set current time to illegal
      if ( retrycount )                                   // Give up syncing?
      {
        retrycount-- ;                                    // No try again
        delaycount = 5 ;                                  // Retry after 5 seconds
      }
    }
    else
    {
      ESP_LOGI ( TAG, "TOD synced" ) ;                    // Time has been synced
    }
  }
  sprintf ( timetxt, "%02d:%02d:%02d",                    // Format new time to a string
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec ) ;
}

#if defined(DEC_VS1053) || defined(DEC_VS1003)

//**************************************************************************************************
//                         P L A Y T A S K  ( V S 1 0 5 3 )                                        *
//**************************************************************************************************
// Play stream data from input queue. Version for VS1053.                                          *
// Handle all I/O to VS1053B during normal playing.                                                *
//**************************************************************************************************
void playtask ( void * parameter )
{
  // static bool once = true ;                                      // Show chunk once  #if defined(DEC_VS1053) || defined(DEC_VS1003)
  bool VS_okay ;                                                    // VS isw okay or not

  ESP_LOGI ( TAG, "Starting VS1053 playtask.." ) ;
  VS_okay = VS1053_begin ( ini_block.vs_cs_pin,                     // Make instance of player and initialize
                           ini_block.vs_dcs_pin,
                           ini_block.vs_dreq_pin,
                           ini_block.shutdown_pin,
                           ini_block.shutdownx_pin ) ;
  while ( true )
  {
    if ( xQueueReceive ( dataqueue, &inchunk, 5 ) == pdTRUE )       // Command/data from queue?
    {
      if ( VS_okay )
      {
        switch ( inchunk.datatyp )                                    // What kind of command?
        {
          case QDATA:
            while ( !vs1053player->data_request() )                   // If hardware FIFO is full..
            {
              vTaskDelay ( 1 ) ;                                      // Yes, take a break
            }
            // if ( once )                                            // Show this chunk?
            // {
            //   Serial.printf ( "First chunk to play (HEX):" ) ;     // Yes, show for testing purpose
            //   for ( int i = 0 ; i < 32 ; i++ )
            //   {
            //     Serial.printf ( " %02X", inchunk.buf[i] ) ; 
            //   }
            //   Serial.printf ( "\n" ) ;
            //   once = false ;                                       // Just show once
            // }
            vs1053player->playChunk ( inchunk.buf,                    // DATA, send to player
                                      sizeof(inchunk.buf) ) ;
            totalcount += sizeof(inchunk.buf) ;                       // Count the bytes
            break ;
          case QSTARTSONG:
            ESP_LOGI ( TAG, "QSTARTSONG" ) ;
            playingstat = 1 ;                                         // Status for MQTT
            mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
            vs1053player->setVolume ( ini_block.reqvol ) ;            // Unmute
            vs1053player->startSong() ;                               // START, start player
            // once = true ;
            break ;
          case QSTOPSONG:
            ESP_LOGI ( TAG, "QSTOPSONG" ) ;
            playingstat = 0 ;                                         // Status for MQTT
            mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
            vs1053player->setVolume ( 0 ) ;                           // Mute
            vs1053player->stopSong() ;                                // STOP, stop player
            break ;
          case QSTOPTASK:
            vTaskDelete ( NULL ) ;                                    // Stop task
            break ;
          default:
            break ;
        }
      }
    }
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}
#endif

#if defined(DEC_HELIX)
//**************************************************************************************************
//                               P L A Y T A S K ( I 2 S )                                         *
//**************************************************************************************************
// Play stream data from input queue. Version for I2S output or output to internal DAC.            *
// I2S output is suitable for a PCM5102A DAC.                                                      *
// Input are blocks with 32 bytes MP3/AAC data delivered in the data queue.                        *
// Internal ESP32 DAC (pin 25 and 26) is used when no pin BCK is configured.                       *
// Note that the naming of the data pin is somewhat confusing.  The data out pin in the pin        *
// configuration is called data_out_num, but this pin should be connected to the "DIN" pin of the  *
// external DAC.  The variable used to configure this pin is therefore called "i2s_din_pin".       *
// If no pin for i2s_bck is configured, output will be sent to the internal DAC.                   *
// Task will stop on OTA update.                                                                   *
//**************************************************************************************************
void playtask ( void * parameter )
{
  esp_err_t        pinss_err = ESP_FAIL ;                            // Result of i2s_set_pin
  i2s_config_t     i2s_config ;                                      // I2S configuration
  bool             playing = false ;                                 // Are we playing or not?

  memset ( &i2s_config, 0, sizeof(i2s_config) ) ;                    // Clear config struct
  i2s_config.mode                   = (i2s_mode_t)(I2S_MODE_MASTER | // I2S mode (5)
                                          I2S_MODE_TX) ;
  #ifdef DEC_HELIX_SPDIF
    i2s_config.use_apll               = true ;
    i2s_config.sample_rate            = 44100 * 2 ;                  // For spdif: biphase and 32 bits
    i2s_config.bits_per_sample        = I2S_BITS_PER_SAMPLE_32BIT ;  // and 32 bits
    #if ESP_ARDUINO_VERSION_MAJOR >= 2                               // New version?
      i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S ;  // Yes, use new definition
    #else
      i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB) ;
    #endif
  #else
    i2s_config.sample_rate            = 44100 ;                      // 44100
    i2s_config.bits_per_sample        = I2S_BITS_PER_SAMPLE_16BIT ;  // (16)
    #if ESP_ARDUINO_VERSION_MAJOR >= 2                               // New version?
      i2s_config.communication_format = I2S_COMM_FORMAT_STAND_MSB ;  // Yes, use new definition
    #else
      i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB) ;
    #endif
    i2s_config.dma_buf_count        = 8 ;
    i2s_config.dma_buf_len          = 256 ;
  #endif
  i2s_config.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT ;   // (0)
  i2s_config.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1 ;         // High interrupt priority
  i2s_config.dma_buf_count        = 12 ;
  i2s_config.dma_buf_len          = 256 ;
  i2s_config.tx_desc_auto_clear   = true ;                         // clear tx descriptor on underflow
  //i2s_config.fixed_mclk         = 0 ;                            // No (pin for) MCLK
  //i2s_config.mclk_multiple      = I2S_MCLK_MULTIPLE_DEFAULT ;    // = 0
  //i2s_config.bits_per_chan      = I2S_BITS_PER_CHAN_DEFAULT ;    // = 0
  #ifdef DEC_HELIX_INT
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER |               // Set I2S mode for internal DAC
                                   I2S_MODE_TX |                   // (4)
                                   I2S_MODE_DAC_BUILT_IN ) ;       // Enable internal DAC (16)
    #if ESP_ARDUINO_VERSION_MAJOR < 2
      i2s_config.communication_format = I2S_COMM_FORMAT_I2S_MSB ;
    #endif
  #endif
  ESP_LOGI ( TAG, "Starting I2S playtask.." ) ;
  #ifdef DEC_HELIX_AI                                              // For AI board?
    #define IIC_DATA 33                                            // Yes, use these I2C signals
    #define IIC_CLK  32
    if ( ! dac.begin ( IIC_DATA, IIC_CLK ) )                       // Initialize AI dac
    {
      ESP_LOGE ( TAG, "AI dac error!" ) ;
    }
    pinMode ( GPIO_PA_EN, OUTPUT ) ;
    digitalWrite ( GPIO_PA_EN, HIGH ) ;
  #endif
  MP3Decoder_AllocateBuffers() ;                                    // Init HELIX buffers
  AACDecoder_AllocateBuffers() ;                                    // Init HELIX buffers
  if ( i2s_driver_install ( I2S_NUM_0, &i2s_config, 0, NULL ) != ESP_OK )
  {
    ESP_LOGE ( TAG, "I2S install error!" ) ;
  }
  #ifdef DEC_HELIX_INT                                              // Use internal (8 bit) DAC?
    ESP_LOGI ( TAG, "Output to internal DAC" ) ;                    // Show output device
    pinss_err = i2s_set_pin ( I2S_NUM_0, NULL ) ;                   // Yes, default pins for internal DAC
    i2s_set_dac_mode ( I2S_DAC_CHANNEL_BOTH_EN ) ;
  #else
    i2s_pin_config_t pin_config ;
    #if ESP_ARDUINO_VERSION_MAJOR >= 2
      pin_config.mck_io_num   = I2S_PIN_NO_CHANGE ;                 // MCK not used
    #endif
    pin_config.data_in_num    = I2S_PIN_NO_CHANGE ;
    #ifdef DEC_HELIX_SPDIF
      pin_config.bck_io_num   = I2S_PIN_NO_CHANGE ;
      pin_config.ws_io_num    = I2S_PIN_NO_CHANGE ;
      pin_config.data_out_num = ini_block.i2s_spdif_pin ;
      pin_config.data_in_num  = I2S_PIN_NO_CHANGE ;
      ESP_LOGI ( TAG, "Output to SPDIF, pin %d",                    // Show pin used for output device
                 pin_config.data_out_num ) ;
    #else
      pin_config.bck_io_num   = ini_block.i2s_bck_pin ;             // This is BCK pin
      pin_config.ws_io_num    = ini_block.i2s_lck_pin ;             // This is L(R)CK pin
      pin_config.data_out_num = ini_block.i2s_din_pin ;             // This is DATA output pin
      ESP_LOGI ( TAG, "Output to I2S, pins %d, %d and %d",          // Show pins used for output device
                 pin_config.bck_io_num,                             // This is the BCK (bit clock) pin
                 pin_config.ws_io_num,                              // This is L(R)CK pin
                 pin_config.data_out_num ) ;                        // This is DATA output pin
    #endif
    pinss_err = i2s_set_pin ( I2S_NUM_0, &pin_config ) ;            // Set I2S pins
  #endif
  i2s_zero_dma_buffer ( I2S_NUM_0 ) ;                               // Zero the buffer
  if ( pinss_err != ESP_OK )                                        // Check error condition
  {
    ESP_LOGE ( TAG, "I2S setpin error!" ) ;                         // Rport bad pins
    while ( true)                                                   // Forever..
    {
      xQueueReceive ( dataqueue, &inchunk, 500 ) ;                  // Ignore all chunk from queue
    }
  }
  while ( true )
  {
    if ( xQueueReceive ( dataqueue, &inchunk, 5 ) == pdTRUE )       // Command/data from queue?
    {
      switch ( inchunk.datatyp )                                    // Yes, what kind of command?
      {
        case QDATA:
          if ( playing )                                            // Are we playing?
          {
            playChunk ( inchunk.buf ) ;                             // Play this chunk
          }
          totalcount += sizeof(inchunk.buf) ;                       // Count the bytes
          break ;
        case QSTARTSONG:
          ESP_LOGI ( TAG, "Playtask start song" ) ;
          playing = true ;                                          // Set local status to playing
          playingstat = 1 ;                                         // Status for MQTT
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          helixInit ( ini_block.shutdown_pin,                       // Enable amplifier output
                      ini_block.shutdownx_pin ) ;                   // Init framebuffering
          break ;
        case QSTOPSONG:
          ESP_LOGI ( TAG, "Playtask stop song" ) ;
          playing = false ;                                         // Reset local play status
          playingstat = 0 ;                                         // Status for MQTT
          i2s_stop ( I2S_NUM_0 ) ;                                  // Stop DAC
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          //vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;               // Pause for a short time
          break ;
        case QSTOPTASK:
          ESP_LOGI ( TAG, "Stop Playtask" ) ;
          playing = false ;                                         // Reset local play status
          i2s_stop ( I2S_NUM_0 ) ;                                  // Stop DAC
          vTaskDelete ( NULL ) ;                                    // Stop task
          break ;
        default:
          break ;
      }
    }
  }
}
#endif


//**************************************************************************************************
//                                     S D F U N C S                                               *
//**************************************************************************************************
// Handles data of SD card and commands in the sdqueue.                                            *
// Commands are received in the input queue.                                                       *
//**************************************************************************************************
void sdfuncs()
{
#ifdef SDCARD
  qdata_type          sdcmd ;                                     // Command from sdqueue
  static bool         openfile = false ;                          // Open input file available
  static bool         autoplay = true ;                           // Play next after end
  size_t              n ;                                         // Number of bytes read from SD

  if ( openfile )
  {
    while ( ( mp3filelength > 0 ) &&                              // Read until eof or dataqueue full
            ( uxQueueSpacesAvailable ( dataqueue ) > 0 ) )
    {
      n = mp3file.read ( outchunk.buf, sizeof(outchunk.buf) ) ;   // Read a block of data
      if ( n < sizeof(outchunk.buf) )                             // Incomplete chunk?
      {
        memset ( outchunk.buf + n, 0,                             // Yes, clear rest
                 sizeof(outchunk.buf) - n ) ;
      }
      xQueueSend ( dataqueue, &outchunk, 0 ) ;                    // Send to queue
      mp3filelength -= n ;                                        // Compute rest in file
      if ( mp3filelength == 0 )                                   // End of file?
      {
        vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;                 // Give some time to finish song
        myQueueSend ( sdqueue, &stopcmd ) ;                       // Stop message to myself
        ESP_LOGI ( TAG, "EOF" ) ;
        queueToPt ( QSTOPSONG ) ;                                 // Tell playtask to stop song
        if ( autoplay )                                           // Continue with next track?
        {
          ESP_LOGI ( TAG, "Autoplay next track" ) ;
          getNextSDFileName() ;                                   // Select next track
          myQueueSend ( sdqueue, &startcmd ) ;                    // Start message to myself
        }
      }
    }
  }
  if ( xQueueReceive ( sdqueue, &sdcmd, 0 ) == pdTRUE )           // New command in queue?
  {
    ESP_LOGI ( TAG, "SDfuncs cmd is %d", sdcmd ) ;
    switch ( sdcmd )                                              // Yes, examine command
    {
      case QSTARTSONG:                                            // Start a new song?
        if ( openfile )                                           // Still playing?
        {
          close_SDCARD() ;                                        // Clode file
        }
        queueToPt ( QSTOPSONG ) ;                                 // Tell playtask to stop song
        if ( ( openfile = connecttofile_SD() ) )                  // Yes, connect to file, set mp3filelength
        {
          ESP_LOGI ( TAG, "File opened, track = %s",
                     getCurrentSDFileName() ) ;
          ESP_LOGI ( TAG, "File length is %d", mp3filelength ) ;
          audio_ct = String ( "audio/mpeg" ) ;                    // Force mp3 mode
          myQueueSend ( radioqueue, &stopcmd ) ;                  // Stop playing icecast station
          radiofuncs() ;                                          // Allow radiofuncs to react
          queueToPt ( QSTARTSONG ) ;                              // Tell playtask
          autoplay = true ;                                       // Set autoplay mode
        }
        else
        {
          ESP_LOGI ( TAG, "Error opening file" ) ;
        }
        break ;
      case QSTOPSONG:                                             // Stop the song?
        if ( openfile )                                           // Still playing?
        {
          close_SDCARD() ;                                        // Clode file
          mp3filelength = 0 ;                                     // Yes, force end of file
          autoplay = false ;                                      // Stop autoplay
        }
      default:
        break ;
    }
  }
#endif
}

