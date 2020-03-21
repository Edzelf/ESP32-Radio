rem Run this before update Git!!!
rem remove ino file, when build in platformio
@echo off
echo ---Prepare .ino file from .cpp for easy work with Arduino
echo ---PlatformIO (intellisense) does not like .ino files :-(
del Esp32_radio\Esp32_radio.ino
copy Esp32_radio\Esp32_radio.cpp Esp32_radio\Esp32_radio.ino
