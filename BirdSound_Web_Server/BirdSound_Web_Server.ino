// Import required libraries
#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

#define PIN_BUSY D5

SoftwareSerial mp3Serial(D1, D2); // RX, TX

AlarmID_t birdChaserAlarm = -1;

//variables for time
int timezone = 0; // 2*3600;
int dst = 0; //day light saving
int romaniaTimeZone = 2; // UTC +2 romania timezont
time_t ntp_time = 0;

//Bird chaser work time
int startSoundFromTime = 8; // 8 in the morning
int stopSoundFromTime = 19; // 7 in the evening

// Replace with your network credentials
//const char* ssid = "BalazsEsAlbert";
//const char* password = "emeseesrobi87";

const char* ssid = "AlbertPanzio";
const char* password = "albertpanzio";

// config static IP
IPAddress ip(192, 168, 1, 155); // where 155 is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway
IPAddress subnet(255, 255, 255, 0); // set subnet mask

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .button {
        background-color: #2196F3;
        border: none;
        color: white;
        padding: 20px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 16px;
        margin: 4px 2px;
        cursor: pointer;
        border-radius: 12px;
    }
  </style>
</head>

<body>
 <div>
    Current time is : <span id="currentDateTime">0</span><br>
 </div>
 
  <h2>Madar Hangoskodo</h2>
  %BUTTONPLACEHOLDER%
<script>
getCurrentDateAndTime();

setInterval(function() {
  getCurrentDateAndTime();
}, 10000); //10 seconds

function getCurrentDateAndTime() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("currentDateTime").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/dateTime", true);
  xhttp.send();
}
function stopMusic() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/stopMusic"); 
  xhr.send();
}
function startMusic() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/startMusic"); 
  xhr.send();
}
function nextMusic() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/nextMusic"); 
  xhr.send();
}
function startRepeater() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/repeatStart"); 
  xhr.send();
}
function stopRepeater() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/repeatStop"); 
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="<div><button class=\"button\" onclick=\"startMusic()\" id=\"start_music\">Start Play</button>";
    buttons+= "<button class=\"button\" onclick=\"stopMusic()\" id=\"stop_music\">Stop Play</button>";
    buttons+= "<button class=\"button\" onclick=\"nextMusic()\" id=\"next_music\">Next</button></div>";
    buttons+= "<div><button class=\"button\" onclick=\"startRepeater()\" id=\"start_repeater\">Start Music Repeater</button>";
    buttons+= "<button class=\"button\" onclick=\"stopRepeater()\" id=\"stop_repeater\">Stop Music Repeater</button></div>";
    buttons+= "<div>Start Sound at : <span id=\"startSound\">"+String(startSoundFromTime)+"</span> hours <br>";
    buttons+= "End Sound at : <span id=\"stopSound\">"+String(stopSoundFromTime)+"</span> hours <br></div>";
    return buttons;
  }
  return String();
}

double getSecondsOfDayToRefTime(time_t ref_time) {
  // to get current time
  time_t now = time(nullptr);
  struct tm curentDayTime = *localtime(&now);
  curentDayTime.tm_hour = curentDayTime.tm_hour + romaniaTimeZone;

  struct tm refTime = *localtime(&ref_time); //convert the time to struct tm*

  String currentTime = String(curentDayTime.tm_mday) + "/" + String(curentDayTime.tm_mon + 1) + "/" + String(curentDayTime.tm_year + 1900) + " ";
  currentTime = currentTime + String(curentDayTime.tm_hour) + ":" + String(curentDayTime.tm_min) + ":" + String(curentDayTime.tm_sec);
  Serial.println("Current Day Time: " + currentTime);

  // show on serial the calculate time
  String currentTime2 = String(refTime.tm_mday) + "/" + String(refTime.tm_mon + 1) + "/" + String(refTime.tm_year + 1900) + " ";
  currentTime2 = currentTime2 + String(refTime.tm_hour) + ":" + String(refTime.tm_min) + ":" + String(refTime.tm_sec);
  Serial.println("Sunrize Time: " + currentTime2);

  double daySec = 0;
  int minusMin = 0;
  int minusHour = 0;

  if (curentDayTime.tm_sec < refTime.tm_sec) {
    daySec = curentDayTime.tm_sec - refTime.tm_sec + 60;
    minusMin = -1;
  } else {
    daySec = curentDayTime.tm_sec - refTime.tm_sec;
  }

  if ((curentDayTime.tm_min - minusMin ) < refTime.tm_min) {
    daySec = daySec + ( 60 * (curentDayTime.tm_min - minusMin - refTime.tm_min + 60));
    minusHour = -1;
  } else {
    daySec = daySec + ( 60 * (curentDayTime.tm_min + minusMin - refTime.tm_min));
  }

  daySec = daySec + ( 3600 * (curentDayTime.tm_hour + minusHour - refTime.tm_hour));

  return daySec;
}

