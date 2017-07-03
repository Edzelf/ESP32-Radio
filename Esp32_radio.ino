//*  Esp32_radio -- Webradio receiver for ESP32, 1.8 color display and VS1053 MP3 module.   *
//*******************************************************************************************
// ESP32 libraries used:
//  - WiFi
//  - WiFiMulti
//  - nvs
//  - SPI
//  - Adafruit_GFX
//  - TFT_ILI9163C
//  - ArduinoOTA
//  - PubSubClient
//  - TinyXML
//  - SD
//  - FS
// A library for the VS1053 (for ESP32) is not available (or not easy to find).  Therefore
// a class for this module is derived from the maniacbug library and integrated in this sketch.
//
// See http://www.internet-radio.com for suitable stations.  Add the stations of your choice
// to the preferences in either Esp32_radio_init.ino sketch or through the webinterface.
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
// The metadata is empty in most cases, but if any is available the content will be presented on the TFT.
// Pushing an input button causes the player to execute a programmable command.
//
// The display used is a Chinese 1.8 color TFT module 128 x 160 pixels.  The TFT_ILI9163C.h
// file has been changed to reflect this particular module.  TFT_ILI9163C.cpp has been
// changed to use the full screenwidth if rotated to mode "3".  Now there is room for 26
// characters per line and 16 lines.  Software will work without installing the display.
//
// For configuration of the WiFi network(s): see the global data section further on.
//
// The VSPI interface is used for VS1053 and TFT.
//
// Wiring, note that Featherbord has other connections:
// ESP32dev Signal  Wired to LCD        Wired to VS1053      SDCARD   Wired to the rest
// -------- ------  --------------      -------------------  ------   ---------------
// GPIO16           -                   pin 1 XDCS                    -
// GPIO5            -                   pin 2 XCS                     -
// GPIO4            -                   pin 4 DREQ                    -
// GPIO2            pin 3 D/C           -                             -
// GPIO18   SCLK    pin 5 CLK           pin 5 SCK             CLK     -
// GPIO19   MISO    -                   pin 7 MISO            MISO    -
// GPIO23   MOSI    pin 4 DIN           pin 6 MOSI            MOSI    -
// GPIO21           -                   -                     CS      -
// GPIO15           pin 2 CS            -                             -
// GPI03    RXD0    -                   -                             Reserved serial input
// GPIO1    TXD0    -                   -                             Reserved serial output
// GPIO34   -       -                   -                             Optional pull-up resistor
// GPIO35   -       -                   -                             Infrared receiver VS1838B
// -------  ------  ---------------     -------------------  ------   ----------------
// GND      -       pin 8 GND           pin 8 GND                     Power supply GND
// VCC 5 V  -       pin 7 BL            -                             Power supply
// VCC 5 V  -       pin 6 VCC           pin 9 5V                      Power supply
// EN       -       pin 1 RST           pin 3 XRST                    -
//
// 26-04-2017, ES: First set-up, derived from ESP8266 version.
// 08-05-2017, ES: Handling of preferences.
// 20-05-2017, ES: Handling input buttons and MQTT.
// 22-05-2017, ES: Save preset, volume and tone settings.
// 23-05-2017, ES: No more calls of non-iram functions on interrupts.
// 24-05-2017, ES: Support for featherboard.
// 26-05-2017, ES: Correction playing from .m3u playlist. Allow single hidden SSID.
// 30-05-2017, ES: Add SD card support (FAT format), volume indicator.
// 26-06-2017, ES: Correction: start in AP-mode if no WiFi networks configured.
// 28-06-2017, ES: Added IR interface.
// 30-06-2017, ES: Improved functions for SD card play.
//
//
// Define the version number, also used for webserver as Last-Modified header:
#define VERSION "Fri, 30 Jun 2017 10:50:00 GMT"

// TFT.  Define USETFT if required.
#define USETFT
#include <WiFi.h>
#include <nvs.h>
#include <PubSubClient.h>
#include <WiFiMulti.h>
#include <SPI.h>
#if defined ( USETFT )
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#endif
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include "SD.h"
#include <ArduinoOTA.h>
#include <TinyXML.h>

// Color definitions for the TFT screen (if used)
#define	BLACK   0x0000
#define	BLUE    0xF800
#define	RED     0x001F
#define	GREEN   0x07E0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
#define WHITE   BLUE | RED | GREEN
// Digital I/O used
// Pins for VS1053 module
#define Qantum_MP3_shield 1
#if defined( Qantum_MP3_shield )
#define VS1053_CS     26
#define VS1053_DCS    27
#define VS1053_DREQ   2
#define SDCARDCS      21
#elif defined ( ARDUINO_FEATHER_ESP32 )
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15
#define SDCARDCS      14
#else
#define VS1053_CS     5
#define VS1053_DCS    16
#define VS1053_DREQ   4
#define SDCARDCS      21
#endif
// Pins CS and DC for TFT module (if used, see definition of "USETFT")
#define TFT_CS 15
#define TFT_DC 2
// Pin for IR receiver
#define IR_PIN 35
// Ringbuffer for smooth playing. 20000 bytes is 160 Kbits, about 1.5 seconds at 128kb bitrate.
// Use a multiple of 1024 for optimal handling of bufferspace.  See definition of tmpbuff.
#define RINGBFSIZ 20480
// Debug buffer size
#define DEBUG_BUFFER_SIZE 130
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Not that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "ESP32Radio"
// Maximum number of MQTT reconnects before give-up
#define MAXMQTTCONNECTS 20
// Adjust size of buffer to the longest expected string for nvsgetstr
#define NVSBUFSIZE 150
//
// Subscription topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
// by the the mqttprefix in the preferences.  The next definition will yield the topic "ESP32Radio/command"
// if mqttprefix is "ESP32Radio".
#define MQTT_SUBTOPIC     "command"           // Command to receive from MQTT
//
//******************************************************************************************
// Forward declaration of various functions                                                *
//******************************************************************************************
void        displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color ) ;
void        showstreamtitle ( const char* ml, bool full = false ) ;
void        handlebyte ( uint8_t b, bool force = false ) ;
void        handlebyte_ch ( uint8_t b, bool force = false ) ;
void        handleFSf ( const String& pagename ) ;
void        handleCmd()  ;
char*       dbgprint( const char* format, ... ) ;
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void        chomp ( String &str ) ;
String      httpheader ( String contentstype ) ;

//
//******************************************************************************************
// Global data section.                                                                    *
//******************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is      *
// saved in the preferences by the webinterface.  On restart the new data will             *
// de read from these preferences.                                                         *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.               *
//******************************************************************************************
struct ini_struct
{
  String         mqttbroker ;                              // The name of the MQTT broker server
  String         mqttprefix ;                              // Prefix to use for topics
  uint16_t       mqttport ;                                // Port, default 1883
  String         mqttuser ;                                // User for MQTT authentication
  String         mqttpasswd ;                              // Password for MQTT authentication
  uint8_t        reqvol ;                                  // Requested volume
  uint8_t        rtone[4] ;                                // Requested bass/treble settings
  int8_t         newpreset ;                               // Requested preset
} ;

struct progpin_struct                                      // For programmable input pins
{
  int8_t         gpio ;                                    // Pin number
  bool           avail ;                                   // Pin is available for a command
  String         command ;                                 // Command to execute when activated
  bool           cur ;                                     // Current state
  bool           changed ;                                 // Change of state seen
} ;

enum datamode_t { INIT = 1, HEADER = 2, DATA = 4,          // State for datastream
                  METADATA = 8, PLAYLISTINIT = 16,
                  PLAYLISTHEADER = 32, PLAYLISTDATA = 64,
                  STOPREQD = 128, STOPPED = 256
                } ;

// Global variables
int              DEBUG = 1 ;                               // Debug on/off
WiFiMulti        wifiMulti ;                               // Possible WiFi networks
ini_struct       ini_block ;                               // Holds configurable data
WiFiServer       cmdserver ( 80 ) ;                        // Instance of embedded webserver on port 80
WiFiClient       mp3client ;                               // An instance of the mp3 client
WiFiClient       cmdclient ;                               // An instance of the client for commands
WiFiClient       wmqttclient ;                             // An instance for mqtt
PubSubClient     mqttclient ( wmqttclient ) ;              // Client for MQTT subscriber
hw_timer_t*      timer = NULL ;                            // For timer
char             cmd[130] ;                                // Command from MQTT or Serial
#if defined ( USETFT )
TFT_ILI9163C     tft = TFT_ILI9163C ( TFT_CS, TFT_DC ) ;
#endif
uint32_t         totalcount = 0 ;                          // Counter mp3 data
datamode_t       datamode ;                                // State of datastream
int              metacount ;                               // Number of bytes in metadata
int              datacount ;                               // Counter databytes before metadata
String           metaline ;                                // Readable line in metadata
String           icystreamtitle ;                          // Streamtitle from metadata
String           icyname ;                                 // Icecast station name
String           ipaddress ;                               // Own IP-address
int              bitrate ;                                 // Bitrate in kb/sec
int              metaint = 0 ;                             // Number of databytes between metadata
int8_t           currentpreset = -1 ;                      // Preset station playing
String           host ;                                    // The URL to connect to or file to play
String           playlist ;                                // The URL of the specified playlist
bool             hostreq = false ;                         // Request for new host
bool             reqtone = false ;                         // New tone setting requested
bool             muteflag = false ;                        // Mute output
uint8_t*         ringbuf ;                                 // Ringbuffer for VS1053
uint16_t         rbwindex = 0 ;                            // Fill pointer in ringbuffer
uint16_t         rbrindex = RINGBFSIZ - 1 ;                // Emptypointer in ringbuffer
uint16_t         rcount = 0 ;                              // Number of bytes in ringbuffer
bool             resetreq = false ;                        // Request to reset the ESP32
bool             NetworkFound = false ;                    // True if WiFi network connected
bool             mqtt_on = false ;                         // MQTT in use
String           networks ;                                // Found networks
String           anetworks ;                               // Aceptable networks (present in preferences)
String           presetlist ;                              // List for webserver
uint8_t          num_an ;                                  // Number of acceptable networks in preferences
uint16_t         mqttcount = 0 ;                           // Counter MAXMQTTCONNECTS
int8_t           playlist_num = 0 ;                        // Nonzero for selection from playlist
File             mp3file ;                                 // File containing mp3 on SPIFFS
bool             localfile = false ;                       // Play from local mp3-file or not
bool             chunked = false ;                         // Station provides chunked transfer
int              chunkcount = 0 ;                          // Counter for chunked transfer
String           http_getcmd ;                             // Contents of last GET command
String           http_rqfile ;                             // Requested file
bool             http_reponse_flag = false ;               // Response required
String           lssid, lpw ;                              // Last read SSID and password from wifi_xx
bool             SDokay = false ;                          // True if SD card in place and readable
uint16_t         ir_value = 0 ;                            // IR code
// XML parse globals.
TinyXML     xml ;                                          // For XML parser
const char* xmlhost = "playerservices.streamtheworld.com" ;// XML data source
const char* xmlget =  "GET /api/livestream"                // XML get parameters
                      "?version=1.5"                       // API Version of IHeartRadio
                      "&mount=%sAAC"                       // MountPoint with Station Callsign
                      "&lang=en" ;                         // Language
int         xmlport = 80 ;                                 // XML Port
bool        xmlreq = false ;                               // Request for XML parse.
uint8_t     xmlbuffer[150] ;                               // For XML decoding
String      xmlOpen ;                                      // Opening XML tag
String      xmlTag ;                                       // Current XML tag
String      xmlData ;                                      // Data inside tag
String      stationServer ;                                // Radio stream server
String      stationPort ;                                  // Radio stream port
String      stationMount ;                                 // Radio stream Callsign
String      SD_nodelist ;                                  // Nodes of mp3-files on SD
int         SD_nodecount = 0 ;                             // Number of nodes in SD_nodelist
String      SD_currentnode ;                               // Node ID of song currently playing ("0" if random)
// nvs stuff
esp_err_t   nvserr ;                                       // Error code from nvs functions
uint32_t    nvshandle = 0 ;                                // Handle for nvs access

//
progpin_struct   progpin[] =                               // Input pins with programmed function
{
  {  0, true,  "uppreset=1", false, false },               // Example.  Can be re-programmed.
  { 12, false,  "",          false, false },
  { 13, false,  "",          false, false },
  { 14, false,  "",          false, false },
  { 17, false,  "",          false, false },
  { 22, false,  "",          false, false },
  { 25, false,  "",          false, false },
  { 26, false,  "",          false, false },
  { 27, false,  "",          false, false },
  { 32, false,  "",          false, false },
  { 33, false,  "",          false, false },
  { 34, false,  "",          false, false },               // Note, no pull-up
  { -1, false,  "",          false, false }                // End of list
} ;


//******************************************************************************************
//                             M Q T T P U B _ C L A S S                                   *
//******************************************************************************************
// ID's for the items to publish to MQTT.  Is index in amqttpub[]
enum { MQTT_IP, MQTT_ICYNAME, MQTT_STREAMTITLE, MQTT_NOWPLAYING } ;

class mqttpubc                                             // For MQTT publishing
{
    struct mqttpub_struct
    {
      const char*    topic ;                                 // Topic as partial string (without prefix)
      String*        payload ;                               // Payload for this topic
      bool           topictrigger ;
    } ;
    // Publication topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
    // by the the mqttprefix in the preferences.
  protected:
    mqttpub_struct amqttpub[5] =                           // Definitions of various MQTT topic to publish
    { // Index is equal to enum above
      { "ip",              &ipaddress,      false },     // Definition for MQTT_IP
      { "icy/name",        &icyname,        false },     // Definition for MQTT_ICYNAME
      { "icy/streamtitle", &icystreamtitle, false },     // Definition for MQTT_STREAMTITLE
      { "nowplaying",      &ipaddress,      false },     // Definition for MQTT_NOWPLAYING (not active)
      { NULL,              NULL,            false }      // End of definitions
    } ;
  public:
    void          trigger ( uint8_t item ) ;                // Trigger publishig for one item
    void          publishtopic() ;                          // Publish triggerer items
} ;

//******************************************************************************************
// MQTTPUB  class implementation.                                                          *
//******************************************************************************************

//******************************************************************************************
//                            T R I G G E R                                                *
//******************************************************************************************
// Set request for an item to publish to MQTT.                                             *
//******************************************************************************************
void mqttpubc::trigger ( uint8_t item )                // Trigger publishig for one item
{
  amqttpub[item].topictrigger = true ;                 // Request re-publish for an item
}

