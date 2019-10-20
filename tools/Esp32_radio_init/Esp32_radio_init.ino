//********************************************************************************************
// Initialize the preferences op ESP32-radio.  They can be edited later by the webinterface. *
//********************************************************************************************
// 27-04-2017, ES: First set-up, derived from preferences example sketch.                    *
// 11-02-2019, ES: names of I/O pins changed.                                                *
//********************************************************************************************

#include <Preferences.h>

// Note: Namespace name is limited to 15 chars.  Same name must be used in ESP32-radio.
#define NAME "ESP32Radio"
// Adjust size of buffer to the longest expected string for nvsgetstr
#define NVSBUFSIZE 150

Preferences preferences ;         // Instance of Preferences


//******************************************************************************************
//                              S E T U P                                                  *
//******************************************************************************************
void setup()
{
  String       str ;
  
  Serial.begin ( 115200 ) ;
  Serial.println() ;
  preferences.begin ( NAME, false ) ;                 // Open for read/write
  preferences.clear() ;                               // Remove all preferences under opened namespace
  // Store the preferences for ESP32-radio
  preferences.putString ( "gpio_00",  "uppreset = 1" ) ;
  preferences.putString ( "gpio_12",  "upvolume = 2" ) ;
  preferences.putString ( "gpio_13",  "downvolume = 2" ) ;
  
  preferences.putString ( "mqttbroker",   "none" ) ;
  preferences.putString ( "mqttport",     "1883" ) ;
  preferences.putString ( "mqttuser",     "none" ) ;
  preferences.putString ( "mqttpasswd",   "none" ) ;
  preferences.putString ( "mqttprefix",   "none" ) ;
  //
  preferences.putString ( "wifi_00",     "gozal_2.4/asdffdsa" ) ;
  preferences.putString ( "wifi_01",     "SSID2/YYYYYY" ) ;
  //
  preferences.putString ( "volume",      "95" ) ;
  preferences.putString ( "toneha",      "0" ) ;
  preferences.putString ( "tonehf",      "0" ) ;
  preferences.putString ( "tonela",      "0" ) ;
  preferences.putString ( "tonelf",      "0" ) ;
  //
  preferences.putString ( "preset",      "1" ) ;
  // glzwizzlv.bynetcdn.com/glglz_mp3?awCollectionId=misc
  // http://glzwizzlv.bynetcdn.com/glglz_mp3?awCollectionId=misc&awEpisodeId=glglz
  preferences.putString ( "preset_01",   "glzwizzlv.bynetcdn.com/glglz_mp3?awCollectionId=misc                       #  1 - Glz, galey_tzahal" ) ;
  //preferences.putString ( "preset_00",   "109.206.96.34:8100                       #  0 - NAXI LOVE RADIO, Belgrade, Serbia" ) ;
  preferences.putString ( "preset_02",   "airspectrum.cdnstream1.com:8114/1648_128 #  2 - Easy Hits Florida 128k" ) ;
  preferences.putString ( "preset_03",   "us2.internet-radio.com:8050              #  3 - CLASSIC ROCK MIA WWW.SHERADIO.COM" ) ;
  preferences.putString ( "preset_04",   "airspectrum.cdnstream1.com:8000/1261_192 #  4 - Magic Oldies Florida" ) ;
  preferences.putString ( "preset_05",   "airspectrum.cdnstream1.com:8008/1604_128 #  5 - Magic 60s Florida 60s Classic Rock" ) ;
  preferences.putString ( "preset_06",   "us1.internet-radio.com:8105              #  6 - Classic Rock Florida - SHE Radio" ) ;
  preferences.putString ( "preset_07",   "icecast.omroep.nl:80/radio1-bb-mp3       #  7 - Radio 1, NL" ) ;
  preferences.putString ( "preset_08",   "205.164.62.15:10032                      #  8 - 1.FM - GAIA, 64k" ) ;
  preferences.putString ( "preset_09",   "skonto.ls.lv:8002/mp3                    #  9 - Skonto 128k" ) ;
  preferences.putString ( "preset_10",   "94.23.66.155:8106                        #  10 - *ILR CHILL and GROOVE" ) ;
  preferences.putString ( "preset_11",   "ihr/IHR_IEDM                             # 11- iHeartRadio IHR_IEDM" ) ;
  preferences.putString ( "preset_12",   "ihr/IHR_TRAN                             # 12 - iHeartRadio IHR_TRAN" ) ;
  //
  preferences.putString ( "clk_server",  "pool.ntp.org" ) ;        // Setting for Time Of Day clock on TFT
  preferences.putString ( "clk_offset",  "2" ) ;
  preferences.putString ( "clk_dst",     "1" ) ;
  //
  preferences.putString ( "pin_ir",      "35                                     # GPIO Pin number for IR receiver VS1838B" ) ;
  preferences.putString ( "ir_40BF",     "upvolume = 2" ) ;
  preferences.putString ( "ir_C03F",     "downvolume = 2" ) ;
  //
  preferences.putString ( "pin_vs_cs",   "5                                      # GPIO Pin number for VS1053 CS" ) ;
  preferences.putString ( "pin_vs_dcs",  "16                                     # GPIO Pin number for VS1053 DCS" ) ;
  preferences.putString ( "pin_vs_dreq", "4                                      # GPIO Pin number for VS1053 DREQ" ) ;
  //
  preferences.putString ( "pin_enc_clk", "25                                     # GPIO Pin number for rotary encoder CLK" ) ;
  preferences.putString ( "pin_enc_dt",  "26                                     # GPIO Pin number for rotary encoder DT" ) ;
  preferences.putString ( "pin_enc_sw",  "27                                     # GPIO Pin number for rotary encoder SW" ) ;
  //
  preferences.putString ( "pin_tft_cs",  "15                                     # GPIO Pin number for TFT CS" ) ;
  preferences.putString ( "pin_tft_dc",  "2                                      # GPIO Pin number for TFT DC" ) ;
  //
  preferences.putString ( "pin_sd_cs",   "21                                     # GPIO Pin number for SD card CS" ) ;


  // my adding - for my sotary_switch
  preferences.putString ( "pin_rtry_sw",  "34                                    # Rotary switch for station switch" ) ;
  
  preferences.end() ;
  delay ( 1000 ) ;
}


//******************************************************************************************
//                              L O O P                                                    *
//******************************************************************************************
void loop()
{
  Serial.println ( "ESP32_radio_init completed..." ) ;
  delay ( 10000 ) ;
}