void mp3Next() {
   Serial.println("MP3 Next");
   // to get current time
    time_t now = time(nullptr);
    struct tm curentDayTime = *localtime(&now);
    curentDayTime.tm_hour = curentDayTime.tm_hour + romaniaTimeZone;
    Serial.println("Current time" + String(curentDayTime.tm_hour));
    
   if (curentDayTime.tm_hour > startSoundFromTime && curentDayTime.tm_hour < stopSoundFromTime ) {
     Serial.println("Play next");
     mp3_next ();
   } else {
    mp3_stop ();
   }
}

void initSoundRepeater() {
   if (!Alarm.isAllocated(birdChaserAlarm)) {
       Serial.println("--- Init Sound repeater ");
      birdChaserAlarm = Alarm.timerRepeat(900, mp3Next); // timer for every 15 minutes; 
   }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(PIN_BUSY, INPUT);
  Serial.println("Setting up software serial");
  mp3Serial.begin (9600);
  Serial.println("Setting up mp3 player");
  mp3_set_serial (mp3Serial);  
  // Delay is required before accessing player. From my experience it's ~1 sec
  delay(1000); 
  mp3_set_volume (15);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  /*************************************************************************/
  /*** Setup Time **********************************************************/
  /*************************************************************************/
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for internet time");
  while (!ntp_time) {
    time(&ntp_time);
    Serial.println("*");
    Alarm.delay(1000);
  }
  Serial.println("Time response....OK");


  /*************************************************************************/
  /*** Setup Scheduler ALARMS **********************************************************/
  /*************************************************************************/
  time_t now = time(nullptr);
  setTime(now);

  initSoundRepeater();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/startMusic", HTTP_GET, [] (AsyncWebServerRequest *request) {
     mp3_next ();
    request->send(200, "text/plain", "OK");
  });

  server.on("/stopMusic", HTTP_GET, [] (AsyncWebServerRequest *request) {
     mp3_stop ();
    request->send(200, "text/plain", "OK");
  });

  
  server.on("/nextMusic", HTTP_GET, [] (AsyncWebServerRequest *request) {
     mp3_next ();
    request->send(200, "text/plain", "OK");
  });

  server.on("/repeatStart", HTTP_GET, [] (AsyncWebServerRequest *request) {
     mp3Next();
    initSoundRepeater();
    request->send(200, "text/plain", "OK");
  });

  server.on("/repeatStop", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println("--- Stop Sound repeater ");
    Alarm.free (birdChaserAlarm);
    mp3_stop ();
    request->send(200, "text/plain", "OK");
  });

  /* Get Date and Time*/
  server.on("/dateTime", HTTP_GET, [] (AsyncWebServerRequest *request)
  {
    // to get current time
    time_t now = time(nullptr);
    struct tm curentDayTime = *localtime(&now);
    curentDayTime.tm_hour = curentDayTime.tm_hour + romaniaTimeZone;
  
    String currentTime = String(curentDayTime.tm_mday) + "/" + String(curentDayTime.tm_mon + 1) + "/" + String(curentDayTime.tm_year + 1900) + " ";
    currentTime = currentTime + String(curentDayTime.tm_hour) + ":" + String(curentDayTime.tm_min) + ":" + String(curentDayTime.tm_sec);
    
    // Serial.println("Current Time:" + currentTime);
     request->send(200, "text/html", currentTime);
  });
  
  // Start server
  server.begin();
}
  
void loop () {
    Alarm.delay(0);
}
