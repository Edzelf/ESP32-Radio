# ESP32-Radio
Internet radio based on ESP32, VS1053 and a TFT screen.  Will compile in Arduino IDE.

Features:
-	Can connect to thousands of Internet radio stations that broadcast MP3 or Ogg audio streams.
- Can connect to a standalone mp3 file on a server.
- Support for .m3u playlists.
-	Uses a minimal number of components; no Arduino required.
-	Handles bitrates up to 320 kbps.
-	Has a preset list of maximal 100 favorite radio stations in configuration file.
- Configuration (preferences) can be edited through web interface.
-	Can be controlled by a tablet or other device through a build-in webserver.
- Can be controlled over MQTT.
- Can be controlled over Serial Input.
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

See documentation in doc/pdf-file.

Last changes:
- 26-may-2017: Correctio Upper/Lower Case compare.
- 26-may-2017: Allow connection from single hidden AP.
- 23-may-2017: First release, derived from ESP8266 version.


