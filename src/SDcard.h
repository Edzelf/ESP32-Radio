// SDcard.h
// Includes for SD card interface
//

String            SD_nodelist ;                          // Nodes of mp3-files on SD
bool              SD_okay = false ;                      // True if SD card in place and readable
int               SD_nodecount = 0 ;                     // Number of nodes in SD_nodelist
String            SD_currentnode = "" ;                  // Node ID of song playing ("0" if random)

#ifndef SDCARD
  #define setup_SDCARD()                                 // Dummy initialize
  //#define check_SDCARD()                                 // Dummy check
  #define close_SDCARD()                                 // Dummy close
  #define read_SDCARD(a,b)       0                       // Dummy read file buffer
  #define selectnextSDnode(b)    String("")
  #define getSDfilename(a)       String("")
  #define listsdtracks(a,b,c)    0
  #define connecttofile_SD()      false                  // Dummy connect to file
#else
#include <SPI.h>
#include <SD.h>
#include <FS.h>
File        mp3file ;                              // File containing mp3 on SD card

// forward declaration
void        setdatamode ( datamode_t newmode ) ;

//**************************************************************************************************
//                                  S E L E C T N E X T S D N O D E                                *
//**************************************************************************************************
// Select the next or previous mp3 file from SD.  If the last selected song was random, the next   *
// track is a random one too.  Otherwise the next/previous node is choosen.                        *
// If nodeID is "0" choose a random nodeID.                                                        *
// Delta is +1 or -1 for next or previous track.                                                   *
// The nodeID will be returned to the caller.                                                      *
//**************************************************************************************************
String selectnextSDnode ( int16_t delta )
{
  int16_t        inx, inx2 ;                           // Position in nodelist

  if ( hostreq )                                       // Host request already set?
  {
    return "" ;                                        // Yes, no action
  }
  dbgprint ( "SD_currentnode is %s, "
             "delta is %d",
             SD_currentnode.c_str(),
             delta ) ;
  if ( SD_currentnode == "0" )                         // Random playing?
  {
    return SD_currentnode ;                            // Yes, return random nodeID
  }
  inx = SD_nodelist.indexOf ( SD_currentnode ) ;       // Get position of current nodeID in list
  if ( delta > 0 )                                     // Next track?
  {
    inx += SD_currentnode.length() + 1 ;               // Get position of next nodeID in list
    if ( inx >= SD_nodelist.length() )                 // End of list?
    {
      inx = 0 ;                                        // Yes, wrap around
    }
  }
  else
  {
    if ( inx == 0 )                                    // At the begin of the list?
    {
      inx = SD_nodelist.length()  ;                    // Yes, goto end of list
    }
    inx-- ;                                            // Index of delimeter of previous node ID
    while ( ( inx > 0 ) &&
            ( SD_nodelist[inx - 1] != '\n' ) )
    {
      inx-- ;
    }
  }
  inx2 = SD_nodelist.indexOf ( "\n", inx ) ;              // Find end of node ID
  SD_currentnode = SD_nodelist.substring ( inx, inx2 ) ;  // Return nodeID
  return SD_currentnode ;                                 // Return nodeID
}


//**************************************************************************************************
//                                      G E T S D F I L E N A M E                                  *
//**************************************************************************************************
// Translate the nodeID of a track to the full filename that can be used as a station.             *
// If nodeID is "0" choose a random nodeID.                                                        *
//**************************************************************************************************
String getSDfilename ( String &nodeID )
{
  String          res ;                                    // Function result
  File            root, file ;                             // Handle to root and directory entry
  uint16_t        n, i ;                                   // Current seqnr and counter in directory
  int16_t         inx ;                                    // Position in nodeID
  const char*     p = "/" ;                                // Points to directory/file
  uint16_t        rndnum ;                                 // Random index in SD_nodelist
  int             nodeinx = 0 ;                            // Points to node ID in SD_nodecount
  int             nodeinx2 ;                               // Points to end of node ID in SD_nodecount

  SD_currentnode = nodeID ;                                // Save current node
  if ( nodeID == "0" )                                     // Empty parameter?
  {
    rndnum = random ( SD_nodecount ) ;                     // Yes, choose a random node
    for ( i = 0 ; i < rndnum ; i++ )                       // Find the node ID
    {
      // Search to begin of the random node by skipping lines
      nodeinx = SD_nodelist.indexOf ( "\n", nodeinx ) + 1 ;
    }
    nodeinx2 = SD_nodelist.indexOf ( "\n", nodeinx ) ;     // Find end of node ID
    nodeID = SD_nodelist.substring ( nodeinx,
                                     nodeinx2 ) ;          // Get node ID
  }
  dbgprint ( "getSDfilename requested node ID is %s",      // Show requeste node ID
             nodeID.c_str() ) ;
  while ( ( n = nodeID.toInt() ) > 0 )                     // Next sequence in current level
  {
    claimSPI ( "sdopen" ) ;                                // Claim SPI bus
    root = SD.open ( p ) ;                                 // Open the directory (this level)
    for ( i = 1 ; i <=  n ; i++ )
    {
      file = root.openNextFile() ;                         // Get next directory entry
    }
    releaseSPI() ;                                         // Release SPI bus
    p = file.name() ;                                      // Points to directory- or file name
    inx = nodeID.indexOf ( "," ) ;                         // Find position of comma
    if ( inx >= 0 )
    {
      nodeID = nodeID.substring ( inx + 1 ) ;              // Remove sequence in this level from nodeID
    }
    else
    {
      break ;                                              // All levels handled
    }
  }
  res = String ( "localhost" ) + String ( p ) ;            // Format result
  dbgprint ( "Selected file is %s", res.c_str() ) ;        // Show result
  return res ;                                             // Return full station spec
}


