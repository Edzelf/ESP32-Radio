// Preferences test
#include <Preferences.h>
Preferences preferences ;

// Number of keys to test
#define NKEY 30

void setup()
{
  int i ;
  char key[20] ;
  
  Serial.begin(115200);
  Serial.println();

  preferences.begin ( "ESP32Radio", false ) ;
  preferences.clear() ;
  if ( true )                                               // Change to false for successive runs
  {
    Serial.println ( "Fill NVS" ) ;
    for ( i = 0 ; i < NKEY ; i++ )                          // Fill with a number of keys
    {
      sprintf ( key, "TKEY_%02d", i ) ;                     // Form TKEY_00 .. TKEY_nn
      preferences.putString ( key, "<someting>" ) ;         // Add the key and contents
    }
  }
  // Close and reopen the Preferences
  preferences.end();
  preferences.begin ( "ESP32Radio", true ) ;                // Readonly
}


//******************************************************************************************
//                              R E A D P R E F S                                          *
//******************************************************************************************
// Show all keys/content in nvs.                                                           *
//******************************************************************************************
void readprefs()
{
  int         i ;
  char        key[20] ;
  String      val ;                                         // Contents of preference entry
  int         count = 0 ;                                   // Number of keys found
  
  for ( i = 0 ; i < NKEY ; i++ )
  {
    sprintf ( key, "TKEY_%02d", i ) ;                       // Form TKEY_00 .. TKEY_29
    val = preferences.getString ( key ) ;
    if ( val.length() )                                     // Key exists?
    {
      count++ ;                                             // Count number of keys
      Serial.printf ( "%s = %s\n",
                      key, val.c_str() ) ;                  // Show
    }
  }
  if ( count < NKEY )
  {
    Serial.printf ( "Not enough keys found!  Found %d, should be %d\n",
                     count, NKEY ) ;
  }
}



void loop()
{
  static int ln = 0 ;
  
  Serial.printf ( "Loop number %d\n", ++ln ) ;
  readprefs() ;
  Serial.println() ;
  delay ( 5000 ) ;
}