//******************************************************************************************
//                            P U B L I S H T O P I C                                      *
//******************************************************************************************
// Publish a topic to MQTT broker.                                                         *
//******************************************************************************************
void mqttpubc::publishtopic()
{
  int         i = 0 ;                                         // Loop control
  char        topic[40] ;                                     // Topic to send
  const char* payload ;                                       // Points to payload

  while ( amqttpub[i].topic )
  {
    if ( amqttpub[i].topictrigger )                           // Topic ready to send?
    {
      amqttpub[i].topictrigger = false ;                      // Success or not: clear trigger
      sprintf ( topic, "%s/%s", ini_block.mqttprefix.c_str(),
                amqttpub[i].topic ) ;                 // Add prefix to topic
      payload = (*amqttpub[i].payload).c_str() ;              // Get payload
      dbgprint ( "Publish to topic %s : %s",                  // Show for debug
                 topic, payload ) ;
      if ( !mqttclient.publish ( topic, payload ) )           // Publish!
      {
        dbgprint ( "MQTT publish failed!" ) ;                 // Failed
      }
      return ;                                                // Do the rest later
    }
    i++ ;                                                     // Next entry
  }
}

mqttpubc mqttpub ;                                            // Instance for mqttpubc

//******************************************************************************************
// End of global data section.                                                             *
//******************************************************************************************

//******************************************************************************************
// Pages, CSS and data for the webinterface.                                               *
//******************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
#include "mp3play_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
#include "defaultprefs.h"
//
//******************************************************************************************
// VS1053 stuff.  Based on maniacbug library.                                              *
//******************************************************************************************
// VS1053 class definition.                                                                *
//******************************************************************************************
class VS1053
{
  private:
    uint8_t       cs_pin ;                        // Pin where CS line is connected
    uint8_t       dcs_pin ;                       // Pin where DCS line is connected
    uint8_t       dreq_pin ;                      // Pin where DREQ line is connected
    uint8_t       curvol ;                        // Current volume setting 0..100%
    const uint8_t vs1053_chunk_size = 32 ;
    // SCI Register
    const uint8_t SCI_MODE          = 0x0 ;
    const uint8_t SCI_BASS          = 0x2 ;
    const uint8_t SCI_CLOCKF        = 0x3 ;
    const uint8_t SCI_AUDATA        = 0x5 ;
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;
    const uint8_t SCI_AIADDR        = 0xA ;
    const uint8_t SCI_VOL           = 0xB ;
    const uint8_t SCI_AICTRL0       = 0xC ;
    const uint8_t SCI_AICTRL1       = 0xD ;
    const uint8_t SCI_num_registers = 0xF ;
    // SCI_MODE bits
    const uint8_t SM_SDINEW         = 11 ;        // Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;         // Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;         // Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;         // Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;        // Bitnumber in SCI_MODE for Line input
    SPISettings   VS1053_SPI ;                    // SPI settings for this slave
    uint8_t       endFillByte ;                   // Byte to send when stopping song
  protected:
    inline void await_data_request() const
    {
      while ( !digitalRead ( dreq_pin ) )
      {
        NOP() ;                                   // Very short delay
      }
    }

    inline void control_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( dcs_pin, HIGH ) ;            // Bring slave in control mode
      digitalWrite ( cs_pin, LOW ) ;
    }

    inline void control_mode_off() const
    {
      digitalWrite ( cs_pin, HIGH ) ;             // End control mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    inline void data_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( cs_pin, HIGH ) ;             // Bring slave in data mode
      digitalWrite ( dcs_pin, LOW ) ;
    }

    inline void data_mode_off() const
    {
      digitalWrite ( dcs_pin, HIGH ) ;            // End data mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    uint16_t read_register ( uint8_t _reg ) const ;
    void     write_register ( uint8_t _reg, uint16_t _value ) const ;
    void     sdi_send_buffer ( uint8_t* data, size_t len ) ;
    void     sdi_send_fillers ( size_t length ) ;
    void     wram_write ( uint16_t address, uint16_t data ) ;
    uint16_t wram_read ( uint16_t address ) ;

  public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    VS1053 ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin ) ;
    void     begin() ;                                   // Begin operation.  Sets pins correctly,
    // and prepares SPI bus.
    void     startSong() ;                               // Prepare to start playing. Call this each
    // time a new song starts.
    void     playChunk ( uint8_t* data, size_t len ) ;   // Play a chunk of data.  Copies the data to
    // the chip.  Blocks until complete.
    void     stopSong() ;                                // Finish playing a song. Call this after
    // the last playChunk call.
    void     setVolume ( uint8_t vol ) ;                 // Set the player volume.Level from 0-100,
    // higher is louder.
    void     setTone ( uint8_t* rtone ) ;                // Set the player baas/treble, 4 nibbles for
    // treble gain/freq and bass gain/freq
    uint8_t  getVolume() ;                               // Get the current volume setting.
    // higher is louder.
    void     printDetails ( const char *header ) ;       // Print configuration details to serial output.
    void     softReset() ;                               // Do a soft reset
    bool     testComm ( const char *header ) ;           // Test communication with module
    inline bool data_request() const
    {
      return ( digitalRead ( dreq_pin ) == HIGH ) ;
    }
} ;

//******************************************************************************************
// VS1053 class implementation.                                                            *
//******************************************************************************************

VS1053::VS1053 ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin ) :
  cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin)
{
}

uint16_t VS1053::read_register ( uint8_t _reg ) const
{
  uint16_t result ;

  control_mode_on() ;
  SPI.write ( 3 ) ;                                // Read operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = ( SPI.transfer ( 0xFF ) << 8 ) |        // Read 16 bits data
           ( SPI.transfer ( 0xFF ) ) ;
  await_data_request() ;                           // Wait for DREQ to be HIGH again
  control_mode_off() ;
  return result ;
}

void VS1053::write_register ( uint8_t _reg, uint16_t _value ) const
{
  control_mode_on( );
  SPI.write ( 2 ) ;                                // Write operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  SPI.write16 ( _value ) ;                         // Send 16 bits data
  await_data_request() ;
  control_mode_off() ;
}

void VS1053::sdi_send_buffer ( uint8_t* data, size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    SPI.writeBytes ( data, chunk_length ) ;
    data += chunk_length ;
  }
  data_mode_off() ;
}

void VS1053::sdi_send_fillers ( size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    while ( chunk_length-- )
    {
      SPI.write ( endFillByte ) ;
    }
  }
  data_mode_off();
}

void VS1053::wram_write ( uint16_t address, uint16_t data )
{
  write_register ( SCI_WRAMADDR, address ) ;
  write_register ( SCI_WRAM, data ) ;
}

uint16_t VS1053::wram_read ( uint16_t address )
{
  write_register ( SCI_WRAMADDR, address ) ;            // Start reading from WRAM
  return read_register ( SCI_WRAM ) ;                   // Read back result
}

bool VS1053::testComm ( const char *header )
{
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.
  int       i ;                                         // Loop control
  uint16_t  r1, r2, cnt = 0 ;
  uint16_t  delta = 300 ;                               // 3 for fast SPI

  if ( !digitalRead ( dreq_pin ) )
  {
    dbgprint ( "VS1053 not properly installed!" ) ;
    // Allow testing without the VS1053 module
    pinMode ( dreq_pin,  INPUT_PULLUP ) ;               // DREQ is now input with pull-up
    return false ;                                      // Return bad result
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if ( strstr ( header, "Fast" ) )
  {
    delta = 3 ;                                         // Fast SPI, more loops
  }
  dbgprint ( header ) ;                                 // Show a header
  for ( i = 0 ; ( i < 0xFFFF ) && ( cnt < 20 ) ; i += delta )
  {
    write_register ( SCI_VOL, i ) ;                     // Write data to SCI_VOL
    r1 = read_register ( SCI_VOL ) ;                    // Read back for the first time
    r2 = read_register ( SCI_VOL ) ;                    // Read back a second time
    if  ( r1 != r2 || i != r1 || i != r2 )              // Check for 2 equal reads
    {
      dbgprint ( "VS1053 error retry SB:%04X R1:%04X R2:%04X", i, r1, r2 ) ;
      cnt++ ;
      delay ( 10 ) ;
    }
  }
  return ( cnt == 0 ) ;                                 // Return the result
}

void VS1053::begin()
{
  pinMode      ( dreq_pin,  INPUT ) ;                   // DREQ is an input
  pinMode      ( cs_pin,    OUTPUT ) ;                  // The SCI and SDI signals
  pinMode      ( dcs_pin,   OUTPUT ) ;
  digitalWrite ( dcs_pin,   HIGH ) ;                    // Start HIGH for SCI en SDI
  digitalWrite ( cs_pin,    HIGH ) ;
  delay ( 100 ) ;
  // Init SPI in slow mode ( 0.2 MHz )
  VS1053_SPI = SPISettings ( 200000, MSBFIRST, SPI_MODE0 ) ;
  //printDetails ( "Right after reset/startup" ) ;
  delay ( 20 ) ;
  //printDetails ( "20 msec after reset" ) ;
  testComm ( "Slow SPI,Testing VS1053 read/write registers..." ) ;
  // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
  // when playing MP3.  You can modify the board, but there is a more elegant way:
  wram_write ( 0xC017, 3 ) ;                            // GPIO DDR = 3
  wram_write ( 0xC019, 0 ) ;                            // GPIO ODATA = 0
  delay ( 100 ) ;
  //printDetails ( "After test loop" ) ;
  softReset() ;                                         // Do a soft reset
  // Switch on the analog parts
  write_register ( SCI_AUDATA, 44100 + 1 ) ;            // 44.1kHz + stereo
  // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
  write_register ( SCI_CLOCKF, 6 << 12 ) ;              // Normal clock settings multiplyer 3.0 = 12.2 MHz
  //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
  VS1053_SPI = SPISettings ( 4000000, MSBFIRST, SPI_MODE0 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_LINE1 ) ) ;
  testComm ( "Fast SPI, Testing VS1053 read/write registers again..." ) ;
  delay ( 10 ) ;
  await_data_request() ;
  endFillByte = wram_read ( 0x1E06 ) & 0xFF ;
  dbgprint ( "endFillByte is %X", endFillByte ) ;
  //printDetails ( "After last clocksetting" ) ;
  delay ( 100 ) ;
}

void VS1053::setVolume ( uint8_t vol )
{
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value ;                                      // Value to send to SCI_VOL

  if ( vol != curvol )
  {
    curvol = vol ;                                      // Save for later use
    value = map ( vol, 0, 100, 0xF8, 0x00 ) ;           // 0..100% to one channel
    value = ( value << 8 ) | value ;
    write_register ( SCI_VOL, value ) ;                 // Volume left and right
  }
}

void VS1053::setTone ( uint8_t *rtone )                 // Set bass/treble (4 nibbles)
{
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0 ;                                  // Value to send to SCI_BASS
  int      i ;                                          // Loop control

  for ( i = 0 ; i < 4 ; i++ )
  {
    value = ( value << 4 ) | rtone[i] ;                 // Shift next nibble in
  }
  write_register ( SCI_BASS, value ) ;                  // Volume left and right
}

uint8_t VS1053::getVolume()                             // Get the currenet volume setting.
{
  return curvol ;
}

void VS1053::startSong()
{
  sdi_send_fillers ( 10 ) ;
}

void VS1053::playChunk ( uint8_t* data, size_t len )
{
  sdi_send_buffer ( data, len ) ;
}

void VS1053::stopSong()
{
  uint16_t modereg ;                     // Read from mode register
  int      i ;                           // Loop control

  sdi_send_fillers ( 2052 ) ;
  delay ( 10 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_CANCEL ) ) ;
  for ( i = 0 ; i < 200 ; i++ )
  {
    sdi_send_fillers ( 32 ) ;
    modereg = read_register ( SCI_MODE ) ;  // Read status
    if ( ( modereg & _BV ( SM_CANCEL ) ) == 0 )
    {
      sdi_send_fillers ( 2052 ) ;
      dbgprint ( "Song stopped correctly after %d msec", i * 10 ) ;
      return ;
    }
    delay ( 10 ) ;
  }
  printDetails ( "Song stopped incorrectly!" ) ;
}

void VS1053::softReset()
{
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_RESET ) ) ;
  delay ( 10 ) ;
  await_data_request() ;
}

void VS1053::printDetails ( const char *header )
{
  uint16_t     regbuf[16] ;
  uint8_t      i ;

  dbgprint ( header ) ;
  dbgprint ( "REG   Contents" ) ;
  dbgprint ( "---   -----" ) ;
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    regbuf[i] = read_register ( i ) ;
  }
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    delay ( 5 ) ;
    dbgprint ( "%3X - %5X", i, regbuf[i] ) ;
  }
}

// The object for the MP3 player
VS1053 vs1053player (  VS1053_CS, VS1053_DCS, VS1053_DREQ ) ;

//******************************************************************************************
// End VS1053 stuff.                                                                       *
//******************************************************************************************


//******************************************************************************************
// Ringbuffer (fifo) routines.                                                             *
//******************************************************************************************
//******************************************************************************************
//                              R I N G S P A C E                                          *
//******************************************************************************************
uint16_t ringspace()
{
  return ( RINGBFSIZ - rcount ) ;     // Free space available
}


//******************************************************************************************
//                              R I N G A V A I L                                          *
//******************************************************************************************
inline uint16_t ringavail()
{
  return rcount ;                     // Return number of bytes available for getring()
}


//******************************************************************************************
//                                P U T R I N G                                            *
//******************************************************************************************
// Version of putring to write a buffer to the ringbuffer.                                 *
// No check on available space.  See ringspace().                                          *
//******************************************************************************************
void putring ( uint8_t* buf, uint16_t len )
{
  uint16_t partl ;                                                // Partial length to xfer

  if ( len )                                                      // anything to do?
  {
    // First see if we must split the transfer.  We cannot write past the ringbuffer end.
    if ( ( rbwindex + len ) >= RINGBFSIZ  )
    {
      partl = RINGBFSIZ - rbwindex ;                              // Part of length to xfer
      memcpy ( ringbuf + rbwindex, buf, partl ) ;                 // Copy next part
      rbwindex = 0 ;
      rcount += partl ;                                           // Adjust number of bytes
      buf += partl ;                                              // Point to next free byte
      len -= partl ;                                              // Adjust rest length
    }
    if ( len )                                                    // Rest to do?
    {
      memcpy ( ringbuf + rbwindex, buf, len ) ;                   // Copy full or last part
      rbwindex += len ;                                           // Point to next free byte
      rcount += len ;                                             // Adjust number of bytes
    }
  }
}


//******************************************************************************************
//                                G E T R I N G                                            *
//******************************************************************************************
uint8_t getring()
{
  // Assume there is always something in the bufferpace.  See ringavail()
  if ( ++rbrindex == RINGBFSIZ )      // Increment pointer and
  {
    rbrindex = 0 ;                    // wrap at end
  }
  rcount-- ;                          // Count is now one less
  return *(ringbuf + rbrindex) ;      // return the oldest byte
}

