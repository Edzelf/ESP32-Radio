// search.html file in raw data format for PROGMEM
//
#define search_html_version 1210215
const char search_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
 <head>
  <title>ESP32-radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="stylesheet" type="text/css" href="radio.css">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ESP32 Radio</a></li>
   <li><a class="pull-left" href="/index.html">Control</a></li>
   <li><a class="pull-left active" href="/search.html">Search</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP32 Radio search stations **</h1>
   <table width="450" class="pull-left">
   <tr>
   <td><button class="button" onclick="listStats('60s')">60s</button></td>
   <td><button class="button" onclick="listStats('70s')">70s</button></td>
   <td><button class="button" onclick="listStats('80s')">80s</button></td>
   <td><button class="button" onclick="listStats('80s')">90s</button></td>
   <td><button class="button" onclick="listStats('metal')">metal</button></td>
   <td><button class="button" onclick="listStats('rock')">Rock</button></td><tr>
   <td><button class="button" onclick="listStats('pop')">Pop</button></td>
   <td><button class="button" onclick="listStats('disco')">Disco</button></td>
   <td><button class="button" onclick="listStats('trance')">Trance</button></td>
   <td><button class="button" onclick="listStats('techno')">Techno</button></td>
   <td><button class="button" onclick="listStats('house')">House</button></td>
   <td><button class="button" onclick="listStats('dance')">Dance</button></td><tr>
   <td><button class="button" onclick="listStats('reggae')">Reggae</button></td>
   <td><button class="button" onclick="listStats('funk')">Funk</button></td>
   <td><button class="button" onclick="listStats('top')">Top</button></td>
   <td><button class="button" onclick="listStats('synth')">Synth</button></td><tr>
   <td><button class="button" onclick="listStats('fantasy')">Fantasy</button></td>
   <td><button class="button" onclick="listStats('rain')">Rain</button></td>
   <td><button class="button" onclick="listStats('ambient')">Ambient</button></td>
   <td><button class="button" onclick="listStats('meditation')">Meditation</button></td>
   <td><button class="button" onclick="listStats('lounge')">Lounge</button></td>
   <td><button class="button" onclick="listStats('chill')">Chill</button></td><tr>
   </table>
   <br><input type="text" size="80" id="resultstr" placeholder=""><br>
   <table class="table2" id="stationsTable" width="450">
   </table>
  </center>
 </body>
</html>

<script>

 var stationArr ;

 // Get info from a radiobrowser.  Working servers are:
 // https://de1.api.radio-browser.info, https://fr1.api.radio-browser.info, https://nl1.api.radio-browser.info
 function listStats ( genre )
 {
  var theUrl = "https://nl1.api.radio-browser.info/json/stations/bytag/" +
               genre +
               "?hidebroken=true" ;
  var xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
    var table = document.getElementById('stationsTable') ;
    table.innerHTML = "" ;
    stationArr = JSON.parse ( this.responseText ) ;
    var i ;
    var snam ;
    var oldsnam = "" ;
    for ( i = 0 ; i < stationArr.length ; i++ )
    {
      snam = stationArr[i].name ;
      if ( stationArr[i].url_resolved.startsWith ( "http:") &&            // https: not supported yet
           snam != oldsnam )
      {
        var row = table.insertRow() ;
        var cell1 = row.insertCell(0) ;
        var cell2 = row.insertCell(1) ;
        cell1.innerHTML = snam ;
        cell2.innerHTML = stationArr[i].codec ;
        cell1.setAttribute ( 'onclick', 'setStation(' + i + ');' ) ;
        oldsnam = snam ;
      }
    }
   }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 }

 function setStation ( inx )
 {
  var table = document.getElementById('stationsTable') ;
  var snam ;
  var theUrl ;

  snam = stationArr[inx].url_resolved ;
  snam = snam.substr ( 7 ) ;
  theUrl = "/?station=" + snam + "&version=" + Math.random() ;
  //table.rows[inx].cells[0].style.backgroundColor = "#333333" 
  var xhr = new XMLHttpRequest() ;
  xhr.onreadystatechange = function() 
  {
   if ( this.readyState == XMLHttpRequest.DONE )
   {
     resultstr.value = xhr.responseText ;
   }
  }
  xhr.open ( "GET", theUrl ) ;
  xhr.send() ;
 }
   
</script>
)=====" ;