//**************************************************************************************************
//                                      L I S T S D T R A C K S                                    *
//**************************************************************************************************
// Search all MP3 files on directory of SD card.  Return the number of files found.                *
// A "node" of max. 4 levels ( the subdirectory level) will be generated for every file.           *
// The numbers within the node-array is the sequence number of the file/directory in that          *
// subdirectory.                                                                                   *
// A node ID is a string like "2,1,4,0", which means the 4th file in the first sub-directory       *
// of the second directory.                                                                        *
// The list will be send to the webinterface if parameter "send" is true.                          *
//**************************************************************************************************
int listsdtracks ( const char* dirname, int level = 0, bool send = true )
{
  const  uint16_t SD_MAXDEPTH = 4 ;                     // Maximum depts.  Note: see mp3play_html.
  static uint16_t fcount, oldfcount ;                   // Total number of files
  static uint16_t SD_node[SD_MAXDEPTH + 1] ;            // Node ISs, max levels deep
  static String   SD_outbuf ;                           // Output buffer for cmdclient
  uint16_t        ldirname ;                            // Length of dirname to remove from filename
  File            root, file ;                          // Handle to root and directory entry
  String          filename ;                            // Copy of filename for lowercase test
  uint16_t        i ;                                   // Loop control to compute single node id
  String          tmpstr ;                              // Tijdelijke opslag node ID
  const char*     p ;                                   // Points to filename in directory

  if ( strcmp ( dirname, "/" ) == 0 )                   // Are we at the root directory?
  {
    fcount = 0 ;                                        // Yes, reset count
    memset ( SD_node, 0, sizeof(SD_node) ) ;            // And sequence counters
    SD_outbuf = String() ;                              // And output buffer
    SD_nodelist = String() ;                            // And nodelist
    if ( !SD_okay )                                     // See if known card
    {
      if ( send )
      {
        cmdclient.println ( "0/No tracks found" ) ;     // No SD card, emppty list
      }
      return 0 ;
    }
  }
  oldfcount = fcount ;                                  // To see if files found in this directory
  dbgprint ( "SD directory is %s", dirname ) ;          // Show current directory
  ldirname = strlen ( dirname ) ;                       // Length of dirname to remove from filename
  claimSPI ( "sdopen2" ) ;                              // Claim SPI bus
  root = SD.open ( dirname ) ;                          // Open the current directory level
  releaseSPI() ;                                        // Release SPI bus
  if ( !root || !root.isDirectory() )                   // Success?
  {
    dbgprint ( "%s is not a directory or not root",     // No, print debug message
               dirname ) ;
    return fcount ;                                     // and return
  }
  while ( true )                                        // Find all mp3 files
  {
    claimSPI ( "opennextf" ) ;                          // Claim SPI bus
    file = root.openNextFile() ;                        // Try to open next
    releaseSPI() ;                                      // Release SPI bus
    if ( !file )
    {
      break ;                                           // End of list
    }
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
          p = file.name() + ldirname ;                  // Point to filename passed directoryname
          SD_outbuf += tmpstr +                         // Form line for mp3play_html page
                       utf8ascii ( p ) ;                // Filename
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
      else if ( filename.endsWith ( ".m3u" ) )          // Is it an .m3u file?
      {
        dbgprint ( "Playlist %s", file.name() ) ;       // Yes, show
      }
    }
    if ( send )
    {
      mp3loop() ;                                       // Keep playing
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


//**************************************************************************************************
//                                  H A N D L E _ I D 3 _ S D                                      *
//**************************************************************************************************
// Check file on SD card for ID3 tags and use them to display some info.                           *
// Extended headers are not parsed.                                                                *
//**************************************************************************************************
void handle_ID3_SD ( String &path )
{
  char*  p ;                                                // Pointer to filename
  struct ID3head_t                                          // First part of ID3 info
  {
    char    fid[3] ;                                        // Should be filled with "ID3"
    uint8_t majV, minV ;                                    // Major and minor version
    uint8_t hflags ;                                        // Headerflags
    uint8_t ttagsize[4] ;                                   // Total tag size
  } ID3head ;
  uint8_t  exthsiz[4] ;                                     // Extended header size
  uint32_t stx ;                                            // Ext header size converted
  uint32_t sttg ;                                           // Total tagsize converted
  uint32_t stg ;                                            // Size of a single tag
  struct ID3tag_t                                           // Tag in ID3 info
  {
    char    tagid[4] ;                                      // Things like "TCON", "TYER", ...
    uint8_t tagsize[4] ;                                    // Size of the tag
    uint8_t tagflags[2] ;                                   // Tag flags
  } ID3tag ;
  uint8_t  tmpbuf[4] ;                                      // Scratch buffer
  uint8_t  tenc ;                                           // Text encoding
  String   albttl = String() ;                              // Album and title
  bool      talb ;                                          // Tag is TALB (album title)
  bool      tpe1 ;                                          // Tag is TPE1 (artist)

  tftset ( 2, "Playing from local file" ) ;                 // Assume no ID3
  p = (char*)path.c_str() + 1 ;                             // Point to filename (after the slash)
  showstreamtitle ( p, true ) ;                             // Show the filename as title (middle part)
  mp3file = SD.open ( path ) ;                              // Open the file
  if ( path.endsWith ( ".mu3" ) )                           // Is it a playlist?
  {
    return ;                                                // Yes, no ID's, but leave file open
  }
  mp3file.read ( (uint8_t*)&ID3head, sizeof(ID3head) ) ;    // Read first part of ID3 info
  if ( strncmp ( ID3head.fid, "ID3", 3 ) == 0 )
  {
    sttg = ssconv ( ID3head.ttagsize ) ;                    // Convert tagsize
    dbgprint ( "Found ID3 info" ) ;
    if ( ID3head.hflags & 0x40 )                            // Extended header?
    {
      stx = ssconv ( exthsiz ) ;                            // Yes, get size of extended header
      while ( stx-- )
      {
        mp3file.read () ;                                   // Skip next byte of extended header
      }
    }
    while ( sttg > 10 )                                     // Now handle the tags
    {
      sttg -= mp3file.read ( (uint8_t*)&ID3tag,
                             sizeof(ID3tag) ) ;             // Read first part of a tag
      if ( ID3tag.tagid[0] == 0 )                           // Reached the end of the list?
      {
        break ;                                             // Yes, quit the loop
      }
      stg = ssconv ( ID3tag.tagsize ) ;                     // Convert size of tag
      if ( ID3tag.tagflags[1] & 0x08 )                      // Compressed?
      {
        sttg -= mp3file.read ( tmpbuf, 4 ) ;               // Yes, ignore 4 bytes
        stg -= 4 ;                                         // Reduce tag size
      }
      if ( ID3tag.tagflags[1] & 0x044 )                     // Encrypted or grouped?
      {
        sttg -= mp3file.read ( tmpbuf, 1 ) ;               // Yes, ignore 1 byte
        stg-- ;                                            // Reduce tagsize by 1
      }
      if ( stg > ( sizeof(metalinebf) + 2 ) )               // Room for tag?
      {
        break ;                                             // No, skip this and further tags
      }
      sttg -= mp3file.read ( (uint8_t*)metalinebf,
                             stg ) ;                        // Read tag contents
      metalinebf[stg] = '\0' ;                              // Add delimeter
      tenc = metalinebf[0] ;                                // First byte is encoding type
      if ( tenc == '\0' )                                   // Debug all tags with encoding 0
      {
        dbgprint ( "ID3 %s = %s", ID3tag.tagid,
                   metalinebf + 1 ) ;
      }
      talb = ( strncmp ( ID3tag.tagid, "TALB", 4 ) == 0 ) ; // Album title
      tpe1 = ( strncmp ( ID3tag.tagid, "TPE1", 4 ) == 0 ) ; // Artist?
      if ( talb || tpe1 )                                   // Album title or artist?
      {
        albttl += String ( metalinebf + 1 ) ;               // Yes, add to string
        if ( displaytype == T_NEXTION )                     // NEXTION display?
        {
          albttl += String ( "\\r" ) ;                      // Add code for newline (2 characters)
        }
        else
        {
          albttl += String ( "\n" ) ;                       // Add newline (1 character)
        }
        if ( tpe1 )                                         // Artist tag?
        {
          icyname = String ( metalinebf + 1 ) ;             // Yes, save for status in webinterface
        }
      }
      if ( strncmp ( ID3tag.tagid, "TIT2", 4 ) == 0 )       // Songtitle?
      {
        tftset ( 2, metalinebf + 1 ) ;                      // Yes, show title
        icystreamtitle = String ( metalinebf + 1 ) ;        // For status in webinterface
      }
    }
    tftset ( 1, albttl ) ;                                  // Show album and title
  }
  mp3file.close() ;                                         // Close the file
  mp3file = SD.open ( path ) ;                              // And open the file again
}


//**************************************************************************************************
//                                  C O N N E C T T O F I L E _ S D                                *
//**************************************************************************************************
// Open the local mp3-file.                                                                        *
//**************************************************************************************************
bool connecttofile_SD()
{
  String path ;                                           // Full file spec

  stop_mp3client() ;                                      // Disconnect if still connected
  tftset ( 0, "ESP32 MP3 Player" ) ;                      // Set screen segment top line
  displaytime ( "" ) ;                                    // Clear time on TFT screen
  if ( host.endsWith ( ".m3u" ) )                         // Is it an m3u playlist?
  {
    setdatamode ( DATA ) ;                                // Yes, start in PLAYLIST mode
    playlist = host ;                                     // Save copy of playlist URL
    if ( playlist_num == 0 )                              // First entry to play?
    {
      playlist_num = 1 ;                                  // Yes, set index
    }
    dbgprint ( "Playlist request, entry %d", playlist_num ) ;
  }
  else
  {
    setdatamode ( DATA ) ;                                // Start in datamode 
  }
  if ( mp3file )
  {
    while ( mp3file.available() )
    {
      mp3file.read() ;
    }
    dbgprint ( "Closed SD file" ) ;                       // TEST*TEST*TEST
    mp3file.close() ;                                     // Be sure to close current file
  }
  path = host.substring ( 9 ) ;                           // Path, skip the "localhost" part
  icystreamtitle = path ;                                 // If no ID3 available
  icyname = String ( "" ) ;                               // If no ID3 available
  claimSPI ( "sdopen3" ) ;                                // Claim SPI bus
  handle_ID3_SD ( path ) ;                                // See if there are ID3 tags in this file
  releaseSPI() ;                                          // Release SPI bus
  if ( !mp3file )
  {
    dbgprint ( "Error opening file %s", path.c_str() ) ;  // No luck
    return false ;
  }
  mp3filelength = mp3file.available() ;                   // Get length
  mqttpub.trigger ( MQTT_STREAMTITLE ) ;                  // Request publishing to MQTT
  chunked = false ;                                       // File not chunked
  metaint = 0 ;                                           // No metadata
  return true ;
}


//**************************************************************************************************
//                                       S E T U P _ S D C A R D                                   *
//**************************************************************************************************
// Initialize the SD card.                                                                         *
//**************************************************************************************************
void setup_SDCARD()
{
  char *p ;                                              // Last debug string
  
  if ( ini_block.sd_cs_pin >= 0 )                        // SD configured?
  {
    if ( !SD.begin ( ini_block.sd_cs_pin, SPI,           // Yes,
                     SDSPEED ) )                         // try to init SD card driver
    {
      p = dbgprint ( "SD Card Mount Failed!" ) ;         // No success, check formatting (FAT)
      tftlog ( p ) ;                                     // Show error on TFT as well
    }
    else
    {
      SD_okay = ( SD.cardType() != CARD_NONE ) ;         // See if known card
      usb_sd = FS_SD ;                                   // Set SD for MP3 player mode

      if ( !SD_okay )
      {
        p = dbgprint ( "No SD card attached" ) ;         // Card not readable
        tftlog ( p ) ;                                   // Show error on TFT as well
      }
      else
      {
        dbgprint ( "Locate mp3 files on SD, may take a while..." ) ;
        tftlog ( "Read SD card" ) ;
        SD_nodecount = listsdtracks ( "/", 0, false ) ;  // Build nodelist
        p = dbgprint ( "%d tracks on SD", SD_nodecount ) ;
        tftlog ( p ) ;                                   // Show number of tracks on TFT
      }
    }
  }
}



//**************************************************************************************************
//                                       C L O S E _ S D C A R D                                   *
//**************************************************************************************************
// Close file on SD card.                                                                          *
//**************************************************************************************************
void close_SDCARD()
{
  mp3file.close() ;
}


//**************************************************************************************************
//                                       R E A D _ S D C A R D                                     *
//**************************************************************************************************
// Read a block of data from SD card file.                                                         *
//**************************************************************************************************
int read_SDCARD ( uint8_t* buf, uint32_t len )
{
  return mp3file.read ( buf, len ) ;                   // Read a block of data
}


#endif