//******************************************************************************************
//                               E M P T Y R I N G                                         *
//******************************************************************************************
void emptyring()
{
  rbwindex = 0 ;                      // Reset ringbuffer administration
  rbrindex = RINGBFSIZ - 1 ;
  rcount = 0 ;
}


//******************************************************************************************
//                              N V S O P E N                                              *
//******************************************************************************************
// Open Preferences with my-app namespace. Each application module, library, etc.          *
// has to use namespace name to prevent key name collisions. We will open storage in       *
// RW-mode (second parameter has to be false).                                             *
//******************************************************************************************
void nvsopen()
{
  if ( ! nvshandle )                                         // Opened already?
  {
    nvserr = nvs_open ( NAME, NVS_READWRITE, &nvshandle ) ;  // No, open nvs
    if ( nvserr )
    {
      dbgprint ( "nvs_open failed!" ) ;
    }
  }
}


//******************************************************************************************
//                              N V S C L E A R                                            *
//******************************************************************************************
// Clear all preferences.                                                                  *
//******************************************************************************************
esp_err_t nvsclear()
{
  nvsopen() ;                                         // Be sure to open nvs
  return nvs_erase_all ( nvshandle ) ;                // Clear all keys
}


//******************************************************************************************
//                              N V S G E T S T R                                          *
//******************************************************************************************
// Read a string from nvs.                                                                 *
// A copy of the key is used because nvs_get_str will fail after some time otherwise....   *
//******************************************************************************************
String nvsgetstr ( const char* key )
{
  uint32_t      counter ;
  static char   nvs_buf[NVSBUFSIZE] ;       // Buffer for contents
  size_t        len = NVSBUFSIZE ;          // Max length of the string, later real length

  nvsopen() ;                               // Be sure to open nvs
  nvs_buf[0] = '\0' ;                       // Return empty string on error
  nvserr = nvs_get_str ( nvshandle, key, nvs_buf, &len ) ;
  if ( nvserr )
  {
    dbgprint ( "nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
               nvserr, key, strlen ( key), len ) ;
    dbgprint ( "Contents: %s", nvs_buf ) ;
  }
  return String ( nvs_buf ) ;
}


//******************************************************************************************
//                              N V S S E T S T R                                          *
//******************************************************************************************
// Put a key/value pair in nvs.  Length is limited to allow easy read-back.                *
// No writing if no change.                                                                *
//******************************************************************************************
esp_err_t nvssetstr ( const char* key, String val )
{
  String curcont ;                                         // Current contents
  bool   wflag = true  ;                                   // Assume update or new key

  //dbgprint ( "Setstring for %s: %s", key, val.c_str() ) ;
  if ( val.length() >= NVSBUFSIZE )                        // Limit length of string to store
  {
    dbgprint ( "nvssetstr length failed!" ) ;
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE ;
  }
  if ( nvssearch ( key ) )                                 // Already in nvs?
  {
    curcont = nvsgetstr ( key ) ;                          // Read current value
    wflag = ( curcont != val ) ;                           // Value change?
  }
  if ( wflag )                                             // Update or new?
  {
    //dbgprint ( "nvssetstr update value" ) ;
    nvserr = nvs_set_str ( nvshandle, key, val.c_str() ) ; // Store key and value
    if ( nvserr )                                          // Check error
    {
      dbgprint ( "nvssetstr failed!" ) ;
    }
  }
  return nvserr ;
}


//******************************************************************************************
//                              N V S S E A R C H                                          *
//******************************************************************************************
// Check if key exists in nvs.                                                             *
//******************************************************************************************
bool nvssearch ( const char* key )
{
  size_t        len = NVSBUFSIZE ;                      // Length of the string

  nvsopen() ;                                           // Be sure to open nvs
  nvserr = nvs_get_str ( nvshandle, key, NULL, &len ) ; // Get length of contents
  return ( nvserr == ESP_OK ) ;                         // Return true if found
}


//******************************************************************************************
//                              U T F 8 A S C I I                                          *
//******************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                    *
// Convert a single Character from UTF8 to Extended ASCII.                                 *
// Return "0" if a byte has to be ignored.                                                 *
//******************************************************************************************
byte utf8ascii ( byte ascii )
{
  static const byte lut_C3[] =
  { "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###aaaaaaaceeeeiiiidnooooo##uuuuyyy" } ;
  static byte       c1 ;              // Last character buffer
  byte              res = 0 ;         // Result, default 0

  if ( ascii <= 0x7F )                // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0 ;
    res = ascii ;                     // Return unmodified
  }
  else
  {
    switch ( c1 )                     // Conversion depending on first UTF8-character
    {
      case 0xC2: res = '~' ;
        break ;
      case 0xC3: res = lut_C3[ascii - 128] ;
        break ;
      case 0x82: if ( ascii == 0xAC )
        {
          res = 'E' ;       // Special case Euro-symbol
        }
    }
    c1 = ascii ;                      // Remember actual character
  }
  return res ;                        // Otherwise: return zero, if character has to be ignored
}


//******************************************************************************************
//                              U T F 8 A S C I I                                          *
//******************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                  *
//******************************************************************************************
void utf8ascii ( char* s )
{
  int  i, k = 0 ;                     // Indexes for in en out string
  char c ;

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      s[k++] = c ;                    // Yes, put in output string
    }
  }
  s[k] = 0 ;                          // Take care of delimeter
}


//******************************************************************************************
//                              U T F 8 A S C I I                                          *
//******************************************************************************************
// Conversion UTF8-String to Extended ASCII String.                                        *
//******************************************************************************************
String utf8ascii ( const char* s )
{
  int  i ;                            // Index for input string
  char c ;
  String res = "" ;                   // Result string

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      res += String ( c ) ;           // Yes, put in output string
    }
  }
  return res ;
}


//******************************************************************************************
//                                  D B G P R I N T                                        *
//******************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.*
// Print only if DEBUG flag is true.  Always returns the the formatted string.             *
//******************************************************************************************
char* dbgprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
    Serial.println ( sbuf ) ;                          // and the info
  }
  return sbuf ;                                        // Return stored string
}


//******************************************************************************************
//                          S E L E C T N E X T S D F I L E                                *
//******************************************************************************************
// Select the next mp3 file from SD.  If the last selected song was random, the next song  *
// is a random one.  Otherwise the next node is choosen.                                   *
// If nodeID is "0" choose a random nodeID.                                                *
//******************************************************************************************
void selectnextSDfile()
{
  uint16_t        inx, inx2 ;                           // Position in nodelist

  if ( hostreq )                                        // Host request already set?
  {
    return ;                                            // Yes, no action
  }
  if ( SD_currentnode == "0" )                          // Random playing?
  {
    host = getSDfilename ( SD_currentnode ) ;           // Yes, select next random nodeID
  }
  else
  {
    inx = SD_nodelist.indexOf ( SD_currentnode ) +     // Get position of next nodeID in list
          SD_currentnode.length() + 1 ;
    if ( inx >= SD_nodelist.length() )                 // End of list?
    {
      inx = 0 ;                                        // Yes, wrap around
    }
    inx2 = SD_nodelist.indexOf ( "\n", inx ) ;         // Find end of node ID
    host = getSDfilename ( SD_nodelist.substring ( inx, inx2 ) ) ;
  }
  hostreq = true ;                                     // Request new song 
}


//******************************************************************************************
//                              G E T S D F I L E N A M E                                  *
//******************************************************************************************
// Translate the NnodeID of a track to the full filename that can be used as a station.    *
// If nodeID is "0" choose a random nodeID.                                                *
//******************************************************************************************
String getSDfilename ( String nodeID )
{
  String          res ;                                 // Function result
  File            root, file ;                          // Handle to root and directory entry
  uint16_t        n, i ;                                // Current sequence number and counter in directory
  uint16_t        inx ;                                 // Position in nodeID
  const char*     p = "/" ;                             // Points to directory/file
  uint16_t        rndnum ;                              // Random index in SD_nodelist
  int             nodeinx = 0 ;                         // Points to node ID in SD_nodecount
  int             nodeinx2 ;                            // Points to end of node ID in SD_nodecount

  SD_currentnode = nodeID ;                             // Save current node
  if ( nodeID == "0" )                                  // Empty parameter?
  {
    rndnum = random ( SD_nodecount ) ;                  // Yes, choose a random node
    for ( i = 0 ; i < rndnum ; i++ )                    // Find the node ID
    {
      // Search to begin of the random node by skipping lines
      nodeinx = SD_nodelist.indexOf ( "\n", nodeinx ) + 1 ;
    }
    nodeinx2 = SD_nodelist.indexOf ( "\n", nodeinx ) ;     // Find end of node ID
    nodeID = SD_nodelist.substring ( nodeinx, nodeinx2 ) ; // Get node ID
  }
  dbgprint ( "getSDfilename requested node ID is %s",   // Show requeste node ID
             nodeID.c_str() ) ;
  while ( ( n = nodeID.toInt() ) )                      // Next sequence in current level
  {
    inx = nodeID.indexOf ( "," ) ;                      // Find position of comma
    if ( inx >= 0 )
    {
      nodeID = nodeID.substring ( inx + 1 ) ;           // Remove sequence in this level from nodeID
    }
    root = SD.open ( p ) ;                              // Open the directory (this level)
    for ( i = 1 ; i <=  n ; i++ )
    {
      file = root.openNextFile() ;                      // Get next directory entry
      //dbgprint ( "file nr %d/%d is %s", i, n, file.name() ) ;
    }
    p = file.name() ;                                   // Points to directory- or file name
  }
  res = String ( "localhost" ) + String ( p ) ;         // Format result
  return res ;                                          // Return full station spec
}


//******************************************************************************************
//                              L I S T S D T R A C K S                                    *
//******************************************************************************************
// Search all MP3 files on directory of SD card.  Return the number of files found.        *
// A "node" of max. 4 levels ( the subdirectory level) will be generated for every file.   *
// The numbers within the node-array is the sequence number of the file/directory in that  *
// subdirectory.                                                                           *
// A node ID is a string like "2,1,4,0", which means the 4th file in the first directory   *
// of the second directory.                                                                *
// The list will be send to the webinterface if parameter "send"is true.                   *
//******************************************************************************************
int listsdtracks ( const char * dirname, int level = 0, bool send = true )
{
  const  uint16_t SD_MAXDEPTH = 4 ;                     // Maximum depts.  Note: see mp3play_html.
  static uint16_t fcount, oldfcount ;                   // Total number of files
  static uint16_t SD_node[SD_MAXDEPTH+1] ;              // Node ISs, max levels deep
  static String   SD_outbuf ;                           // Output buffer for cmdclient
  uint16_t        ldirname ;                            // Length of dirname to remove from filename
  File            root, file ;                          // Handle to root and directory entry
  String          filename ;                            // Copy of filename for lowercase test
  uint16_t        i ;                                   // Loop control to compute single node id
  String          tmpstr ;                              // Tijdelijke opslag node ID

  if ( strcmp ( dirname, "/" ) == 0 )                   // Are we at the root directory?
  {
    fcount = 0 ;                                        // Yes, reset count
    memset ( SD_node, 0, sizeof(SD_node) ) ;            // And sequence counters
    SD_outbuf = String() ;                              // And output buffer
    SD_nodelist = String() ;                            // And nodelist
    if ( !SDokay )                                      // See if known card
    {
      if ( send )
      {
        cmdclient.println ( "0/No tracks found" ) ;     // No SD card, emppty list
      }
      return 0 ;
    }
  }
  oldfcount = fcount ;                                  // To see if files found in this directory
  //dbgprint ( "SD directory is %s", dirname ) ;        // Show current directory
  ldirname = strlen ( dirname ) ;                       // Length of dirname to remove from filename
  root = SD.open ( dirname ) ;                          // Open the current directory level
  if ( !root || !root.isDirectory() )                   // Success?
  {
    dbgprint ( "%s is not a directory or not root",     // No, print debug message
               dirname ) ;
    return fcount ;                                     // and return
  }
  while ( ( file = root.openNextFile() ) )
  {
    SD_node[level]++ ;                                  // Set entry sequence of current level
    if ( file.name()[0] == '.' )                        // Skip hidden directories
    {
      continue ;
    }
    if ( file.isDirectory() )                           // Is it a directory?
    {
      if ( level < SD_MAXDEPTH )                        // Yes, dig deeper
      {
        listsdtracks ( file.name(), level + 1, send ) ; // Note: called recursively
        SD_node[level + 1] = 0 ;                        // Forget counter for one level up
      }
    }
    else
    {
      filename = String ( file.name() ) ;               // Copy filename
      filename.toLowerCase() ;                          // Force lowercase
      if ( filename.endsWith ( ".mp3" ) )               // It is a file, but is it an MP3?
      {
        fcount++ ;                                      // Yes, count total number of MP3 files
        tmpstr = String() ;                             // Empty
        for ( i = 0 ; i < SD_MAXDEPTH ; i++ )           // Add a line containing the node to SD_outbuf
        {
          if ( i )                                      // Need to add separating comma?
          {
            tmpstr += String ( "," ) ;                  // Yes, add comma
          }
          tmpstr += String ( SD_node[i] ) ;             // Add sequence number
        }
        if ( send )                                     // Need to add to string for webinterface?
        {
          SD_outbuf += tmpstr +                         // Form line for mp3play_html page
                       utf8ascii ( file.name() +        // Filename starts after directoryname
                                   ldirname ) +
                     String ( "\n" ) ;
        }
        SD_nodelist += tmpstr + String ( "\n" ) ;       // Add to nodelist
        //dbgprint ( "Track: %s",                       // Show debug info
        //           file.name() + ldirname ) ;
        if ( SD_outbuf.length() > 1000 )                // Buffer full?
        {
          cmdclient.print ( SD_outbuf ) ;               // Yes, send it
          SD_outbuf = String() ;                        // Clear buffer
        }
      }
    }
  }
  if ( fcount != oldfcount )                            // Files in this directory?
  {
    SD_outbuf += String ( "-1/ \n" ) ;                  // Spacing in list
  }
  if ( SD_outbuf.length() )                             // Flush buffer if not empty
  {
    cmdclient.print ( SD_outbuf ) ;                     // Filled, send it
    SD_outbuf = String() ;                              // Continue with empty buffer
  }
  return fcount ;                                       // Return number of MP3s (sofar)
}


//******************************************************************************************
//                             G E T E N C R Y P T I O N T Y P E                           *
//******************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                     *
//******************************************************************************************
const char* getEncryptionType ( wifi_auth_mode_t thisType )
{
  switch (thisType)
  {
    case WIFI_AUTH_OPEN:
      return "OPEN" ;
    case WIFI_AUTH_WEP:
      return "WEP" ;
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK" ;
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK" ;
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK" ;
    case WIFI_AUTH_MAX:
      return "MAX" ;
  }
  return "????" ;
}


