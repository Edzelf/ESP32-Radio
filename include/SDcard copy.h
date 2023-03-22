// SDcard.h
// Includes for SD card interface
//
#if ESP_ARDUINO_VERSION_MAJOR < 2                     // File function "path()" not available in older versions
  #define path() name()                               // Use "name()" instead
#endif
#define MAXFNLEN    512                               // Max length of a full filespec
#define MAXSPACE    30000                             // Max space for filenames (bytes, not tracks).
                                                      // Approx. 36 tracks per kB
#define SD_MAXDEPTH 4                                 // Maximum depths.  Note: see mp3play_html.

struct mp3specbuf_t
{
  char mp3specbuf[MAXSPACE] ;                         // Space for all MP3 filenames on SD card
} ;

struct mp3spec_t                                      // For List of mp3 file on SD card
{
  uint16_t prevlen ;                                  // Number of chars that are equal to previous
  char     filespec[MAXFNLEN] ;                       // Full file spec, the divergent end part
} ;


#ifndef SDCARD
  #define mount_SDCARD()         false                   // Dummy mount
  #define scan_SDCARD()                                  // Dummy scan files
  //#define check_SDCARD()                               // Dummy check
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
  #define SDSPEED 4000000                               // SPI speed of SD card

  int         SD_filecount = 0 ;                        // Number of filenames in SD_nodelist
  int         SD_curindex ;                             // Current index in mp3names
  mp3spec_t*  mp3spec ;                                 // Pointer to next mp3spec
  char        SD_lastmp3spec[MAXFNLEN] ;                // Previous full file spec
  char*       mp3names = nullptr ;                      // Filenames on SD
  char*       fillptr = nullptr ;                       // Fill pointer in mp3names
  File        mp3file ;                                 // File containing mp3 on SD card
  int         mp3filelength ;                           // Length of file
  int         spaceLeft ;                               // Space left in mp3names
  bool        randomplay = false ;                      // Switch for random play

  // Forward declaration
  void        setdatamode ( datamode_t newmode ) ;

  //**************************************************************************************************
  //                               R E W I N D S D D I R E C T O R Y                                 *
  //**************************************************************************************************
  // Initialization for getnextSDFileName().                                                         *
  //**************************************************************************************************
  void rewindSDdirectory()
  {

  }


  //**************************************************************************************************
  //                               G E T F I R S T S D F I L E N A M E                               *
  //**************************************************************************************************
  // Get the first filespec from mp3names.                                                           *
  //**************************************************************************************************
  char* getFirstSDFileName()
  {
    mp3spec = (mp3spec_t*)mp3names ;                    // Set pointer to begin of list
    strcpy ( SD_lastmp3spec, mp3spec->filespec ) ;      // Copy filename into SD_last
    SD_curindex = 0 ;                                   // Set current index to 0
    return SD_lastmp3spec ;                             // Return pointer to filename
  }


  //**************************************************************************************************
  //                                   G E T S D F I L E N A M E                                     *
  //**************************************************************************************************
  // Get a filespec from mp3names at index.                                                          *
  // If index is negative, a random track is selected.                                               *
  //**************************************************************************************************
  char* getSDFileName ( int inx )
  {
    int      entrysize ;                                // Size of entry including string delimeter
    char*    p ;                                        // For pointer manipulation

    if ( inx < 0 )                                      // Negative track number?
    {
      inx = (int) random ( SD_filecount ) ;             // Yes, pick random track
      randomplay = true ;                               // Set random play flag
    }
    else
    {
      randomplay = false ;                              // Not random, reset flag
    }
    if ( inx >= SD_filecount )                          // Protect against going beyond last track
    {
      inx = 0 ;                                         // Beyond last track: rewind to begin
    }
    if ( inx < SD_curindex )                            // Going backwards?
    {
      getFirstSDFileName() ;                            // Yes, start all over
    }
    while ( SD_curindex < inx )                         // Move forward until at required position
    {
      entrysize = sizeof(mp3spec->prevlen) +            // Size of entry including string delimeter
                  strlen ( mp3spec->filespec )  + 1 ;
      p = (char*)mp3spec + entrysize ;
      mp3spec = (mp3spec_t*)p ;                         // Set pointer to begin of next entry
      strcpy ( SD_lastmp3spec + mp3spec->prevlen,       // Copy filename intp SD_last
              mp3spec->filespec ) ;
      SD_curindex++ ;                                   // Set current index
    }
    return SD_lastmp3spec ;                             // Return pointer to filename
  }


  //**************************************************************************************************
  //                               G E T N E X T S D F I L E N A M E                                 *
  //**************************************************************************************************
  // Get next filespec from mp3names.                                                                *
  //**************************************************************************************************
  char* getNextSDFileName()
  {
    int inx = SD_curindex + 1 ;                          // By default next track

    if ( randomplay )                                    // Playing random tracks?
    {
      inx = -1 ;                                         // Yes, next track will be random too
    }
    return ( getSDFileName ( inx ) ) ;                   // Select the track
  }


  //**************************************************************************************************
  //                           G E T C U R R E N T S D F I L E N A M E                               *
  //**************************************************************************************************
  // Get current filespec from mp3names.                                                             *
  //**************************************************************************************************
  char* getCurrentSDFileName()
  {
    return SD_lastmp3spec ;                             // Return pointer to filename
  }


  //**************************************************************************************************
  //                        G E T C U R R E N T S D T R A C K N U M B E R                            *
  //**************************************************************************************************
  // Get current index in list of filespecs.                                                         *
  //**************************************************************************************************
  int getCurrentSDTrackNumber()
  {
    return SD_curindex ;                               // Return current track number
  }


  //**************************************************************************************************
  //                      G E T C U R R E N T S H O R T S D F I L E N A M E                          *
  //**************************************************************************************************
  // Get last part of current filespec from mp3names.                                                *
  //**************************************************************************************************
  char* getCurrentShortSDFileName()
  {
    return strrchr ( SD_lastmp3spec, '/' ) + 1 ;        // Last part of filespec
  }


  //**************************************************************************************************
  //                                  C L E A R F I L E L I S T                                      *
  //**************************************************************************************************
  // Clear the list with full filespecs.                                                             *
  //**************************************************************************************************
  bool clearFileList()
  {
    SD_lastmp3spec[0] = '\0' ;                        // Last filename is now unknown
    SD_filecount = 0 ;                                // Reset counter
    spaceLeft = MAXSPACE ;                            // Set total space left
    if ( mp3names == nullptr )                        // Already initialized?
    {
      mp3names = (char*)new ( mp3specbuf_t ) ;        // Space for mp3 filenames on SD card
    }
    if ( mp3names == nullptr )
    {
      dbgprint ( "No space for SD buffer!" ) ;
      spaceLeft = 0 ;                                 // Set space left in buffer to 0
    }
    fillptr = mp3names ;                              // Start filling here
    return ( mp3names != nullptr ) ;                  // Return result
  }


  //**************************************************************************************************
  //                                  A D D T O F I L E L I S T                                      *
  //**************************************************************************************************
  // Add a filename to the file listpec.                                                             *
  // Example:                                                                                        *
  // Entry prevlen   filespec                             Interpreted result                         *
  // ----- -------   --------------------------------     ------------------------------------       *
  // [0]         0   /Fleetwood Mac/Albatross.mp3         /Fleetwood Mac/Albatross.mp3               *
  // [1]        15   Hold Me.mp3                          /Fleetwood Mac/Hold Me.mp3                 *
  //**************************************************************************************************
  bool addToFileList ( const char* newfnam )
  {
    static mp3spec_t  entry ;                           // New entry to add to list (too big for stack)
    char*             p = SD_lastmp3spec ;              // Set pointer to compare
    uint16_t          n = 0 ;                           // Counter for number of equal characters
    uint16_t          l = strlen ( newfnam ) ;          // Length of new file name
    int               entrysize ;                       // Size of entry
    bool              res = false ;                     // Function result

    if ( l >= sizeof ( SD_lastmp3spec ) )               // Block very long filenames
    {
      dbgprint ( "SD filename too long (%d)!", l ) ;
      return res ;                                      // Filename too long: skip
    }
    while ( *p == *newfnam )                            // Compare next character of filename
    {
      if ( *p == '\0' )                                 // End of name?
      {
        break ;                                         // Yes, stop
      }
      n++ ;                                             // Equal: count
      p++ ;                                             // Update pointers
      newfnam++ ;
    }
    entry.prevlen = n ;                                 // This part is equal to previous name
    strcpy ( entry.filespec, newfnam ) ;                // This is last part of new filename
    entrysize = sizeof(entry.prevlen) +                 // Size of entry including string delimeter
                strlen (newfnam) + 1 ;
    //dbgprint ( "Entrysize is %d", entrysize ) ;
    res = ( ( fillptr != nullptr ) &&
          ( entrysize <= spaceLeft ) ) ;                // Space for this entry?
    if ( res )
    {
      strcpy ( p, newfnam ) ;                           // Set a new lastmp3spec
      dbgprint ( "Added %3d : %s", n,                   // Show last part of filename
                 getCurrentShortSDFileName() ) ;
      memcpy ( fillptr, &entry, entrysize ) ;           // Yes, add to list
      spaceLeft -= entrysize ;                          // Adjust space left
      fillptr = fillptr + entrysize ;                   // Update pointer
      SD_filecount++ ;                                  // Count number of files in list
    }
    else
    {
      dbgprint ( "No space for %s, %d > %d",
                newfnam, entrysize, spaceLeft ) ;
    }
    return res ;                                        // Return result of adding name
  }


  //**************************************************************************************************
  //                                      G E T S D T R A C K S                                      *
  //**************************************************************************************************
  // Search all MP3 files on directory of SD card.                                                   *
  // Will be called recursively.                                                                     *
  //**************************************************************************************************
  void getsdtracks ( const char * dirname, uint8_t levels )
  {
    File       root ;                                     // Work directory
    File       file ;                                     // File in work directory

    //dbgprint ( "getsdt dir is %s", dirname ) ;
    claimSPI ( "sdopen1" ) ;                              // Claim SPI bus
    root = SD.open ( dirname ) ;                          // Open directory
    releaseSPI() ;                                        // Release SPI bus
    if ( !root )                                          // Check on open
    {
      dbgprint ( "Failed to open directory" ) ;
      return ;
    }
    if ( !root.isDirectory() )                            // Really a directory?
    {
      dbgprint ( "Not a directory" ) ;
      return ;
    }
    claimSPI ( "sdopen2" ) ;                              // Claim SPI bus
    file = root.openNextFile() ;
    releaseSPI() ;                                        // Release SPI bus
    while ( file )
    {
      if ( file.isDirectory() )                           // Is it a directory?
      {
        //dbgprint ( "  DIR : %s", file.path() ) ;
        if ( levels )                                     // Dig in subdirectory?
        {
          if ( strrchr ( file.path(), '/' )[1] != '.' )   // Skip hidden directories
          {
            getsdtracks ( file.path(), levels -1 ) ;      // Non hidden directory: call recursive
          }
        }
      }
      else                                                // It is a file
      {
        const char* ext = file.name() ;                   // Point to begin of name
        ext = ext + strlen ( ext ) - 4 ;                  // Point to extension
        if ( ( strcmp ( ext, ".MP3" ) == 0 ) ||           // It is a file, but is it an MP3?
            ( strcmp ( ext, ".mp3" ) == 0 ) )
        {
          if ( ! addToFileList ( file.path() ) )          // Add file to the list
          {
            break ;                                       // No need to continue
          }
        }
      }
      claimSPI ( "sdopen3" ) ;                            // Claim SPI bus
      file = root.openNextFile() ;
      releaseSPI() ;                                      // Release SPI bus
    }
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
          sttg -= mp3file.read ( tmpbuf, 4 ) ;                // Yes, ignore 4 bytes
          stg -= 4 ;                                          // Reduce tag size
        }
        if ( ID3tag.tagflags[1] & 0x044 )                     // Encrypted or grouped?
        {
          sttg -= mp3file.read ( tmpbuf, 1 ) ;                // Yes, ignore 1 byte
          stg-- ;                                             // Reduce tagsize by 1
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
          #ifdef T_NEXTION                                    // NEXTION display?
            albttl += String ( "\\r" ) ;                      // Add code for newline (2 characters)
          #else
            albttl += String ( "\n" ) ;                       // Add newline (1 character)
          #endif
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
    mp3file.seek ( 0 ) ;                                      // Back to begin of filr
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
    tftset ( 0, "MP3 Player" ) ;                            // Set screen segment top line
    displaytime ( "" ) ;                                    // Clear time on TFT screen
    setdatamode ( DATA ) ;                                  // Start in datamode 
    path = String ( getCurrentSDFileName() ) ;              // Set path to file to play
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
  //                                       M O U N T _ S D C A R D                                   *
  //**************************************************************************************************
  // Initialize the SD card.                                                                         *
  //**************************************************************************************************
  bool mount_SDCARD ( int8_t csPin )
  {
    bool       SD_okay = false ;                           // True if SD card in place and readable

    if ( csPin >= 0 )                                      // SD configured?
    {
      if ( !SD.begin ( csPin, SPI, SDSPEED ) )             // Yes, try to init SD card driver
      {
        dbgprint ( "SD Card Mount Failed!" ) ;             // No success, check formatting (FAT)
      }
      else
      {
        SD_okay = ( SD.cardType() != CARD_NONE ) ;         // See if known card
        if ( !SD_okay )
        {
          dbgprint ( "No SD card attached" ) ;             // Card not readable
        }
      }
    }
    return SD_okay ;
  }


  //**************************************************************************************************
  //                                       S C A N _ S D C A R D                                     *
  //**************************************************************************************************
  // Scan the SD card for mp3-files.                                                                 *
  //**************************************************************************************************
  void scan_SDCARD()
  {
    clearFileList() ;                                     // Create list with names and count
    dbgprint ( "Locate mp3 files on SD, "
              "may take a while..." ) ;
    getsdtracks ( "/", SD_MAXDEPTH ) ;                    // Build file list
    dbgprint ( "Space %d/%d", heapspace,                  // Largest block/total
               ESP.getFreeHeap() ) ;
    dbgprint ( "%d tracks on SD", SD_filecount ) ;        // Show number of files
    getFirstSDFileName() ;                                // Point to first entry
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
  size_t read_SDCARD ( uint8_t* buf, uint32_t len )
  {
    return mp3file.read ( buf, len ) ;                   // Read a block of data
  }
#endif
