# ESP32-Radio
Internet radio based on ESP32, VS1053 and a TFT screen.  Will compile in Arduino IDE.
The 28-jul-2017 is a major update.  You MUST include the GPIO pin assignments in the preferences.
See the documentation and/or the defaultsettings in the config page of the webinterface.

Features:
-	Can connect to thousands of Internet radio stations that broadcast MP3 or Ogg audio streams.
- Can connect to a standalone mp3 file on a server.
- Support for .m3u playlists.
- Can play mp3 tracks from SD card.
-	Uses a minimal number of components; no Arduino required.
-	Handles bitrates up to 320 kbps.
-	Has a preset list of maximal 100 favorite radio stations in configuration file.
- Configuration (preferences) can be edited through web interface.
-	Can be controlled by a tablet or other device through a build-in webserver.
- Can be controlled over MQTT.
- Can be controlled over Serial Input.
- Can be controlled by IR.
-	Can be controlled by rotary switch encoder.
- Can be controlled by touch pins.
-	Up to 14 free input pins can be configured to control the radio.
-	The strongest available WiFi network is automatically selected.
-	Heavily commented source code, easy to add extra functionality.
-	Debug information through serial output.
-	20 kB ring buffer to provide smooth playback.
-	Software update over WiFi possible (OTA).
-	Bass and treble control.
-	Saves volume, bass, treble and preset station over restart.
- Configuration also possible if no WiFi connection can be established.
- Can play iHeartRadio stations.
- Displays time of day on TFT.

See documentation in doc/pdf-file.

Last changes:
- 30-aug-2017: Limit number of retries foor MQTT connection.
- 28-aug-2017: Preferences for SPI bus, touch pins.
               Corrected bug in handling programmable pins.
               Handling of http redirections.
- 28-jul-2017: Added rotary swich encoder, flexible GPIO assignment.
- 19-jul-2017: Minor corrections.
- 18-jul-2017: Show time of day on TFT.
- 04-jul-2017: Correction MQTT subscription, keep playing during long oprerations.
- 03-jul-2017: Webinterface control page shows current settings.
- 30-jun-2017: Improved MP3 player.
- 28-jun-2017: Added IR interface.
- 31-may-2017: Experimental: play MP3 tracks from SD card.
- 26-may-2017: Correction Upper/Lower Case compare.
- 26-may-2017: Allow connection from single hidden AP.
- 23-may-2017: First release, derived from ESP8266 version.