//******************************************************************************************
//                                L I S T N E T W O R K S                                  *
//******************************************************************************************
// List the available networks and select the strongest.                                   *
// Acceptable networks are those who have a "SSID.pw" file in the SPIFFS.                  *
// SSIDs of available networks will be saved for use in webinterface.                      *
//******************************************************************************************
void listNetworks()
{
  wifi_auth_mode_t encryption ;       // TKIP(WPA), WEP, etc.
  const char*      acceptable ;       // Netwerk is acceptable for connection
  int              i ;                // Loop control
  String           sassid ;           // Search string in anetworks

  dbgprint ( "Scan Networks" ) ;      // Scan for nearby networks
  int numSsid = WiFi.scanNetworks() ;
  dbgprint ( "Scan completed" ) ;
  if ( numSsid <= 0 )
  {
    dbgprint ( "Couldn't get a wifi connection" ) ;
    return ;
  }
  // print the list of networks seen:
  dbgprint ( "Number of available networks: %d",
             numSsid ) ;
  // Print the network number and name for each network found and
  for ( i = 0 ; i < numSsid ; i++ )
  {
    acceptable = "" ;                                    // Assume not acceptable
    sassid = WiFi.SSID ( i ) + String ( "|" ) ;          // For search string
    if ( anetworks.indexOf ( sassid ) >= 0 )             // Is this SSID acceptable?
    {
      acceptable = "Acceptable" ;
    }
    encryption = WiFi.encryptionType ( i ) ;
    dbgprint ( "%2d - %-25s Signal: %3d dBm, Encryption %4s, %s",
               i + 1, WiFi.SSID ( i ).c_str(), WiFi.RSSI ( i ),
               getEncryptionType ( encryption ),
               acceptable ) ;
    // Remember this network for later use
    networks += WiFi.SSID ( i ) + String ( "|" ) ;
  }
  dbgprint ( "End of list" ) ;
}


//******************************************************************************************
//                                  T I M E R 1 0 S E C                                    *
//******************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                               *
// If totalcount has not been changed, there is a problem and playing will stop.           *
// Note that calling timely procedures within this routine or in called functions will     *
// cause a crash!                                                                          *
//******************************************************************************************
void IRAM_ATTR timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed foor change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    if ( totalcount == oldtotalcount )            // Still playing?
    {
      if ( morethanonce > 10 )                    // Happened too many times?
      {
        ESP.restart() ;                           // Reset the CPU, probably no return
      }
      if ( datamode & ( PLAYLISTDATA |            // In playlist mode?
                        PLAYLISTINIT |
                        PLAYLISTHEADER ) )
      {
        playlist_num = 0 ;                        // Yes, end of playlist
      }
      if ( ( morethanonce > 0 ) ||                // Happened more than once?
           ( playlist_num > 0 ) )                 // Or playlist active?
      {
        datamode = STOPREQD ;                     // Stop player
        ini_block.newpreset++ ;                   // Yes, try next channel
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      if ( morethanonce )                         // Recovered from data loss?
      {
        morethanonce = 0 ;                        // Data see, reset failcounter
      }
      oldtotalcount = totalcount ;                // Save for comparison in next cycle
    }
  }
}


//******************************************************************************************
//                                  T I M E R 1 0 0                                        *
//******************************************************************************************
// Called every 100 msec on interrupt level, so must be in IRAM and no lengthy operations  *
// allowed.                                                                                *
//******************************************************************************************
void IRAM_ATTR timer100()
{
  static int     count10sec = 0 ;                 // Counter for activatie 10 seconds process

  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
}


//******************************************************************************************
//                                  I S R _ I R                                            *
//******************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                         *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer. *
// Input is complete after 65 level changes.                                               *
// Only the last 32 level changes are significant and will be handed over to common data.  *
//******************************************************************************************
void IRAM_ATTR isr_IR()
{
  static uint32_t t0 = 0 ;                           // To get the interval
  uint32_t        t1, intval ;                       // Current time and interval since last change
  static uint32_t ir_locvalue = 0 ;                  // IR code
  static int      ir_loccount ;                      // Length of code
  int             i ;                                // Loop control
  uint32_t        mask_in = 2 ;                      // Mask input for conversion
  uint16_t        mask_out = 1 ;                     // Mask output for conversion

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare
  if ( ( intval > 400 ) && ( intval < 700 ) )        // Short pulse?
  {
    ir_locvalue = ir_locvalue << 1 ;                 // Shift in a "zero" bit
    ir_loccount++ ;                                  // Count number of received bits
  }
  else if ( ( intval > 1600 ) && ( intval < 1700 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
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


//******************************************************************************************
//                              D I S P L A Y V O L U M E                                  *
//******************************************************************************************
// Show the current volume as an indicator on the screen.                                  *
//******************************************************************************************
void displayvolume()
{
#if defined ( USETFT )
  static uint8_t oldvol = 0 ;                        // Previous volume
  uint8_t        pos ;                               // Positon of volume indicator

  if ( vs1053player.getVolume() != oldvol )
  {
    pos = map ( vs1053player.getVolume(), 0, 100, 0, 160 ) ;
  }
  tft.fillRect ( 0, 126, pos, 2, RED ) ;             // Paint red part
  tft.fillRect ( pos, 126, 160 - pos, 2, GREEN ) ;   // Paint green part
#endif
}


//******************************************************************************************
//                              D I S P L A Y I N F O                                      *
//******************************************************************************************
// Show a string on the LCD at a specified y-position in a specified color                 *
//******************************************************************************************
void displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color )
{
#if defined ( USETFT )
  char buf [ strlen ( str ) + 1 ] ;             // Need some buffer space

  strcpy ( buf, str ) ;                         // Make a local copy of the string
  utf8ascii ( buf ) ;                           // Convert possible UTF8
  tft.fillRect ( 0, pos, 160, height, BLACK ) ; // Clear the space for new info
  tft.setTextColor ( color ) ;                  // Set the requested color
  tft.setCursor ( 0, pos ) ;                    // Prepare to show the info
  tft.println ( buf ) ;                         // Show the string
#endif
}


//******************************************************************************************
//                        S H O W S T R E A M T I T L E                                    *
//******************************************************************************************
// Show artist and songtitle if present in metadata.                                       *
// Show always if full=true.                                                               *
//******************************************************************************************
void showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata

  if ( strstr ( ml, "StreamTitle=" ) )
  {
    dbgprint ( "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    dbgprint ( ml ) ;
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
    return ;                                    // Do not show
  }
  // Save for status request from browser and for MQTT
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    *p1++ = '\n' ;                              // Found: replace 3 characters by newline
    p2 = p1 + 2 ;
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  displayinfo ( streamtitle, 20, 40, CYAN ) ;   // Show title at position 20
}


//******************************************************************************************
//                            S T O P _ M P 3 C L I E N T                                  *
//******************************************************************************************
// Disconnect from the server.                                                             *
//******************************************************************************************
void stop_mp3client ()
{
  while ( mp3client.connected() )
  {
    dbgprint ( "Stopping client" ) ;               // Stop connection to host
    mp3client.flush() ;
    mp3client.stop() ;
    delay ( 500 ) ;
  }
  mp3client.flush() ;                              // Flush stream client
  mp3client.stop() ;                               // Stop stream client
}


//******************************************************************************************
//                            C O N N E C T T O H O S T                                    *
//******************************************************************************************
// Connect to the Internet radio server specified by newpreset.                            *
//******************************************************************************************
bool connecttohost()
{
  int         inx ;                                 // Position of ":" in hostname
  char*       pfs ;                                 // Pointer to formatted string
  int         port = 80 ;                           // Port number for host
  String      extension = "/" ;                     // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext ;                           // Host without extension and portnumber

  stop_mp3client() ;                                // Disconnect if still connected
  dbgprint ( "Connect to new host %s", host.c_str() ) ;
  displayinfo ( "   ** Internet radio **", 0, 20, WHITE ) ;
  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  if ( host.endsWith ( ".m3u" ) )                   // Is it an m3u playlist?
  {
    playlist = host ;                               // Save copy of playlist URL
    datamode = PLAYLISTINIT ;                       // Yes, start in PLAYLIST mode
    if ( playlist_num == 0 )                        // First entry to play?
    {
      playlist_num = 1 ;                            // Yes, set index
    }
    dbgprint ( "Playlist request, entry %d", playlist_num ) ;
  }
  // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
  inx = host.indexOf ( "/" ) ;                      // Search for begin of extension
  if ( inx > 0 )                                    // Is there an extension?
  {
    extension = host.substring ( inx ) ;            // Yes, change the default
    hostwoext = host.substring ( 0, inx ) ;         // Host without extension
  }
  // In the URL there may be a portnumber
  inx = host.indexOf ( ":" ) ;                      // Search for separator
  if ( inx >= 0 )                                   // Portnumber available?
  {
    port = host.substring ( inx + 1 ).toInt() ;     // Get portnumber as integer
    hostwoext = host.substring ( 0, inx ) ;         // Host without portnumber
  }
  pfs = dbgprint ( "Connect to %s on port %d, extension %s",
                   hostwoext.c_str(), port, extension.c_str() ) ;
  displayinfo ( pfs, 60, 66, YELLOW ) ;             // Show info at position 60..125
  if ( mp3client.connect ( hostwoext.c_str(), port ) )
  {
    dbgprint ( "Connected to server" ) ;
    // This will send the request to the server. Request metadata.
    mp3client.print ( String ( "GET " ) +
                      extension +
                      String ( " HTTP/1.1\r\n" ) +
                      String ( "Host: " ) +
                      hostwoext +
                      String ( "\r\n" ) +
                      String ( "Icy-MetaData:1\r\n" ) +
                      String ( "Connection: close\r\n\r\n" ) ) ;
    return true ;
  }
  dbgprint ( "Request %s failed!", host.c_str() ) ;
  return false ;
}


//******************************************************************************************
//                               C O N N E C T T O F I L E                                 *
//******************************************************************************************
// Open the local mp3-file.                                                                *
// Not yet implemented.                                                                    *
//******************************************************************************************
bool connecttofile()
{
  String path ;                                           // Full file spec
  char*  p ;                                              // Pointer to filename

  displayinfo ( "   **** MP3 Player ****", 0, 20, WHITE ) ;
  path = host.substring ( 9 ) ;                           // Path, skip the "localhost" part
  mp3file = SD.open ( path ) ;                            // Open the file
  if ( !mp3file )
  {
    dbgprint ( "Error opening file %s", path.c_str() ) ;  // No luck
    return false ;
  }
  p = (char*)path.c_str() + 1 ;                           // Point to filename
  showstreamtitle ( p, true ) ;                           // Show the filename as title
  mqttpub.trigger ( MQTT_STREAMTITLE ) ;                  // Request publishing to MQTT
  displayinfo ( "Playing from local file",
                60, 68, YELLOW ) ;                        // Show Source at position 60
  icyname = "" ;                                          // No icy name yet
  chunked = false ;                                       // File not chunked
  return true ;
}


//******************************************************************************************
//                               C O N N E C T W I F I                                     *
//******************************************************************************************
// Connect to WiFi using the SSID's available in wifiMulti.                                *
// If only one AP if found in preferences (i.e. wifi_00) the connection is made without    *
// using wifiMulti.                                                                        *
// If connection fails, an AP is created and the function returns false.                   *
//******************************************************************************************
bool connectwifi()
{
  char*  pfs ;                                          // Pointer to formatted string
  bool   localAP = false ;                              // True if only local AP is left

  WiFi.disconnect() ;                                   // After restart the router could
  WiFi.softAPdisconnect(true) ;                         // still keep the old connection
  pfs = "IP = 192.168.4.1" ;                            // Default IP address (no AP found)
  if ( num_an )                                         // Any AP defined?
  {
    if ( num_an == 1 )                                  // Just one AP defined in preferences?
    {
      WiFi.begin ( lssid.c_str(),
                   lpw.c_str() ) ;                      // Connect to single SSID found in wifi_xx
      dbgprint ( "Try WiFi %s", lssid.c_str() ) ;       // Message to show during WiFi connect
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
    dbgprint ( "WiFi Failed!  Trying to setup AP with name %s and password %s.", NAME, NAME ) ;
    WiFi.softAP ( NAME, NAME ) ;                        // This ESP will be an AP
    pfs = dbgprint ( "IP = 192.168.4.1" ) ;             // Address for AP
    delay ( 5000 ) ;
  }
  else
  {
    ipaddress = WiFi.localIP().toString() ;             // Form IP address
    dbgprint ( "Connected to %s", WiFi.SSID().c_str() ) ;
    pfs = dbgprint ( "IP = %s", ipaddress.c_str() ) ;   // String to dispay on TFT
  }
#if defined ( USETFT )
  displayinfo ( pfs, 60, 68, YELLOW ) ;                 // Show info at position 60
#endif
  return ( localAP == false ) ;                         // Return result of connection
}


//******************************************************************************************
//                                   O T A S T A R T                                       *
//******************************************************************************************
// Update via WiFi has been started by Arduino IDE.                                        *
//******************************************************************************************
void otastart()
{
  dbgprint ( "OTA Started" ) ;
}


//******************************************************************************************
//                          R E A D H O S T F R O M P R E F                                *
//******************************************************************************************
// Read the mp3 host from the preferences specified by the parameter.                      *
// The host will be returned.                                                              *
//******************************************************************************************
String readhostfrompref ( int8_t preset )
{
  char           tkey[12] ;                            // Key as an array of chars

  sprintf ( tkey, "preset_%02d", preset ) ;            // Form the search key
  if ( nvssearch ( tkey ) )                            // Does it exists?
  {
    // Get the contents
    return nvsgetstr ( tkey ) ;                        // Get the station (or empty sring)
  }
  else
  {
    return String ( "" ) ;                             // Not found
  }
}


//******************************************************************************************
//                          R E A D H O S T F R O M P R E F                                *
//******************************************************************************************
// Search for the next mp3 host in preferences specified newpreset.                        *
// The host will be returned.  newpreset will be updated                                   *
//******************************************************************************************
String readhostfrompref()
{
  String contents = "" ;                                // Result of search
  int    maxtry ;                                       // Limit number of tries

  while ( ( contents = readhostfrompref ( ini_block.newpreset ) ) == "" )
  {
    if ( ++ maxtry > 99 )
    {
      return "" ;
    }
    if ( ++ini_block.newpreset > 99 )                   // Next or wrap to 0
    {
      ini_block.newpreset = 0 ;
    }
  }
  // Get the contents
  return contents ;                                     // Return the station
}


//******************************************************************************************
//                               R E A D P R O G B U T T O N S                             *
//******************************************************************************************
// Read the preferences for the programmable input pins.                                   *
//******************************************************************************************
String readprogbuttons()
{
  char        mykey[20] ;                                   // For numerated key
  int8_t      pinnr ;                                       // GPIO pinnumber to fill
  int         i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry

  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan for all programmable pins
  {
    sprintf ( mykey, "gpio_%02d", pinnr ) ;                 // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        progpin[i].avail = true ;                           // This one is active now
        progpin[i].command = val ;                          // Set command
        dbgprint ( "GPIO%d will execute %s",                // Show result
                   pinnr, val.c_str() ) ;
      }
    }
  }
}


//******************************************************************************************
//                               R E A D P R E F                                           *
//******************************************************************************************
// Read the preferences and interpret the commands.                                        *
// If output == true, the key / value pairs are returned to the caller as a String.        *
// preset_xx and wifi_xx are included.                                                     *
//******************************************************************************************
String readprefs ( bool output )
{
  char* keys[] = { "mqttbroker", "mqttport", "mqttprefix",  // List of all defined keys
                   "mqttuser",   "mqttpasswd",
                   "#",                                     // Spacing in output
                   "wifi_xx",
                   "#",                                     // Spacing in output
                   "volume",
                   "toneha", "tonehf",
                   "tonela", "tonelf",
                   "#",                                     // Spacing in output
                   "preset",
                   "#",                                     // Spacing in output
                   "preset_xx",
                   "#",                                     // Spacing in output
                   "gpio_xx",
                   "#",                                     // Spacing in output
                   "ir_xx",
                   NULL                                     // Einde keys
                 } ;
  char        mykey[20] ;                                   // For numerated keys
  char*       p ;                                           // Points to sequencenumber of numerated key
  int         i, j ;                                        // Loop control
  int         jmax ;                                        // Max numerated key
  char*       numformat ;                                   // "_%02d" or "_%04X"
  String      val ;                                         // Contents of preference entry
  String      cmd ;                                         // Command for analyzCmd
  String      outstr = "" ;                                 // Outputstring
  int         count = 0 ;                                   // Number of keys found
  size_t      len ;

  for ( i = 0 ; keys[i] ; i++ )                             // Loop trough all possible keys
  {
    if ( strcmp ( keys[i], "#" ) == 0 )                     // Key equals "#"?
    {
      if ( output )                                         // Output requested?
      {
        outstr += "#\n" ;                                   // Yes, add commemt line
      }
      continue ;                                            // Skip further key handling
    }
    p = strstr ( keys[i], "_xx"  ) ;                        // See if numerated key
    if ( ( p != NULL ) && output )                          // Numerated key and output requested ?
    {
      strcpy ( mykey, keys[i] ) ;                           // Copy key
      p = strstr ( mykey, "_xx"  ) ;                        // Get position of "_" in numerated key
      jmax = 100 ;                                          // Normally 100
      numformat = "_%02d" ;                                 // Format for numerated keys
      if ( strstr ( mykey, "ir_xx" ) )                      // Longer range for ir_xx
      {
        jmax = 0x10000 ;                                    // > 64000 possibilities
        numformat = "_%04X" ;                               // Different numeration
      }
      for ( j = 0 ; j < jmax ; j++ )                        // Try for 00 to jmax
      {
        sprintf ( p, numformat, j ) ;                       // Form key in preferences
        if ( nvssearch ( mykey ) )                          // Does it exist?
        {
          val = nvsgetstr ( mykey ) ;                       // Get the contents
          if ( val.length() )                               // Does it exists?
          {
            count++ ;                                       // Count number of keys
            outstr += String ( mykey ) +                    // Yes, form outputstring
                      " = " +
                      String ( val ) +
                      "\n" ;
          }
        }
      }
    }
    else                                                    // Key exists?
    {
      if ( nvssearch ( keys[i] ) )                          // Does it exist?
      {
        val = nvsgetstr ( keys[i] ) ;                       // Read value of next key
        if ( val.length() )                                 // parameter in preference?
        {
          count++ ;                                         // Yes, count number of filled keys
        }
        cmd = String ( keys[i] ) +                          // Yes, form command
              " = " +
              String ( val ) ;
        if ( output )
        {
          outstr += cmd.c_str() ;                          // Add to outstr
          outstr += String ( "\n" ) ;                      // Add newline
        }
        else
        {
          if ( val.length() )                               // parameter in preference?
          {
            analyzeCmd ( cmd.c_str() ) ;                    // Analyze it
          }
        }
      }
    }
  }
  if ( count == 0 )
  {
    outstr = String ( "No preferences found.\n"
                      "Use defaults or run Esp32_radio_init first.\n" ) ;
  }
  return outstr ;
}


//******************************************************************************************
//                            M Q T T R E C O N N E C T                                    *
//******************************************************************************************
// Reconnect to broker.                                                                    *
//******************************************************************************************
bool mqttreconnect()
{
  static uint32_t retrytime = 0 ;                         // Limit reconnect interval
  static uint16_t retrycount = 0 ;                        // Give up after number of tries
  bool            res = false ;                           // Connect result
  char            clientid[20] ;                          // Client ID
  char            subtopic[20] ;                          // Topic to subscribe

  if ( ( millis() - retrytime ) < 5000 )                  // Don't try to frequently
  {
    return res ;
  }
  retrytime = millis() ;                                  // Set time of last try
  if ( retrycount > 50 )                                  // Tried too much?
  {
    mqtt_on = false ;                                     // Yes, switch off forever
    return res ;                                          // and quit
  }
  retrycount++ ;                                          // Count the retries
  dbgprint ( "(Re)connecting number %d to MQTT %s",       // Show some debug info
             retrycount,
             ini_block.mqttbroker.c_str() ) ;
  sprintf ( clientid, "%s-%04d",                          // Generate client ID
            NAME, random ( 10000 ) ) ;
  res = mqttclient.connect ( clientid,                    // Connect to broker
                             ini_block.mqttuser.c_str(),
                             ini_block.mqttpasswd.c_str()
                           ) ;
  if ( res )
  {
    sprintf ( subtopic, "%s/%s",                          // Add prefix to subtopic
              ini_block.mqttprefix.c_str(),
              MQTT_SUBTOPIC ) ;
    mqttclient.subscribe ( NAME "/" MQTT_SUBTOPIC ) ;     // Subscribe to MQTT
    mqttpub.trigger ( MQTT_IP ) ;                         // Publish own IP
  }
  else
  {
    dbgprint ( "MQTT connection failed, rc=%d",
               mqttclient.state() ) ;

  }
  return res ;
}


//******************************************************************************************
//                            O N M Q T T M E S S A G E                                    *
//******************************************************************************************
// Executed when a subscribed message is received.                                         *
// Note that message is not delimited by a '\0'.                                           *
// Note that cmd buffer is shared with serial input.                                       *
//******************************************************************************************
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
    dbgprint ( "MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd ) ;
    reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
    dbgprint ( reply ) ;                              // Result for debugging
  }
}


