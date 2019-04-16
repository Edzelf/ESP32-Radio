// radio.css file in raw data format for PROGMEM
//
const char radio_css[] PROGMEM = R"=====(
body {
   background-color: lightblue;
   font-family: Arial, Helvetica, sans-serif;
}

h1 {
   color: navy;
   margin-left: 20px;
   font-size: 150%
}

ul {
   list-style-type: none;
   margin: 0;
   margin-left: -10px;
   overflow: hidden;
   background-color: #2C3E50;
   position:fixed;
   top:0;
   width:100%;
   z-index:100;
}

li .pull-left {
   float: left;
}

li .pull-right {
   float: right;
}

li a {
   display: block;
   color: white;
   text-align: center;
   padding: 14px 16px;
   text-decoration: none;
}

li a:hover, a.active {
   background-color: #111;
}

.button {
   width: 90px;
   height: 30px;
   background-color: #128F76;
   border: none;
   color: white;
   text-align: center;
   text-decoration: none;
   display: inline-block;
   font-size: 16px;
   margin: 4px 2px;
   cursor: pointer;
   border-radius: 10px;
}

.buttonr {background-color: #D62C1A;}

.select {
   width: 160px;
   height: 32px;
   padding: 5px;
   background: white;
   font-size: 16px;
   line-height: 1;
   border: 1;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}

.selectw {width: 410px;}

input[type="text"] {
   margin: 0;
   height: 28px;
   background: white;
   font-size: 16px;
   appearance: none;
   box-shadow: none;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}

input[type="text"]:focus {
  outline: none;
}

input[type=submit] {
  width: 200px;
  height: 40px;
  background-color: #128F76;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
  border-radius: 5px;
}

textarea {
  width: 80;
  height: 25;
  border: 1px solid black;
  border-radius: 10px;
  padding: 5px;
  font-family: Courier, "Lucida Console", monospace;
  background-color: white;
  resize: none;
})=====" ;
