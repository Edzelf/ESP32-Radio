// SDcard.h
// Includes for SD card interface
//
#if ESP_ARDUINO_VERSION_MAJOR < 2                         // File function "path()" not available in older versions
  #define path() name()                                   // Use "name()" instead
#endif
#define MAXFNLEN        256                               // Max length of a full filespec
#define RINGBUFFERSPACE 2560
#define SD_MAXDEPTH     4                                 // Maximum depths.  Note: see mp3play_html.

#ifndef SDCARD
  #define mount_SDCARD()         false                    // Dummy mount
  //#define check_SDCARD()                                // Dummy check
  #define close_SDCARD()                                  // Dummy close
  #define read_SDCARD(a,b)       0                        // Dummy read file buffer
  #define getSDfilename(a)       String("")
  #define connecttofile_SD()      false                   // Dummy connect to file
  #define initSDTask()                                    // Dummy start of task
#else
  #include <SPI.h>
  #include <SD.h>
  #include <FS.h>
  #include <freertos/ringbuf.h>
  #define SDSPEED 8000000                               // SPI speed of SD card

  RingbufHandle_t fnbuf ;                               // Ringbuffer to store filenames
  bool            SD_okay = false ;                     // SD is mounted
  char            SD_lastmp3spec[MAXFNLEN] ;            // Previous full file spec
  int             SD_filecount = 0 ;                    // Number of files on SD
  int             tracknum = 0 ;                        // Current track number
  File            mp3file ;                             // File containing mp3 on SD card
  int             mp3filelength ;                       // Length of file
  bool            randomplay = false ;                  // Switch for random play

  // Forward declaration
  void SDtask ( void * parameter ) ;
  void setdatamode ( datamode_t newmode ) ;
  bool mount_SDCARD ( int8_t csPin ) ;


  //**************************************************************************************************
  //                                   S E T S D F I L E N A M E                                     *
  //**************************************************************************************************
  // Set the current filespec.                                                                       *
  //**************************************************************************************************
  void setSDFileName ( const char* fnam )
  {
    if ( strlen ( fnam ) < MAXFNLEN )                   // Guard against long filenames
    {
      strcpy ( SD_lastmp3spec,  fnam ) ;                // Copy filename intp SD_last
    }
  }


  //**************************************************************************************************
  //                                   G E T S D F I L E N A M E                                     *
  //**************************************************************************************************
  // Get a filespec from mp3names at index.                                                          *
  // If index is negative, a random track is selected.                                               *
  //**************************************************************************************************
  const char* getSDFileName ( int inx )
  {
    size_t      fnlen ;                                   // Length of filename
    void*       fnam = NULL ;                             // Filename from ringbuffer

    if ( inx < 0 )                                        // Negative track number?
    {
      inx = (int) random ( SD_filecount ) ;               // Yes, pick random track
      randomplay = true ;                                 // Set random play flag
    }
    else
    {
      randomplay = false ;                                // Not random, reset flag
    }
    while ( inx-- )                                       // Move forward until at required position
    {
      fnam = xRingbufferReceive ( fnbuf, &fnlen, 500 ) ;  // Read next filename
      if ( fnam == nullptr )                              // Result?
      {
        break ;                                           // No, stop reading
      }
    }
    if ( fnam )                                           // Final result?
    {
      SD_filecount-- ;                                    // Yes, keep filecount up to date
      strcpy ( SD_lastmp3spec, (const char*)fnam ) ;      // Yes, copy filename into SD_last
      vRingbufferReturnItem ( fnbuf, fnam ) ;             // Return space to the buffer
      dbgprint ( "Filename from ringbuffer is %s",
                 SD_lastmp3spec ) ;
    }
    else
    {
      dbgprint ( "End of ringbuffer list" ) ;
      *SD_lastmp3spec = '\0' ;                            // Bad result is also end of list
    }
    return SD_lastmp3spec ;                               // Return pointer to filename
  }


  //**************************************************************************************************
  //                               G E T N E X T S D F I L E N A M E                                 *
  //**************************************************************************************************
  // Get next filespec from mp3names.                                                                *
  //**************************************************************************************************
  const char* getNextSDFileName()
  {
    int inx =  1 ;                                       // By default next track

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
  const char* getCurrentSDFileName()
  {
    return SD_lastmp3spec ;                             // Return pointer to filename
  }


  //**************************************************************************************************
  //                      G E T C U R R E N T S H O R T S D F I L E N A M E                          *
  //**************************************************************************************************
  // Get last part of current filespec from mp3names.                                                *
  //**************************************************************************************************
  const char* getCurrentShortSDFileName()
  {
    return strrchr ( SD_lastmp3spec, '/' ) + 1 ;        // Last part of filespec
  }


  //**************************************************************************************************
  //                                  A D D T O R I N G B U F                                        *
  //**************************************************************************************************
  // Add a filename to the ring buffer.                                                              *
  // If an error occurs, the filename is lost.                                                       *
  //**************************************************************************************************
  void addToRingbuf ( const char* newfnam )
  {
    size_t  fnlen = strlen ( newfnam ) + 1 ;                  // Length of filename plus delimeter
  
    if ( fnlen >= MAXFNLEN )                                  // Do not store very long filenames
    {
      dbgprint ( "Long filename ignored!" ) ;                 // Show error
      return ;
    }
    while ( xRingbufferSend ( fnbuf, newfnam, fnlen, 100 ) == pdFALSE )
    {
      if ( ! SD_okay )                                        // Stop on SD error
      {
        return ;
      }
    }
    SD_filecount++ ;                                          // Count number of files
  }


  //**************************************************************************************************
  //                                      G E T S D T R A C K S                                      *
  //**************************************************************************************************
  // Search all MP3 files on directory of SD card.  Store them in ringbuffer.                        *
  // Will be called recursively.                                                                     *
  //**************************************************************************************************
  bool getsdtracks ( const char * dirname, uint8_t levels )
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
      return false ;
    }
    if ( !root.isDirectory() )                            // Really a directory?
    {
      dbgprint ( "Not a directory" ) ;
      return false ;
    }
    claimSPI ( "sdopen2" ) ;                              // Claim SPI bus
    file = root.openNextFile() ;
    releaseSPI() ;                                        // Release SPI bus
    while ( file )
    {
      if ( file.isDirectory() )                           // Is it a directory?
      {
        dbgprint ( "  DIR : %s", file.path() ) ;
        if ( levels )                                     // Dig in subdirectory?
        {
          if ( strrchr ( file.path(), '/' )[1] != '.' )   // Skip hidden directories
          {
            if ( ! getsdtracks ( file.path(),             // Non hidden directory: call recursive
                                  levels -1 ) )
            {
              return false ;                              // File I/O error
            }
          }
        }
      }
      else                                                // It is a file
      {
        vTaskDelay ( 50 / portTICK_PERIOD_MS ) ;          // Allow others
        const char* ext = file.name() ;                   // Point to begin of name
        ext = ext + strlen ( ext ) - 4 ;                  // Point to extension
        if ( ( strcmp ( ext, ".MP3" ) == 0 ) ||           // It is a file, but is it an MP3?
            ( strcmp ( ext, ".mp3" ) == 0 ) )
        {
          addToRingbuf ( file.path() ) ;                  // Add file to the list
        }
      }
      claimSPI ( "sdopen3" ) ;                            // Claim SPI bus
      file = root.openNextFile() ;
      releaseSPI() ;                                      // Release SPI bus
    }
    return true ;
  }


  //**************************************************************************************************
  //                                   S D I N S E R T C H E C K                                     *
  //**************************************************************************************************
  // Check if new SD card is inserted and can be read.                                               *
  //**************************************************************************************************
  bool SDInsertCheck()
  {
    static uint32_t nextCheckTime = 0 ;                     // To prevent checking too often
    uint32_t        newmillis = millis() ;                  // Current timestamp
    bool            sdinsNew ;                              // Result of insert check
    void*           p ;                                     // Pointer to item from ringbuffer
    size_t          f0 ;                                    // Length of item from ringbuffer
    static bool     sdInserted = false ;                    // Yes, flag for inserted SD
    int8_t          dpin = ini_block.sd_detect_pin ;        // SD inserted detect pin

    if ( newmillis < nextCheckTime )                        // Time to check?
    {
      return false ;                                        // No, return "no new insert"
    }
    nextCheckTime = newmillis + 5000 ;                      // Yes, set new check time
    if ( dpin >= 0 )                                        // Hardware detection possible?
    {
      sdinsNew = ( digitalRead ( dpin ) == LOW ) ;          // Yes, see if card inserted
      if ( sdinsNew == sdInserted )                         // Situation changed?
      {
        return false ;                                      // No, return "no new insert"
      }
      else
      {
        sdInserted = sdinsNew ;                             // Remember status
        if ( ! sdInserted )                                 // Card out?
        {
          dbgprint ( "SD card removed" ) ;
          while ( ( p = xRingbufferReceive ( fnbuf,         // Flush filename list
                                             &f0, 0 ) ) )
          {
            vRingbufferReturnItem ( fnbuf, p ) ;            // Return space to the buffer
          }
          claimSPI ( "SDend" ) ;                            // Claim SPI bus
          SD.end() ;                                        // Unmount SD card
          releaseSPI() ;                                    // Release SPI bus
          SD_okay = false ;                                 // Not okay anymore
        }
        else
        {
          dbgprint ( "SD card inserted" ) ;
          SD_okay = mount_SDCARD ( ini_block.sd_cs_pin ) ;  // Try to mount
        }
        return SD_okay ;                                    // Return result
      }
    }
    else
    {
      if ( SD_okay )                                        // Card already mounted?
      {
        return false ;                                      // Yes, no change
      }
      while ( ( p = xRingbufferReceive ( fnbuf,             // Flush filename list
                                         &f0, 0 ) ) )
      {
        vRingbufferReturnItem ( fnbuf, p ) ;                // Return space to the buffer
      }
      claimSPI ( "SDend2" ) ;                               // Claim SPI bus
      SD.end() ;                                            // Unmount SD card if needed
      releaseSPI() ;                                        // Release SPI bus
      SD_okay = mount_SDCARD ( ini_block.sd_cs_pin ) ;      // Try to mount
      return SD_okay ;                                      // Return result
    }
  }


  //**************************************************************************************************
  //                                       S D T A S K                                               *
  //**************************************************************************************************
  // This task will constantly try to fill the ringbuffer with filenames on SD.                      *
  //**************************************************************************************************
  void SDtask ( void * parameter )
  {
    fnbuf = xRingbufferCreate ( RINGBUFFERSPACE,          // Create 15k ringbuffer
                                RINGBUF_TYPE_NOSPLIT ) ;
    if ( fnbuf == nullptr )
    {
      dbgprint ( "No space for SD ringbuffer" ) ;
      vTaskDelete ( NULL ) ;                              // End this task
    }
    vTaskDelay ( 20000 / portTICK_PERIOD_MS ) ;           // Start delay
    while ( true )                                        // Endless task
    {
      SDInsertCheck() ;                                   // See if new card is inserted
      while ( SD_okay )
      {
        SD_okay = getsdtracks ( "/", SD_MAXDEPTH ) ;      // Get filenames, store in the ringbuffer
      }
      vTaskDelay ( 100 / portTICK_PERIOD_MS ) ;           // Allow other tasks
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
    claimSPI ( "sdopen1" ) ;                              // Claim SPI bus
    mp3file = SD.open ( path ) ;                              // Open the file
    releaseSPI() ;                                        // Release SPI bus
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
    static const char* lastmsg = NULL ;                     // Last debug message
    const char*        newmsg = NULL ;                      // New debug message

    if ( csPin >= 0 )                                       // SD configured?
    {
      claimSPI ( "mountSD" ) ;                              // Claim SPI bus
      SD_okay = SD.begin ( csPin, SPI, SDSPEED ) ;          // Yes, try to init SD card driver
      releaseSPI() ;                                        // Release SPI bus
      if ( ! SD_okay )                                      // Success?
      {
        newmsg = "SD Card Mount Failed!" ;                  // No success, check formatting (FAT)
      }
      else
      {
        SD_okay = ( SD.cardType() != CARD_NONE ) ;          // See if known card
        if ( SD_okay )
        {
          newmsg = "SD card attached" ;                     // Yes, card is okay
        }
        else
        {
          newmsg = "No SD card attached!" ;                 // Card not readable
        }
      }
    }
    if ( newmsg != lastmsg )                                // Status change?
    {
      dbgprint ( newmsg ) ;                                 // Yes, print message
      lastmsg = newmsg ;                                    // Remember last message
    }
    return SD_okay ;
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