//******************************************************************************************
//                             S C A N S E R I A L                                         *
//******************************************************************************************
// Listen to commands on the Serial inputline.                                             *
//******************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  const char*   reply ;                          // Reply string froma analyzeCmd
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
        dbgprint ( reply ) ;                     // Result for debugging
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


//******************************************************************************************
//                             S C A N D I G I T A L                                       *
//******************************************************************************************
// Scan digital inputs.                                                                    *
//******************************************************************************************
void  scandigital()
{
  static uint32_t oldmillis = 5000 ;                        // To compare with current time
  int             i ;                                       // Loop control
  int8_t          pinnr ;                                   // Pin number to check
  bool            level ;                                   // Input level
  const char*     reply ;                                   // Result of analyzeCmd

  if ( millis() - oldmillis < 100 )                         // Debounce
  {
    return ;
  }
  oldmillis = millis() ;                                    // 100 msec over
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan all inputs
  {
    if ( !progpin[i].avail )                                // Skip unused pins
    {
      continue ;
    }
    level = ( digitalRead ( pinnr ) == HIGH ) ;             // Sample the pin
    if ( level != progpin[i].cur )                          // Change seen?
    {
      progpin[i].changed = true ;                           // Yes, register a change
      progpin[i].cur = level ;                              // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "GPIO%d is now LOW, execute %s",
                   pinnr, progpin[i].command.c_str() ) ;
        reply = analyzeCmd ( progpin[i].command.c_str() ) ; // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
}


//******************************************************************************************
//                             S C A N I R                                                 *
//******************************************************************************************
// See if IR input is available.  Execute the programmed command.                          *
//******************************************************************************************
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
      dbgprint ( "IR code %04X received. Will execute %s",
                 ir_value, val.c_str() ) ;
      reply = analyzeCmd ( val.c_str() ) ;                  // Analyze command and handle it
      dbgprint ( reply ) ;                                  // Result for debugging
    }
    else
    {
      dbgprint ( "IR code %04X received, but not found in preferences!",
                 ir_value ) ;
    }
    ir_value = 0 ;                                          // Reset IR code received
  }
}


//******************************************************************************************
//                                   M K _ L S A N                                         *
//******************************************************************************************
// Make al list of acceptable networks in preferences.                                     *
// The result will be stored in anetworks like "|SSID1|SSID2|......|SSIDn|".               *
// The number of acceptable networks will be stored in num_an.                             *
// Not that the last pound SSID and password are kept in common data.  If only one SSID is *
// defined, the connect is made without using wifiMulti.  In this case a connection will   *
// be made even if de SSID is hidden.                                                      *
//******************************************************************************************
void  mk_lsan()
{
  int         i ;                                        // Loop control
  char        key[10] ;                                  // For example: "wifi_03"
  String      buf ;                                      // "SSID/password"
  int         inx ;                                      // Place of "/"

  num_an = 0 ;                                           // Count acceptable networks
  anetworks = "|" ;                                      // Initial value

  for ( i = 0 ; i < 100 ; i++ )                          // Examine wifi_00 .. wifi_99
  {
    sprintf ( key, "wifi_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( key  ) )                            // Does it exists?
    {
      buf = nvsgetstr ( key ) ;                          // Get the contents
      inx = buf.indexOf ( "/" ) ;                        // Find separator between ssid and password
      if ( inx > 0 )                                     // Separator found?
      {
        lpw = buf.substring ( inx + 1 ) ;                // Isolate password
        lssid = buf.substring ( 0, inx ) ;               // Holds SSID now
        dbgprint ( "Added SSID %s to list of networks",
                   lssid.c_str() ) ;
        anetworks += lssid ;                             // Add to list
        anetworks += "|" ;                               // Separator
        num_an++ ;                                       // Count number of acceptable networks
        wifiMulti.addAP ( lssid.c_str(), lpw.c_str() ) ; // Add to wifi acceptable network list
      }
    }
  }
}


//******************************************************************************************
//                             G E T P R E S E T S                                         *
//******************************************************************************************
// Make a list of all preset stations.                                                     *
// The result will be stored in the String presetlist (global data).                       *
//******************************************************************************************
void getpresets()
{
  String              val ;                              // Result of readhostfrompref
  int                 inx ;                              // Position of search char in line
  int                 i ;                                // Loop control, preset number
  char                vnr[3] ;                           // 2 digit presetnumber as string

  presetlist = String ( "" ) ;                           // No result yet
  for ( i = 0 ; i < 100 ; i++ )                          // Max 99 presets
  {
    val = readhostfrompref ( i ) ;                       // Get next preset
    if ( val.length() )                                  // Does it exists?
    {
      // Show just comment if available.  Otherwise the preset itself.
      inx = val.indexOf ( "#" ) ;                        // Get position of "#"
      if ( inx > 0 )                                     // Hash sign present?
      {
        val.remove ( 0, inx + 1 ) ;                      // Yes, remove non-comment part
      }
      chomp ( val ) ;                                    // Remove garbage from description
      sprintf ( vnr, "%02d", i ) ;                       // Preset number
      presetlist += ( String ( vnr ) + val +             // 2 digits plus description plus separator
                      String ( "|" ) ) ;
    }
  }
}


//******************************************************************************************
//                                   S E T U P                                             *
//******************************************************************************************
// Setup for the program.                                                                  *
//******************************************************************************************
void setup()
{
  int      itrpt ;                                       // Interrupt number for DREQ
  int      i ;                                           // Loop control
  int      pinnr ;                                       // Input pinnumber
  char*    p ;
  byte     mac[6] ;                                      // WiFi mac address
  char     tmpstr[20] ;                                  // For version and Mac address
  char*    wvn = "Include file %s_html has the wrong version number!  Replace header file." ;

  Serial.begin ( 115200 ) ;                              // For debug
  Serial.println() ;
  // Version tests for some vital include files
  if ( about_html_version < 170626 )   dbgprint ( wvn, "about" ) ;
  if ( config_html_version < 170626 )  dbgprint ( wvn, "config" ) ;
  if ( index_html_version < 170626 )   dbgprint ( wvn, "index" ) ;
  if ( mp3play_html_version < 170626 ) dbgprint ( wvn, "mp3play" ) ;
  // Print some memory and sketch info
  dbgprint ( "Starting ESP32-radio Version %s...  Free memory %d",
             VERSION,
             ESP.getFreeHeap() ) ;
#if defined ( SDCARDCS )
  pinMode ( SDCARDCS, OUTPUT ) ;                         // Deselect SDCARD
  digitalWrite ( SDCARDCS, HIGH ) ;
#endif
  pinMode ( IR_PIN, INPUT ) ;                            // Pin for IR receiver VS1838B
  attachInterrupt ( IR_PIN, isr_IR, CHANGE ) ;           // Interrupts will be handle by isr_IR
#if defined ( USETFT )
  tft.begin() ;                                          // Init TFT interface
  tft.fillRect ( 0, 0, 160, 128, BLACK ) ;               // Clear screen does not work when rotated
  tft.setRotation ( 3 ) ;                                // Use landscape format
  tft.clearScreen() ;                                    // Clear screen
  tft.setTextSize ( 1 ) ;                                // Small character font
  tft.setTextColor ( WHITE ) ;                           // Info in white
  tft.println ( "Starting..." ) ;
  tft.print ( "Version:" ) ;
  strncpy ( tmpstr, VERSION, 16 ) ;                      // Limit version length
  tft.println ( tmpstr ) ;
  tft.println ( "By Ed Smallenburg" ) ;
#endif
  ringbuf = (uint8_t*) malloc ( RINGBFSIZ ) ;            // Create ring buffer
  for ( i = 0 ; (pinnr = progpin[i].gpio) >= 0 ; i++ )   // Check programmable input pins
  {
    pinMode ( pinnr, INPUT_PULLUP ) ;                    // Input for control button
    delay ( 10 ) ;
    // Check if pull-up active
    if ( ( progpin[i].cur = digitalRead ( pinnr ) ) == HIGH )
    {
      p = "HIGH" ;
    }
    else
    {
      p = "LOW, probably no PULL-UP" ;                   // No Pull-up
    }
    dbgprint ( "GPIO%d is %s", pinnr, p ) ;
  }
  readprogbuttons() ;                                    // Program the free input pins
  xml.init ( xmlbuffer, sizeof(xmlbuffer),               // Initilize XML stream.
             &XML_callback ) ;
  memset ( &ini_block, 0, sizeof(ini_block) ) ;          // Init ini_block
  ini_block.mqttport = 1883 ;                            // Default port for MQTT
  ini_block.mqttprefix = "" ;                            // No prefix for MQTT topics seen yet
  mk_lsan() ;                                            // Make al list of acceptable networks in preferences.
  WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
  //WiFi.persistent ( false ) ;                          // Do not save SSID and password
  WiFi.disconnect() ;                                    // After restart the router could still keep the old connection
  delay ( 100 ) ;
  listNetworks() ;                                       // Search for WiFi networks
  readprefs ( false ) ;                                  // Read preferences
  getpresets() ;                                         // Get the presets from preferences
  tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA, NAME ) ;
  SPI.begin() ;                                          // Init VSPI bus with default pins
  vs1053player.begin() ;                                 // Initialize VS1053 player
  delay(10);
  dbgprint ( "Connect to WiFi" ) ;
  NetworkFound = connectwifi() ;                         // Connect to WiFi network
  dbgprint ( "Start server for commands" ) ;
  cmdserver.begin() ;
  if ( NetworkFound )                                    // OTA and MQTT only if Wifi network found
  {
    mqtt_on = ( ini_block.mqttbroker.length() > 0 ) &&   // Use MQTT if broker specified
              ( ini_block.mqttbroker != "none" ) ;
    ArduinoOTA.setHostname ( NAME ) ;                    // Set the hostname
    ArduinoOTA.onStart ( otastart ) ;
    ArduinoOTA.begin() ;                                 // Allow update over the air
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
      dbgprint ( "MQTT uses prefix %s", ini_block.mqttprefix.c_str() ) ;
      dbgprint ( "Init MQTT" ) ;
      mqttclient.setServer(ini_block.mqttbroker.c_str(), // Specify the broker
                           ini_block.mqttport ) ;        // And the port
      mqttclient.setCallback ( onMqttMessage ) ;         // Set callback on receive
    }
  }
  else
  {
    currentpreset = ini_block.newpreset ;                // No network: do not start radio
  }
  timer = timerBegin ( 0, 80, true ) ;                   // User 1st timer with prescaler 80
  timerAttachInterrupt ( timer, &timer100, true ) ;      // Call timer100() on timer alarm
  timerAlarmWrite ( timer, 100000, true ) ;              // Alarm every 100 msec
  timerAlarmEnable ( timer ) ;                           // Enable the timer
  delay ( 1000 ) ;                                       // Show IP for a while
