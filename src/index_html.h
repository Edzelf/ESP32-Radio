// index.html file in raw data format for PROGMEM
//
#define index_html_version 180102
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
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
   <li><a class="pull-left active" href="/index.html">Control</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/search.html">Search</a></li>
   <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP32 Radio **</h1>
   <button class="button" onclick="httpGet('downpreset=1')">PREV</button>
   <button class="button" onclick="httpGet('uppreset=1')">NEXT</button>
   <button class="button" onclick="httpGet('downvolume=2')">VOL-</button>
   <button class="button" onclick="httpGet('upvolume=2')">VOL+</button>
   <button class="button" onclick="httpGet('mute')">(un)MUTE</button>
   <button class="button" onclick="httpGet('stop')">(un)STOP</button>
   <button class="button" onclick="httpGet('status')">STATUS</button>
   <button class="button" onclick="httpGet('test')">TEST</button>
   <table style="width:500px">
    <tr>
     <td colspan="2"><center>
       <label for="preset"><big>Preset:</big></label>
       <br>
       <select class="select selectw" onChange="handlepreset(this)" id="preset">
        <option value="-1">Select a preset here</option>
       </select>
       <br><br>
     </center></td>
    </tr>
    <tr>
     <td><center>
      <label for="HA"><big>Treble Gain:</big></label>
      <br>
      <select class="select" onChange="handletone(this)" id="toneha">
       <option value="8">-12 dB</option>
       <option value="9">-10.5 dB</option>
       <option value="10">-9 dB</option>
       <option value="11">-7.5 dB</option>
       <option value="12">-6 dB</option>
       <option value="13">-4.5 dB</option>
       <option value="14">-3 dB</option>
       <option value="15">-1.5 dB</option>
       <option value="0" selected>Off</option>
       <option value="1">+1.5 dB</option>
       <option value="2">+3 dB</option>
       <option value="3">+4.5 dB</option>
       <option value="4">+6 dB</option>
       <option value="5">+7.5 dB</option>
       <option value="6">+9 dB</option>
       <option value="7">+10.5 dB</option>
      </select>
      <br><br>
     </td>
     <td><center>
      <label for="HF"><big>Treble Freq:</big></label>
      <br>
      <select class="select" onChange="handletone(this)" id="tonehf">
        <option value="1">1 kHz</option>
        <option value="2">2 kHz</option>
        <option value="3">3 kHz</option>
        <option value="4">4 kHz</option>
        <option value="5">5 kHz</option>
        <option value="6">6 kHz</option>
        <option value="7">7 kHz</option>
        <option value="8">8 kHz</option>
        <option value="9">9 kHz</option>
        <option value="10">10 kHz</option>
        <option value="11">11 kHz</option>
        <option value="12">12 kHz</option>
        <option value="13">13 kHz</option>
        <option value="14">14 kHz</option>
        <option value="15">15 kHz</option>
      </select>
      <br><br> 
     </center></td>
     <br><br>
    </tr>
    <tr>
     <td><center>
      <label for="LA"><big>Bass Gain:</big></label>
      <br>
      <select class="select" onChange="handletone(this)" id="tonela">
       <option value="0" selected>Off</option>
       <option value="1">+1 dB</option>
       <option value="2">+2 dB</option>
       <option value="3">+3 dB</option>
       <option value="4">+4 dB</option>
       <option value="5">+5 dB</option>
       <option value="6">+6 dB</option>
       <option value="7">+7 dB</option>
       <option value="8">+8 dB</option>
       <option value="9">+9 dB</option>
       <option value="10">+10 dB</option>
       <option value="11">+11 dB</option>
       <option value="12">+12 dB</option>
       <option value="13">+13 dB</option>
       <option value="14">+14 dB</option>
       <option value="15">+15 dB</option>
      </select>
      <br>
     </td>
     <td><center>
      <label for="LF"><big>Bass Freq:</big></label>
      <br>
      <select class="select" onChange="handletone(this)" id="tonelf">
       <option value="2">10 Hz</option>
       <option value="3">20 Hz</option>
       <option value="4">30 Hz</option>
       <option value="5">40 Hz</option>
       <option value="6">50 Hz</option>
       <option value="7">60 Hz</option>
       <option value="8">70 Hz</option>
       <option value="9">80 Hz</option>
       <option value="10">90 Hz</option>
       <option value="11">100 Hz</option>
       <option value="12">110 Hz</option>
       <option value="13">120 Hz</option>
       <option value="14">130 Hz</option>
       <option value="15">140 Hz</option>
      </select> 
     </center></td>
     <br><br>
    </tr>
   </table>
   <br>
   <input type="text" size="60" id="station" placeholder="Enter a station/file here....">
   <button class="button" onclick="setstat()">PLAY</button>
   <br>
   <br>
   <input type="text" width="600px" size="72" id="resultstr" placeholder="Waiting for a command...."><br>
   <br><br>
   <p>Find new radio stations at <a target="blank" href="http://www.internet-radio.com">http://www.internet-radio.com</a></p>
   <p>Examples: us1.internet-radio.com:8105, skonto.ls.lv:8002/mp3, 85.17.121.103:8800</p><br>
  </center>
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

   function handlepreset ( presctrl )
   {
     httpGet ( "preset=" + presctrl.value ) ;
   }

   function handletone ( tonectrl )
   {
     var theUrl = "/?" + tonectrl.id + "=" + tonectrl.value +
                  "&version=" + Math.random() ;
     var xhr = new XMLHttpRequest() ;
     xhr.onreadystatechange = function() {
       if ( xhr.readyState == XMLHttpRequest.DONE )
       {
         resultstr.value = xhr.responseText ;
       }
     }
     xhr.open ( "GET", theUrl, false ) ;
     xhr.send() ;
   }

   function setstat()
   {
     var theUrl = "/?station=" + station.value + "&version=" + Math.random() ;
     var xhr = new XMLHttpRequest() ;
     xhr.onreadystatechange = function() {
       if ( xhr.readyState == XMLHttpRequest.DONE )
       {
         resultstr.value = xhr.responseText ;
       }
     }
     xhr.open ( "GET", theUrl, false ) ;
     xhr.send() ;
   }

   function selectItemByValue(elmnt, value)
   {
     var sel = document.getElementById(elmnt) ;
     for(var i=0; i < sel.options.length; i++)
     {
       if(sel.options[i].value == value)
         sel.selectedIndex = i;
     }
   }
   
   // Request current status.
   function myRefresh()
   {
    httpGet ('status') ;
    setTimeout(myRefresh,5000) ;
   }

   // Fill preset list initially
   //
   function onloadfunc()
   {
     var i, sel, opt, lines, parts ;
     var theUrl = "/?settings" + "&version=" + Math.random() ;
     var xhr = new XMLHttpRequest() ;
     xhr.onreadystatechange = function() {
       if ( xhr.readyState == XMLHttpRequest.DONE )
       {
         lines = xhr.responseText.split ( "\n" ) ;
         for ( i = 0 ; i < ( lines.length-1 ) ; i++ )
         {
           sel = document.getElementById ( "preset" ) ;
           parts = lines[i].split ( "=" ) ;
           if ( parts[0].indexOf ( "preset_" ) == 0 )
           {
             opt = document.createElement ( "OPTION" ) ;
             opt.value = parts[0].substring ( 7 ) ;
             opt.text = parts[1] ;
             sel.add( opt ) ;
           }
           if ( ( parts[0].indexOf ( "tone" ) == 0 ) ||
                ( parts[0] == "preset" ) )
           {
             selectItemByValue ( parts[0], parts[1] ) ;
           }
         }
         setTimeout(myRefresh,5000) ;
       }
     }
     xhr.open ( "GET", theUrl, false ) ;
     xhr.send() ;
   }

   window.onload = onloadfunc ;   // Run after page has been loaded
 
 </script>
 </body>
</html>
<noscript>
  <link rel="stylesheet" href="radio.css">
  Sorry, ESP-radio does not work without JavaScript!
</noscript>
)=====" ;
