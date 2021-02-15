// config.html file in raw data format for PROGMEM
//
#define config_html_version 180806
const char config_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <title>Configuration ESP32-radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="stylesheet" type="text/css" href="radio.css">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ESP32 Radio</a></li>
   <li><a class="pull-left" href="/index.html">Control</a></li>
   <li><a class="pull-left" href="/search.html">Search</a></li>
   <li><a class="pull-left active" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP32 Radio **</h1>
   <p>You can edit the configuration here. <i>Note that this will be effective on the next restart of the radio.</i>
   </p>
   <h3>Available WiFi networks
   <select class="select" onChange="handletone(this)" id="ssid"></select>
   </h3>
   <textarea rows="20" cols="100" id="prefs">Loading preferences</textarea> 
   <br>
   <button class="button" onclick="fsav()">Save</button>
   &nbsp;&nbsp;
   <button class="button buttonr" onclick="httpGet('reset')">Restart</button>
   &nbsp;&nbsp;
   <button class="button buttonr" onclick="httpGet('update')">Update</button>
   &nbsp;&nbsp;
   <button class="button" onclick="ldef('getdefs')">Default</button>
    <br><input type="text" size="80" id="resultstr" placeholder="Waiting for input....">
    <br>

    <script>
      function httpGet ( theReq )
      {
        var theUrl = "/?" + theReq + "&version=" + Math.random() ;
        var xhr = new XMLHttpRequest() ;
        xhr.onreadystatechange = function() {
          if ( xhr.readyState == XMLHttpRequest.DONE )
          {
            resultstr.value = xhr.responseText ;
          }
        }
        xhr.open ( "GET", theUrl ) ;
        xhr.send() ;
      }

      // Load preferences or default preferences
      function ldef ( source )
      {
        var xhr = new XMLHttpRequest() ;
        xhr.onreadystatechange = function()
        {
          if ( xhr.readyState == XMLHttpRequest.DONE )
          {
            prefs.value = xhr.responseText ;
          }
        }
        xhr.open ( "GET", "/?" + source  + "&version=" + Math.random(), false ) ;
        xhr.send() ;
      }

      // Save the preferences
      function fsav()
      {
        var str = prefs.value ;
        var theUrl = "/?saveprefs&version=" + Math.random() ;
        var xhr = new XMLHttpRequest() ;
        xhr.onreadystatechange = function()
        {
          if ( xhr.readyState == XMLHttpRequest.DONE )
          {
            resultstr.value = xhr.responseText ;
          }
        }
        // Remove empty lines
        while ( str.indexOf ( "\r\n\r\n" ) >= 0 )
        {
          str = str.replace ( /\r\n\r\n/g, "\r\n" )      
        }
        while ( str.indexOf ( "\n\n" ) >= 0 )
        {
          str = str.replace ( /\n\n/g, "\n" )      
        }
        xhr.open ( "POST", theUrl, true ) ;
        xhr.setRequestHeader ( "Content-type", "application/x-www-form-urlencoded" ) ;
        xhr.send ( str + "\n" ) ;
      }

      // Fill configuration initially
      // First the available WiFi networks
      var i, select, opt, networks, params ;

      select = document.getElementById("selnet") ;
      var theUrl = "/?getnetworks" + "&version=" + Math.random() ;
      var xhr = new XMLHttpRequest() ;
      xhr.onreadystatechange = function() {
        if ( xhr.readyState == XMLHttpRequest.DONE )
        {
          networks = xhr.responseText.split ( "|" ) ;
          
          for ( i = 0 ; i < ( networks.length - 1 ) ; i++ )
          {
            opt = document.createElement( "OPTION" ) ;
            opt.value = i ;
            opt.text = networks[i] ;
            ssid.add( opt ) ;
          }
        }
      }
      xhr.open ( "GET", theUrl, false ) ;
      xhr.send() ;
      // Now get the configuration parameters from preferences
      ldef ( "getprefs" ) ;
    </script>
  </body>
</html>
)=====" ;