#if defined ( SDCARDCS )
  if ( !SD.begin ( SDCARDCS ) )                          // Try to init SD card driver
  {
    dbgprint ( "SD Card Mount Failed!" ) ;               // No success, check formatting (FAT)
  }
  else
  {
    SDokay = ( SD.cardType() != CARD_NONE ) ;            // See if known card
    if ( !SDokay )
    {
      dbgprint ( "No SD card attached" ) ;               // Card not readable
    }
    else
    {
      dbgprint ( "Locate mp3 files on SD, may take a while.." ) ;
      SD_nodecount = listsdtracks ( "/", 0, false ) ;    // Build nodelist
      dbgprint ( "Finished, %d tracks found", SD_nodecount ) ;
    }
  }
#endif
}


//******************************************************************************************
//                                R I N B Y T                                              *
//******************************************************************************************
// Read next byte from http inputbuffer.  Buffered for speed reasons.                      *
//******************************************************************************************
uint8_t rinbyt ( bool forcestart )
{
  static uint8_t  buf[1024] ;                           // Inputbuffer
  static uint16_t i ;                                   // Pointer in inputbuffer
  static uint16_t len ;                                 // Number of bytes in buf
  uint16_t        tlen ;                                // Number of available bytes
  uint16_t        trycount = 0 ;                        // Limit max. time to read

  if ( forcestart || ( i == len ) )                     // Time to read new buffer
  {
    while ( cmdclient.connected() )                     // Loop while the client's connected
    {
      tlen = cmdclient.available() ;                    // Number of bytes to read from the client
      len = tlen ;                                      // Try to read whole input
      if ( len == 0 )                                   // Any input available?
      {
        if ( ++trycount > 3 )                           // Not for a long time?
        {
          dbgprint ( "HTTP input shorter than expected" ) ;
          return '\n' ;                                 // Error! No input
        }
        delay ( 10 ) ;                                  // Give communication some time
        continue ;                                      // Next loop of no input yet
      }
      if ( len > sizeof(buf) )                          // Limit number of bytes
      {
        len = sizeof(buf) ;
      }
      len = cmdclient.read ( buf, len ) ;               // Read a number of bytes from the stream
      i = 0 ;                                           // Pointer to begin of buffer
      break ;
    }
  }
  return buf[i++] ;
}


//******************************************************************************************
//                                W R I T E P R E F S                                      *
//******************************************************************************************
// Update the preferences.  Called from the web interface.                                 *
//******************************************************************************************
void writeprefs()
{
  int     inx ;                                           // Position in inputstr
  char    c ;                                             // Input character
  uint8_t nlcount = 0 ;                                   // For double newline detection
  String  inputstr = "" ;                                 // Input regel
  String  key, contents ;                                 // Pair for Preferences entry

  nvsclear() ;                                            // Remove all preferences
  while ( true )
  {
    c = rinbyt ( false ) ;                                // Get next inputcharacter
    if ( c == '\n' )                                      // Newline?
    {
      if ( inputstr.length() == 0 )
      {
        dbgprint ( "End of writing preferences" ) ;
        break ;                                           // End of contents
      }
      if ( !inputstr.startsWith ( "#" ) )                 // Skip pure comment lines
      {
        inx = inputstr.indexOf ( "=" ) ;
        if ( inx >= 0 )                                   // Line with "="?
        {
          key = inputstr.substring ( 0, inx ) ;           // Yes, isolate the key
          key.trim() ;
          contents = inputstr.substring ( inx + 1 ) ;     // and contents
          contents.trim() ;
          nvssetstr ( key.c_str(), contents ) ;           // Save new pair
          if ( ( contents.indexOf ( "passw" ) >= 0 ) ||   // Do not reveal passwords
               ( contents.indexOf ( "wifi_" ) >= 0 ) )
          {
            contents = "*******" ;                        // Show asterisks instead
          }
          dbgprint ( "writeprefs %s = %s",
                     key.c_str(), contents.c_str() ) ;
        }
      }
      inputstr = "" ;
    }
    else
    {
      if ( c != '\r' )                                    // Not newline.  Is is a CR?
      {
        inputstr += String ( c ) ;                        // No, normal char, add to string
      }
    }
  }
}


//******************************************************************************************
//                                H A N D L E H T T P R E P L Y                            *
//******************************************************************************************
// Handle the output after an http request.                                                *
//******************************************************************************************
void handlehttpreply()
{
  const char*   p ;                                         // Pointer to reply if command
  String        sndstr = "" ;                               // String to send
  int           n ;                                         // Number of files on SD card

  if ( http_reponse_flag )
  {
    http_reponse_flag = false ;
    if ( cmdclient.connected() )
    {
      if ( http_rqfile.length() == 0 &&                     // An empty "GET"?
           http_getcmd.length() == 0 )
      {
        if ( NetworkFound )                                 // Yes, check network
        {
          handleFSf ( String( "index.html") ) ;             // Okay, send the startpage
        }
        else
        {
          handleFSf ( String( "config.html") ) ;            // Or the configuration page if in AP mode
        }
      }
      else
      {
        if ( http_getcmd.length() )                         // Command to analyze?
        {
          dbgprint ( "Start reply for %s", http_getcmd.c_str() ) ;
          sndstr = httpheader ( String ( "text/html" ) ) ;  // Set header
          if ( http_getcmd.startsWith ( "getprefs" ) )      // Is it a "Get preferences"?
          {
            sndstr += readprefs ( true ) ;                  // Read and send
          }
          else if ( http_getcmd.startsWith ( "getdefs" ) )  // Is it a "Get default preferences"?
          {
            sndstr += String ( defprefs_txt + 1 ) ;         // Yes, read initial values
          }
          else if ( http_getcmd.startsWith ("saveprefs") )  // Is is a "Save preferences"
          {
            writeprefs() ;                                  // Yes, handle it
          }
          else if ( http_getcmd.startsWith ("mp3list") )    // Is is a "Get SD MP3 tracklist"
          {
            cmdclient.print ( sndstr ) ;                    // Send header
            n = listsdtracks ( "/" ) ;                      // Yes, handle it
            dbgprint ( "%d tracks found on SD card", n ) ;
            return ;                                        // Do not send empty line
          }
          else
          {
            p = analyzeCmd ( http_getcmd.c_str() ) ;        // Yes, do so
            sndstr += String ( p ) ;                        // Content of HTTP response follows the header
          }
          sndstr += String ( "\n" ) ;                       // The HTTP response ends with a blank line
          cmdclient.print ( sndstr ) ;
        }
        else if ( http_rqfile.length() )                    // File requested?
        {
          dbgprint ( "Start file reply for %s",
                     http_rqfile.c_str() ) ;
          handleFSf ( http_rqfile ) ;                       // Yes, send it
        }
        else
        {
          httpheader ( String ( "text/html" ) ) ;           // Send header
          // the content of the HTTP response follows the header:
          cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
          dbgprint ( "Dummy response sent" ) ;
        }
      }
    }
  }
}


//******************************************************************************************
//                                H A N D L E H T T P                                      *
//******************************************************************************************
// Handle the input of an http request.                                                    *
//******************************************************************************************
void handlehttp()
{
  bool        first = true ;                                 // First call to rinbyt()
  char        c ;                                            // Next character from http input
  int         inx0, inx ;                                    // Pos. of search string in currenLine
  String      currentLine = "" ;                             // Build up to complete line
  bool        reqseen = false ;                              // No GET seen yet

  if ( !cmdclient.connected() )                              // Action if client is connected
  {
    return ;                                                 // No client active
  }
  dbgprint ( "handlehttp started" ) ;
  while ( true )                                             // Loop till command/file seen
  {
    c = rinbyt ( first ) ;                                   // Get a byte
    first = false ;                                          // No more first call
    if ( c == '\n' )
    {
      // If the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if ( currentLine.length() == 0 )
      {
        http_reponse_flag = reqseen ;                        // Response required or not
        break ;
      }
      else
      {
        // Newline seen, remember if it is like "GET /xxx?y=2&b=9 HTTP/1.1"
        if ( currentLine.startsWith ( "GET /" ) )            // GET request?
        {
          inx0 = 5 ;                                         // Start search at pos 5
        }
        else if ( currentLine.startsWith ( "POST /" ) )      // POST request?
        {
          inx0 = 6 ;
        }
        else
        {
          inx0 = 0 ;                                         // Not GET nor POST
        }
        if ( inx0 )                                          // GET or POST request?
        {
          reqseen = true ;                                   // Request seen
          inx = currentLine.indexOf ( "&" ) ;                // Search for 2nd parameter
          if ( inx < 0 )
          {
            inx = currentLine.indexOf ( " HTTP" ) ;          // Search for end of GET command
          }
          // Isolate the command
          http_getcmd = currentLine.substring ( inx0, inx ) ;
          inx = http_getcmd.indexOf ( "?" ) ;                // Search for command
          if ( inx == 0 )                                    // Arguments only?
          {
            http_getcmd = http_getcmd.substring ( 1 ) ;      // Yes, get rid of question mark
            http_rqfile = "" ;                               // No file
          }
          else if ( inx > 0 )                                // Filename present?
          {
            http_rqfile = http_getcmd.substring ( 0, inx ) ; // Remember filename
            http_getcmd = http_getcmd.substring ( inx + 1 ) ; // Remove filename from GET command
          }
          else
          {
            http_rqfile = http_getcmd ;                      // No parameters, set filename
            http_getcmd = "" ;
          }
          if ( http_getcmd.length() )
          {
            dbgprint ( "Get command is: %s",                 // Show result
                       http_getcmd.c_str() ) ;
          }
          if ( http_rqfile.length() )
          {
            dbgprint ( "Filename is: %s",                    // Show requested file
                       http_rqfile.c_str() ) ;
          }
        }
        currentLine = "" ;
      }
    }
    else if ( c != '\r' )                                    // No LINFEED.  Is it a CR?
    {
      currentLine += c ;                                     // No, add normal char to currentLine
    }
  }
  //cmdclient.stop() ;
}


//******************************************************************************************
//                                  X M L _ C A L L B A C K                                *
//******************************************************************************************
// Process XML tags into variables.                                                        *
//******************************************************************************************
void XML_callback ( uint8_t statusflags, char* tagName, uint16_t tagNameLen,
                    char* data,  uint16_t dataLen )
{
  if ( statusflags & STATUS_START_TAG )
  {
    if ( tagNameLen )
    {
      xmlOpen = String ( tagName ) ;
      //dbgprint ( "Start tag %s",tagName ) ;
    }
  }
  else if ( statusflags & STATUS_END_TAG )
  {
    //dbgprint ( "End tag %s", tagName ) ;
  }
  else if ( statusflags & STATUS_TAG_TEXT )
  {
    xmlTag = String( tagName ) ;
    xmlData = String( data ) ;
    //dbgprint ( Serial.print( "Tag: %s, text: %s", tagName, data ) ;
  }
  else if ( statusflags & STATUS_ATTR_TEXT )
  {
    //dbgprint ( "Attribute: %s, text: %s", tagName, data ) ;
  }
  else if  ( statusflags & STATUS_ERROR )
  {
    //dbgprint ( "XML Parsing error  Tag: %s, text: %s", tagName, data ) ;
  }
}


