// Import required libraries
#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>

#define PIN_BUSY D5

SoftwareSerial mp3Serial(D1, D2); // RX, TX

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true  

// Set number of relays
#define NUM_RELAYS  1

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {13};

// Replace with your network credentials
//const char* ssid = "BalazsEsAlbert";
//const char* password = "emeseesrobi87";

const char* ssid = "AlbertPanzio";
const char* password = "albertpanzio";

// config static IP
IPAddress ip(192, 168, 1, 155); // where 155 is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway
IPAddress subnet(255, 255, 255, 0); // set subnet mask

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

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
  <h2>Madar Hangoskodo</h2>
  %BUTTONPLACEHOLDER%
<script>
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
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="<button class=\"button\" onclick=\"startMusic()\" id=\"stop music\">Start Play</button>";
    buttons+= "<button class=\"button\" onclick=\"stopMusic()\" id=\"stop music\">Stop Play</button>";
    buttons+= "<button class=\"button\" onclick=\"nextMusic()\" id=\"stop music\">Next</button>";
    return buttons;
  }
  return String();
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
  
  // Start server
  server.begin();
}
  
void loop () {
}
