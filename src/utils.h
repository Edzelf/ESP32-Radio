// utils.h
// Some useful utilities.
//
#ifndef UTILS_H
#define UTILS_H
#include <esp_wifi_types.h>

#define DEBUG_BUFFER_SIZE 200                                      // Debug buffer size

void        dbg_set ( bool onoff ) ;                               // Turn on/off debuglines
void        dbgprint ( const char* format, ... ) ;                 // Print a formatted debug line
char        utf8ascii ( char ascii ) ;                             // Convert UTF to Ascii
void        utf8ascii_ip ( char* s ) ;                             // Convert UTF to Ascii in place
String      utf8ascii ( const char* s ) ;                          // Convert UTF to Ascii as String
const char* getEncryptionType ( wifi_auth_mode_t thisType ) ;      // Get encryption type voor WiFi networks
String      getContentType ( String filename ) ;


#endif