//******************************************************************************************
//                                  X M L P A R S E                                        *
//******************************************************************************************
// Parses streams from XML data.                                                           *
//******************************************************************************************
String xmlparse ( String mount )
{
  // Example URL for XML Data Stream:
  // http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en
  // Clear all variables for use.
  char   tmpstr[200] ;                              // Full GET command, later stream URL
  char   c ;                                        // Next input character from reply
  String urlout ;                                   // Result URL
  bool   urlfound = false ;                         // Result found

  stationServer = "" ;
  stationPort = "" ;
  stationMount = "" ;
  xmlTag = "" ;
  xmlData = "" ;
  stop_mp3client() ; // Stop any current wificlient connections.
  dbgprint ( "Connect to new iHeartRadio host: %s", mount.c_str() ) ;
  datamode = INIT ;                                   // Start default in metamode
  chunked = false ;                                   // Assume not chunked
  // Create a GET commmand for the request.
  sprintf ( tmpstr, xmlget, mount.c_str() ) ;
  dbgprint ( "%s", tmpstr ) ;
  // Connect to XML stream.
  if ( mp3client.connect ( xmlhost, xmlport ) )
  {
    dbgprint ( "Connected!" ) ;
    mp3client.print ( String ( tmpstr ) + " HTTP/1.1\r\n"
                      "Host: " + xmlhost + "\r\n"
                      "User-Agent: Mozilla/5.0\r\n"
                      "Connection: close\r\n\r\n" ) ;
    dbgprint ( "XML parser processing..." ) ;
    while ( true )                                    // Process XML Data.
    {
      if ( mp3client.available() )
      {
        c = mp3client.read() ;
        xml.processChar ( c ) ;
        if ( xmlTag != "" )                           // Tag seen?
        {
          if ( xmlTag.endsWith ( "/status-code" ) )   // Status code seen?
          {
            if ( xmlData != "200" )                   // Good result?
            {
              dbgprint ( "Bad xml status-code %s",    // No, show and stop interpreting
                         xmlData.c_str() ) ;
              break ;
            }
          }
          if ( xmlTag.endsWith ( "/ip" ) )
          {
            stationServer = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/port" ) )
          {
            stationPort = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/mount"  ) )
          {
            stationMount = xmlData ;
          }
        }
      }
      // Check if all the station values are stored.
      urlfound = ( stationServer != "" && stationPort != "" && stationMount != "" ) ;
      if ( urlfound )
      {
        xml.reset() ;
        break ;
      }
    }
    tmpstr[0] = '\0' ;
    if ( urlfound )
    {
      sprintf ( tmpstr, "%s:%s/%s_SC",                   // Build URL for ESP-Radio to stream.
                stationServer.c_str(),
                stationPort.c_str(),
                stationMount.c_str() ) ;
      dbgprint ( "Found: %s", tmpstr ) ;
    }
    dbgprint ( "Closing XML connection." ) ;
  }
  else
  {
    dbgprint ( "Can't connect to XML host!" ) ;
    tmpstr[0] = '\0' ;
  }
  return String ( tmpstr ) ;                           // Return final streaming URL.
}


//******************************************************************************************
//                              H A N D L E S A V E R E Q                                  *
//******************************************************************************************
// Handle save volume/preset/tone.  This will save current settings every 10 minutes to    *
// the preferences.  On the next restart these values will be loaded.                      *
// Note that saving prefences will only take place if contents has changed.                *
//******************************************************************************************
void handleSaveReq()
{
  static uint32_t savetime = 0 ;                          // Limit save to once per 10 minutes

  if ( ( millis() - savetime ) < 600000 )                 // 600 sec is 10 minutes
  {
    return ;
  }
  savetime = millis() ;                                   // Set time of last save
  nvssetstr ( "preset", String ( currentpreset )  ) ;     // Save current preset
  nvssetstr ( "volume", String ( ini_block.reqvol ) );    // Save current volue
  nvssetstr ( "toneha", String ( ini_block.rtone[0] ) ) ; // Save current toneha
  nvssetstr ( "tonehf", String ( ini_block.rtone[1] ) ) ; // Save current tonehf
  nvssetstr ( "tonela", String ( ini_block.rtone[2] ) ) ; // Save current tonela
  nvssetstr ( "tonelf", String ( ini_block.rtone[3] ) ) ; // Save current tonelf
}


//******************************************************************************************
//                              H A N D L E I P P U B                                      *
//******************************************************************************************
// Handle publish op IP to MQTT.  This will happen every 10 minutes.                       *
//******************************************************************************************
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


//******************************************************************************************
//                                   M P 3 L O O P                                         *
//******************************************************************************************
// Called from the mail loop() for the mp3 functions.                                      *
// A connection to an MP3 server is active and we are ready to receive data.               *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the    *
// sender.                                                                                 *
//******************************************************************************************
void mp3loop()
{
  static uint8_t  tmpbuff[1024] ;                        // Input buffer for mp3 stream
  uint32_t        maxchunk ;                             // Max number of bytes to read
  int             res = 0 ;                              // Result reading from mp3 stream
  int             i ;                                    // Index in tmpbuff
  uint32_t        rs ;                                   // Free space in ringbuffer
  uint32_t        av ;                                   // Available in stream

  // Try to keep the ringbuffer filled up by adding as much bytes as possible
  if ( datamode & ( INIT | HEADER | DATA |               // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    rs = ringspace() ;                                   // Get free ringbuffer space
    if ( rs >= sizeof(tmpbuff) )                         // Need to fill the ringbuffer?
    {
      maxchunk = sizeof(tmpbuff) ;                       // Reduce byte count for this mp3loop()
      if ( localfile )                                   // Playing file from SD card?
      {
        av = mp3file.available() ;                       // Bytes left in file
        if ( av < maxchunk )                             // Reduce byte count for this mp3loop()
        {
          maxchunk = av ;
        }
        if ( maxchunk )                                  // Anything to read?
        {
          res = mp3file.read ( tmpbuff, maxchunk ) ;     // Read a block of data
        }
      }
      else
      {
        av = mp3client.available() ;                     // Available from stream
        if ( av < maxchunk )                             // Limit read size
        {
          maxchunk = av ;
        }
        if ( maxchunk )                                  // Anything to read?
        {
          res = mp3client.read ( tmpbuff, maxchunk ) ;   // Read a number of bytes from the stream
        }
      }
      putring ( tmpbuff, res ) ;                         // Transfer to ringbuffer
    }
  }
  while ( vs1053player.data_request() && ringavail() )   // Try to keep VS1053 filled
  {
    handlebyte_ch ( getring() ) ;                        // Yes, handle it
  }
  if ( datamode == STOPREQD )                            // STOP requested?
  {
    dbgprint ( "STOP requested" ) ;
    if ( localfile )
    {
      mp3file.close() ;
    }
    else
    {
      stop_mp3client() ;                                 // Disconnect if still connected
    }
    handlebyte_ch ( 0, true ) ;                          // Force flush of buffer
    vs1053player.setVolume ( 0 ) ;                       // Mute
    vs1053player.stopSong() ;                            // Stop playing
    emptyring() ;                                        // Empty the ringbuffer
    metaint = 0 ;                                        // No metaint known now
    datamode = STOPPED ;                                 // Yes, state becomes STOPPED
#if defined ( USETFT )
    tft.fillRect ( 0, 0, 160, 128, BLACK ) ;             // Clear screen does not work when rotated
#endif
    delay ( 500 ) ;
  }
  if ( localfile )                                       // Playin from SD?
  {
    if ( datamode & ( INIT | HEADER | DATA |             // Test op playing
                      METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER |
                      PLAYLISTDATA ) )
    {
      if ( ( av == 0 ) && ( ringavail() == 0 ) )         // End of mp3 data?
      {
        datamode = STOPREQD ;                            // End of local mp3-file detected
        selectnextSDfile() ;                             // Select the next file on SD
      }
    }
  }
  if ( ini_block.newpreset != currentpreset )            // New station or next from playlist requested?
  {
    if ( datamode != STOPPED )                           // Yes, still busy?
    {
      datamode = STOPREQD ;                              // Yes, request STOP
    }
    else
    {
      if ( playlist_num )                                 // Playing from playlist?
      { // Yes, retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset ;                   // Next entry in playlist
        ini_block.newpreset = currentpreset ;             // Stay at current preset
      }
      else
      {
        host = readhostfrompref() ;                       // Lookup preset in preferences
        chomp ( host ) ;                                  // Get rid of part after "#"
      }
      dbgprint ( "New preset/file requested (%d/%d) from %s",
                 ini_block.newpreset, playlist_num, host.c_str() ) ;
      if ( host != ""  )                                  // Preset in ini-file?
      {
        hostreq = true ;                                  // Force this station as new preset
      }
      else
      {
        // This preset is not available, return to preset 0, will be handled in next mp3loop()
        dbgprint ( "No host for this preset" ) ;
        ini_block.newpreset = 0 ;                         // Wrap to first station
      }
    }
  }
  if ( hostreq )                                          // New preset or station?
  {
    hostreq = false ;
    currentpreset = ini_block.newpreset ;                 // Remember current preset
    // Find out if this URL is on localhost.  Not yet implemented.
    localfile = ( host.indexOf ( "localhost/" ) >= 0 ) ;
    if ( localfile )                                      // Play file from localhost?
    {
      if ( connecttofile() )                              // Yes, open mp3-file
      {
        datamode = DATA ;                                 // Start in DATA mode
      }
    }
    else
    {
      if ( host.startsWith ( "ihr/" ) )                   // iHeartRadio station requested?
      {
        host = host.substring ( 4 ) ;                     // Yes, remove "ihr/"
        host = xmlparse ( host ) ;                        // Parse the xml to get the host
      }
      connecttohost() ;                                   // Switch to new host
    }
  }
  if ( xmlreq )                                           // Directly xml requested?
  {
    xmlreq = false ;                                      // Yes, clear request flag
    host = xmlparse ( host ) ;                            // Parse the xml to get the host
    connecttohost() ;                                     // and connect to this host
  }
}


//******************************************************************************************
//                                   L O O P                                               *
//******************************************************************************************
// Main loop of the program.                                                               *
//******************************************************************************************
void loop()
{
  mp3loop() ;                                               // Do mp3 related actions
  if ( reqtone )                                            // Request to change tone?
  {
    reqtone = false ;
    vs1053player.setTone ( ini_block.rtone ) ;              // Set SCI_BASS to requested value
  }
  if ( resetreq )                                           // Reset requested?
  {
    delay ( 1000 ) ;                                        // Yes, wait some time
    ESP.restart() ;                                         // Reboot
  }
  if ( muteflag )
  {
    vs1053player.setVolume ( 0 ) ;                          // Mute
  }
  else
  {
    vs1053player.setVolume ( ini_block.reqvol ) ;           // Unmute
  }
  displayvolume() ;                                         // Show volume on display
  scanserial() ;                                            // Handle serial input
  scandigital() ;                                           // Scan digital inputs
  scanIR() ;                                                // See if IR input
  ArduinoOTA.handle() ;                                     // Check for OTA
  handlehttpreply() ;
  cmdclient = cmdserver.available() ;                       // Check Input from client?
  if ( cmdclient )                                          // Client connected?
  {
    dbgprint ( "Command client available" ) ;
    handlehttp() ;
  }
  // Handle MQTT.
  if ( mqtt_on )
  {
    if ( !mqttclient.connected() )                          // See if connected
    {
      mqttreconnect() ;                                     // No, reconnect
    }
    else
    {
      mqttpub.publishtopic() ;                              // Check if any publishing to do
    }
    mqttclient.loop() ;                                     // Handling of MQTT connection
  }
  handleSaveReq() ;                                         // See if time to save settings
  handleIpPub() ;                                           // See if time to publish IP
}


//******************************************************************************************
//                            C H K H D R L I N E                                          *
//******************************************************************************************
// Check if a line in the header is a reasonable headerline.                               *
// Normally it should contain something like "icy-xxxx:abcdef".                            *
//******************************************************************************************
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
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
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


//******************************************************************************************
//                           H A N D L E B Y T E _ C H                                     *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// Chunked transfer encoding aware. Chunk extensions are not supported.                    *
//******************************************************************************************
void handlebyte_ch ( uint8_t b, bool force )
{
  static int  chunksize = 0 ;                         // Chunkcount read from stream

  if ( chunked && !force &&
       ( datamode & ( DATA |                          // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                            // Expecting a new chunkcount?
    {
      if ( b == '\r' )                               // Skip CR
      {
        return ;
      }
      else if ( b == '\n' )                          // LF ?
      {
        chunkcount = chunksize ;                     // Yes, set new count
        chunksize = 0 ;                              // For next decode
        return ;
      }
      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper ( b ) - '0' ;                      // Be sure we have uppercase
      if ( b > 9 )
      {
        b = b - 7 ;                                  // Translate A..F to 10..15
      }
      chunksize = ( chunksize << 4 ) + b ;
    }
    else
    {
      handlebyte ( b, force ) ;                       // Normal data byte
      chunkcount-- ;                                  // Update count to next chunksize block
    }
  }
  else
  {
    handlebyte ( b, force ) ;                         // Normal handling of this byte
  }
}


//******************************************************************************************
//                           H A N D L E B Y T E                                           *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// This byte will be send to the VS1053 most of the time.                                  *
// Note that the buffer the data chunk must start at an address that is a muttiple of 4.   *
// Set force to true if chunkbuffer must be flushed.                                       *
//******************************************************************************************
void handlebyte ( uint8_t b, bool force )
{
  static uint16_t  playlistcnt ;                       // Counter to find right entry in playlist
  static bool      firstmetabyte ;                     // True if first metabyte (counter)
  static int       LFcount ;                           // Detection of end of header
  static __attribute__((aligned(4))) uint8_t buf[32] ; // Buffer for chunk
  static int       bufcnt = 0 ;                        // Data in chunk
  static bool      firstchunk = true ;                 // First chunk as input
  String           lcml ;                              // Lower case metaline
  String           ct ;                                // Contents type
  static bool      ctseen = false ;                    // First line of header seen or not
  int              inx ;                               // Pointer in metaline
  int              i ;                                 // Loop control

  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    dbgprint ( "Switch to HEADER" ) ;
    datamode = HEADER ;                                // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metaline = "" ;                                    // No metadata yet
    firstchunk = true ;                                // First chunk expected
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    buf[bufcnt++] = b ;                                // Save byte in chunkbuffer
    if ( bufcnt == sizeof(buf) || force )              // Buffer full?
    {
      if ( firstchunk )
      {
        firstchunk = false ;
        dbgprint ( "First chunk:" ) ;                  // Header for printout of first chunk
        for ( i = 0 ; i < 32 ; i += 8 )                // Print 4 lines
        {
          dbgprint ( "%02X %02X %02X %02X %02X %02X %02X %02X",
                     buf[i],   buf[i + 1], buf[i + 2], buf[i + 3],
                     buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7] ) ;
        }
      }
      vs1053player.playChunk ( buf, bufcnt ) ;         // Yes, send to player
      bufcnt = 0 ;                                     // Reset count
    }
    totalcount++ ;                                     // Count number of bytes, ignore overflow
    if ( metaint != 0 )                                // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        if ( bufcnt )                                  // Yes, still data in buffer?
        {
          vs1053player.playChunk ( buf, bufcnt ) ;     // Yes, send to player
          bufcnt = 0 ;                                 // Reset count
        }
        datamode = METADATA ;
        firstmetabyte = true ;                         // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( //( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      if ( chkhdrline ( metaline.c_str() ) )           // Reasonable input?
      {
        lcml = metaline ;                              // Use lower case for compare
        lcml.toLowerCase() ;
        dbgprint ( metaline.c_str() ) ;                // Yes, Show it
        if ( lcml.indexOf ( "content-type" ) >= 0)     // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          ct = metaline.substring ( 14 ) ;             // Set contentstype. Not used yet
          dbgprint ( "%s seen.", ct.c_str() ) ;
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( metaline.startsWith ("icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
          icyname.trim() ;                             // Remove leading and trailing spaces
          displayinfo ( icyname.c_str(), 60, 68,
                        YELLOW ) ;                     // Show station name at position 60
          mqttpub.trigger ( MQTT_ICYNAME ) ;           // Request publishing to MQTT
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( metaline.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metaline = "" ;                                  // Reset this line
      if ( ( LFcount == 2 ) && ctseen )                // Some data seen and a double LF?
      {
        dbgprint ( "Switch to DATA, bitrate is %d"     // Show bitrate
                   ", metaint is %d",                  // and metaint
                   bitrate, metaint ) ;
        datamode = DATA ;                              // Expecting data now
        datacount = metaint ;                          // Number of bytes before first metadata
        bufcnt = 0 ;                                   // Reset buffer count
        vs1053player.startSong() ;                     // Start a new song
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( firstmetabyte )                               // First byte of metadata?
    {
      firstmetabyte = false ;                          // Not the first anymore
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        dbgprint ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
      metaline = "" ;                                  // Set to empty
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
    }
    if ( --metacount == 0 )
    {
      if ( metaline.length() )                         // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metaline.c_str() ) ;         // Show artist and title if present in metadata
        mqttpub.trigger ( MQTT_STREAMTITLE ) ;         // Request publishing to MQTT
      }
      if ( metaline.length() > 1500 )                  // Unlikely metaline length?
      {
        dbgprint ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
        metaline = "" ;                                // Do not waste memory on this
      }
      datacount = metaint ;                            // Reset data count
      bufcnt = 0 ;                                     // Reset buffer count
      datamode = DATA ;                                // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                      // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metaline = "" ;                                    // Prepare for new line
    LFcount = 0 ;                                      // For detection end of header
    datamode = PLAYLISTHEADER ;                        // Handle playlist data
    playlistcnt = 1 ;                                  // Reset for compare
    totalcount = 0 ;                                   // Reset totalcount
    dbgprint ( "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                    // Read header
  {
    if (// ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      dbgprint ( "Playlistheader: %s",                 // Show playlistheader
                 metaline.c_str() ) ;
      metaline = "" ;                                  // Ready for next line
      if ( LFcount == 2 )
      {
        dbgprint ( "Switch to PLAYLISTDATA" ) ;
        datamode = PLAYLISTDATA ;                      // Expecting data now
        return ;
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                      // Read next byte of .m3u file data
  {
    if (// ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      dbgprint ( "Playlistdata: %s",                   // Show playlistheader
                 metaline.c_str() ) ;
      if ( metaline.length() < 5 )                     // Skip short lines
      {
        metaline = "" ;                                // Flush line
        return ;
      }
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )      // Info?
      {
        if ( playlist_num == playlistcnt )             // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;             // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) ;
            mqttpub.trigger ( MQTT_STREAMTITLE ) ;     // Request publishing to MQTT
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )               // Commentline?
      {
        metaline = "" ;
        return ;                                       // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ( "Entry %d in playlist found: %s", playlistcnt, metaline.c_str() ) ;
      if ( playlist_num == playlistcnt  )
      {
        inx = metaline.indexOf ( "http://" ) ;         // Search for "http://"
        if ( inx >= 0 )                                // Does URL contain "http://"?
        {
          host = metaline.substring ( inx + 7 ) ;      // Yes, remove it and set host
        }
        else
        {
          host = metaline ;                            // Yes, set new host
        }
        connecttohost() ;                              // Connect to it
      }
      metaline = "" ;
      host = playlist ;                                // Back to the .m3u host
      playlistcnt++ ;                                  // Next entry in playlist
    }
    else
    {
      metaline += (char)b ;                            // Normal character, add it to metaline
    }
    return ;
  }
}


//******************************************************************************************
//                             G E T C O N T E N T T Y P E                                 *
//******************************************************************************************
// Returns the contenttype of a file to send.                                              *
//******************************************************************************************
String getContentType ( String filename )
{
  if      ( filename.endsWith ( ".html" ) ) return "text/html" ;
  else if ( filename.endsWith ( ".png"  ) ) return "image/png" ;
  else if ( filename.endsWith ( ".gif"  ) ) return "image/gif" ;
  else if ( filename.endsWith ( ".jpg"  ) ) return "image/jpeg" ;
  else if ( filename.endsWith ( ".ico"  ) ) return "image/x-icon" ;
  else if ( filename.endsWith ( ".css"  ) ) return "text/css" ;
  else if ( filename.endsWith ( ".zip"  ) ) return "application/x-zip" ;
  else if ( filename.endsWith ( ".gz"   ) ) return "application/x-gzip" ;
  else if ( filename.endsWith ( ".mp3"  ) ) return "audio/mpeg" ;
  else if ( filename.endsWith ( ".pw"   ) ) return "" ;              // Passwords are secret
  return "text/plain" ;
}


//******************************************************************************************
//                                H A N D L E F S F                                        *
//******************************************************************************************
// Handling of requesting pages from the PROGMEM. Example: favicon.ico                     *
//******************************************************************************************
void handleFSf ( const String& pagename )
{
  String                 ct ;                           // Content type
  const char*            p ;
  int                    l ;                            // Size of requested page
  int                    TCPCHUNKSIZE = 1024 ;          // Max number of bytes per write

  dbgprint ( "FileRequest received %s", pagename.c_str() ) ;
  ct = getContentType ( pagename ) ;                    // Get content type
  if ( ( ct == "" ) || ( pagename == "" ) )             // Empty is illegal
  {
    cmdclient.println ( "HTTP/1.1 404 Not Found" ) ;
    cmdclient.println ( "" ) ;
    return ;
  }
  else
  {
    if ( pagename.indexOf ( "index.html" ) >= 0 )       // Index page is in PROGMEM
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    else if ( pagename.indexOf ( "radio.css" ) >= 0 )   // CSS file is in PROGMEM
    {
      p = radio_css + 1 ;
      l = sizeof ( radio_css ) ;
    }
    else if ( pagename.indexOf ( "config.html" ) >= 0 ) // Config page is in PROGMEM
    {
      p = config_html ;
      l = sizeof ( config_html ) ;
    }
    else if ( pagename.indexOf ( "mp3play.html" ) >= 0 ) // Mp3player page is in PROGMEM
    {
      p = mp3play_html ;
      l = sizeof ( mp3play_html ) ;
    }
    else if ( pagename.indexOf ( "about.html" ) >= 0 )  // About page is in PROGMEM
    {
      p = about_html ;
      l = sizeof ( about_html ) ;
    }
    else if ( pagename.indexOf ( "favicon.ico" ) >= 0 ) // Favicon icon is in PROGMEM
    {
      p = (char*)favicon_ico ;
      l = sizeof ( favicon_ico ) ;
    }
    else
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    if ( *p == '\n' )                                   // If page starts with newline:
    {
      p++ ;                                             // Skip first character
      l-- ;
    }
    dbgprint ( "Length of page is %d", strlen ( p ) ) ;
    cmdclient.print ( httpheader ( ct ) ) ;             // Send header
    // The content of the HTTP response follows the header:
    if ( l < 10 )
    {
      cmdclient.println ( "Testline<br>" ) ;
    }
    else
    {
      while ( l )                                       // Loop through the output page
      {
        if ( l <= TCPCHUNKSIZE )                        // Near the end?
        {
          cmdclient.write ( p, l ) ;                    // Yes, send last part
          l = 0 ;
        }
        else
        {
          cmdclient.write ( p, TCPCHUNKSIZE ) ;         // Send part of the page
          p += TCPCHUNKSIZE ;                           // Update startpoint and rest of bytes
          l -= TCPCHUNKSIZE ;
        }
      }
    }
    // The HTTP response ends with another blank line:
    cmdclient.println() ;
    dbgprint ( "Response send" ) ;
  }
}


//******************************************************************************************
//                             A N A L Y Z E C M D                                         *
//******************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                 *
// Version for handling string with: <parameter>=<value>                                   *
//******************************************************************************************
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


//******************************************************************************************
//                                 C H O M P                                               *
//******************************************************************************************
// Do some filtering on de inputstring:                                                    *
//  - String comment part (starting with "#").                                             *
//  - Strip trailing CR.                                                                   *
//  - Strip leading spaces.                                                                *
//  - Strip trailing spaces.                                                               *
//******************************************************************************************
void chomp ( String &str )
{
  int   inx ;                                         // Index in de input string

  if ( ( inx = str.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    str.remove ( inx ) ;                              // Yes, remove
  }
  str.trim() ;                                        // Remove spaces and CR
}


//******************************************************************************************
//                             A N A L Y Z E C M D                                         *
//******************************************************************************************
// Handling of the various commands from remote webclient, serial or MQTT.                 *
// par holds the parametername and val holds the value.                                    *
// "wifi_00" and "preset_00" may appear more than once, like wifi_01, wifi_02, etc.        *
// Examples with available parameters:                                                     *
//   preset     = 12                        // Select start preset to connect to           *
//   preset_00  = <mp3 stream>              // Specify station for a preset 00-99 *)       *
//   volume     = 95                        // Percentage between 0 and 100                *
//   upvolume   = 2                         // Add percentage to current volume            *
//   downvolume = 2                         // Subtract percentage from current volume     *
//   toneha     = <0..15>                   // Setting treble gain                         *
//   tonehf     = <0..15>                   // Setting treble frequency                    *
//   tonela     = <0..15>                   // Setting bass gain                           *
//   tonelf     = <0..15>                   // Setting treble frequency                    *
//   station    = <mp3 stream>              // Select new station (will not be saved)      *
//   station    = <URL>.mp3                 // Play standalone .mp3 file (not saved)       *
//   station    = <URL>.m3u                 // Select playlist (will not be saved)         *
//   stop                                   // Stop playing                                *
//   resume                                 // Resume playing                              *
//   mute                                   // Mute/unmute the music (toggle)              *
//   wifi_00    = mySSID/mypassword         // Set WiFi SSID and password *)               *
//   mqttbroker = mybroker.com              // Set MQTT broker to use *)                   *
//   mqttprefix = XP93g                     // Set MQTT broker to use                      *
//   mqttport   = 1883                      // Set MQTT port to use, default 1883 *)       *
//   mqttuser   = myuser                    // Set MQTT user for authentication *)         *
//   mqttpasswd = mypassword                // Set MQTT password for authentication *)     *
//   mp3track   = <nodeID>                  // Play track from SD card, nodeID 0 = random  *
//   status                                 // Show current URL to play                    *
//   test                                   // For test purposes                           *
//   debug      = 0 or 1                    // Switch debugging on or off                  *
//   reset                                  // Restart the ESP32                           *
//  Commands marked with "*)" are sensible in ini-file only                                *
//******************************************************************************************
const char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[180] ;                     // Reply to client, will be returned
  uint8_t            oldvol ;                         // Current volume
  bool               relative ;                       // Relative argument (+ or -)
  int                inx ;                            // Index in string
  String             tmpstr ;                         // Temporary for value

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
  ivalue = abs ( ivalue ) ;                           // Make positive
  relative = argument.indexOf ( "up" ) == 0 ;         // + relative setting?
  if ( argument.indexOf ( "down" ) == 0 )             // - relative setting?
  {
    relative = true ;                                 // It's relative
    ivalue = - ivalue ;                               // But with negative value
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  if ( value.length() )
  {
    tmpstr = value ;                                  // Make local copy of value
    if ( argument.indexOf ( "passw" ) >= 0 )          // Password in value?
    {
      tmpstr = String ( "******" ) ;                  // Yes, hide it
    }
    dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), tmpstr.c_str() ) ;
  }
  else
  {
    dbgprint ( "Command: %s (without parameter)",
               argument.c_str() ) ;
  }
  if ( argument.indexOf ( "volume" ) >= 0 )           // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    oldvol = vs1053player.getVolume() ;               // Get current volume
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = oldvol + ivalue ;            // Up by 0.5 or more dB
    }
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
    muteflag = false ;                                // Stop possibly muting
    sprintf ( reply, "Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
  }
  else if ( argument == "mute" )                      // Mute/unmute request
  {
    muteflag = !muteflag ;                            // Request volume to zero/normal
  }
  else if ( argument.indexOf ( "preset" ) >= 0 )      // (UP/DOWN)Preset station?
  {
    if ( relative )                                   // Relative argument?
    {
      ini_block.newpreset += ivalue ;                 // Yes, adjust currentpreset
    }
    else
    {
      ini_block.newpreset = ivalue ;                  // Otherwise set station
    }
    sprintf ( reply, "Preset is now %d",              // Reply new preset
              ini_block.newpreset ) ;
    playlist_num = 0 ;
  }
  else if ( argument == "stop" )                      // Stop requested?
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      datamode = STOPREQD ;                           // Request STOP
    }
    else
    {
      strcpy ( reply, "Command not accepted!" ) ;     // Error reply
    }
  }
  else if ( argument == "resume" )                    // Request to resume?
  {
    if ( datamode == STOPPED )                        // Yes, are we stopped?
    {
      hostreq = true ;                                // Yes, request restart
    }
  }
  else if ( ( argument == "mp3track" ) ||             // Select a track from SD card?
            ( argument == "station" ) )               // Station in the form address:port
  {
    if ( argument.startsWith ( "mp3" ) )              // MP3 track to search for
    {
      value = getSDfilename ( value ) ;               // like "localhost/........"
    }
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    hostreq = true ;                                  // Force this station as new preset
    sprintf ( reply,
              "Playing %s",                           // Format reply
              host.c_str() ) ;
    utf8ascii ( reply ) ;                             // Remove possible strange characters
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
  else if ( argument.startsWith ( "reset" ) )         // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument == "test" )                      // Test command
  {
    sprintf ( reply, "Free memory is %d, ringbuf %d, stream %d",
              ESP.getFreeHeap(), rcount, mp3client.available() ) ;
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
  }
  else if ( argument == "debug" )                     // debug on/off request?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "getnetworks" )               // List all WiFi networks?
  {
    sprintf ( reply, networks.c_str() ) ;             // Reply is SSIDs
  }
  else if ( argument.startsWith ( "list" ) )          // List all presets?
  {
    return presetlist.c_str() ;                       // Send preset list as reply
  }
  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//******************************************************************************************
//                             H T T P H E A D E R                                         *
//******************************************************************************************
// Set http headers to a string.                                                           *
//******************************************************************************************
String httpheader ( String contentstype )
{
  return String ( "HTTP/1.1 200 OK\nContent-type:" ) +
         contentstype +
         String ( "\n"
                  "Server: " NAME "\n"
                  "Cache-Control: " "max-age=3600\n"
                  "Last-Modified: " VERSION "\n\n" ) ;
}

