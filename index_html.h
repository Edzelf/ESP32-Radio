// index.html file in raw data format for PROGMEM
//
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <title>ESP32-radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ESP32 Radio</a></li>
   <li><a class="pull-left active" href="/index.html">Control</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <li><a class="pull-left" href="/about.html">About</a></li>
  </ul>
  <br><br><br>
  <center>
   <h1>** ESP32 Radio **</h1>
   <button class="button" onclick="httpGet('downpreset=1')">PREV</button>
   <button class="button" onclick="httpGet('uppreset=1')">NEXT</button>
   <button class="button" onclick="httpGet('downvolume=2')">VOL-</button>
   <button class="button" onclick="httpGet('upvolume=2')">VOL+</button>
   <button class="button" onclick="httpGet('stop')">STOP</button>
   <button class="button" onclick="httpGet('resume')">RESUME</button>
   <button class="button" onclick="httpGet('status')">STATUS</button>
   <button class="button" onclick="httpGet('test')">TEST</button>
   <table style="width:500px">
    <tr>
     <td colspan="2"><center>
       <label for="selpres"><big>Preset:</big></label>
       <br>
       <select class="select selectw" onChange="handlepreset(this)" id="selpres">
        <option value="-1">Select a preset here</option>
       </select>
       <br><br>
     </center></td>
    </tr>
    <tr>
     <td><center>
      <label for="HA"><big>Treble Gain:</big></label>
      <br>
      <select class="select" onChange="handletone(this)" id="HA">
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
      <select class="select" onChange="handletone(this)" id="HF">
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
      <select class="select" onChange="handletone(this)" id="LA">
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
      <select class="select" onChange="handletone(this)" id="LF">
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
   <button class="button button-play" onclick="setstat()">PLAY</button>
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
    if ( presctrl.value >= 0 )
    {
      httpGet ( "preset=" + presctrl.value ) ;
    }
   }

   function handletone ( tonectrl )
   {
     var theUrl = "/?tone" + tonectrl.id + "=" + tonectrl.value +
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
   // Fill preset list initially
   //
   var i, select, opt, stations ;
   select = document.getElementById("selpres") ;
   var theUrl = "/?list" + "&version=" + Math.random() ;
   var xhr = new XMLHttpRequest() ;
   xhr.onreadystatechange = function() {
     if ( xhr.readyState == XMLHttpRequest.DONE )
     {
       stations = xhr.responseText.split ( "|" ) ;
       for ( i = 0 ; i < ( stations.length - 1 ) ; i++ )
       {
         opt = document.createElement( "OPTION" ) ;
         opt.value = stations[i].substring ( 0, 2 ) ;
         opt.text = stations[i].substring ( 2 ) ;
         select.add( opt ) ;
       }
     }
   }
   xhr.open ( "GET", theUrl, false ) ;
   xhr.send() ;
  </script>
  <script type="text/javascript">
    var stylesheet = document.createElement('link') ;
    stylesheet.href = 'radio.css' ;
    stylesheet.rel = 'stylesheet' ;
    stylesheet.type = 'text/css' ;
    document.getElementsByTagName('head')[0].appendChild(stylesheet) ;
  </script>
 </body>
</html>
<noscript>
  <link rel="stylesheet" href="radio.css">
  Sorry, ESP-radio does not work without JavaScript!
</noscript>
)=====" ;
