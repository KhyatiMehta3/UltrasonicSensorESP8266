/* ESP8266 Web Server With Web Socket to display Sensor readings on a web page.
 * The Example code here uses ultrasonic Sensor HCSR-04 and sends the reading from the server to client
 * Using web socket on port 81 with server on port 80.
 * Authored by KKM. 
 * References :
 * https://github.com/Links2004/arduinoWebSockets
 * https://github.com/esp8266/Arduino
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>

#define trig 13
#define echo 12

const char* ssid = "......"; // Your WiFi SSID will go here.
const char* password = "......"; // Your WiFi password will go here.

long duration, distance;

ESP8266WebServer server(80); //Launch a server on default port 80.

WebSocketsServer webSocket = WebSocketsServer(81); // Web socket on default port 81.

//Write HTML code of website
static const char PROGMEM INDEX_HTML [] = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable = 0">
<title> ULTRASONIC SENSOR </title>
<style>
"body { background-color = #808080; font-family: Arial, Helvetica, Sans-Serif, roboto; Color: #000000;}"
</style>
<script>
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
  console.log(evt);
  document.getElementById('sensorReading').innerHTML = evt.data + ' cm';
  };
}
</script>
</head>
<body onload = "javascript:start()">
<h1><center> Ultrasonic Sensor Reading</h1>
<p> Your ultrasonic sensor reads : <div id="sensorReading"> </div></p>
</body>
</html>
)rawliteral";

//Code to fire up the ultrasonic sensor and get back the distance in the form of string.
String fireSensor(){
  digitalWrite(trig,LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH); //Trig pin should be high for 10uS according to the working of the sensor.
  delayMicroseconds(10);
  digitalWrite(trig,LOW);
  duration = pulseIn(echo,HIGH);

  /* Duration includes time taken for the wave to transmit and receive. 
  We're only interested in the duration of the receiving wave.
  Speed of sound in cm per microsecond is 34300/1000000 = 0.0343 cm/us
  (Duration/2)*0.0343 = Duration/58.2
  */
  
  distance = duration/58.2;

  Serial.println(distance);
  delay(50);

  return String(distance);
}

//Handle Web socket events.
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      webSocket.sendTXT(num, fireSensor().c_str(), strlen(fireSensor().c_str()));
    }
    break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
}
}

void handleRoot() {
  digitalWrite(LED_BUILTIN, HIGH);
  server.send(200, "text/html", INDEX_HTML); //Server sends the website to the client.
  digitalWrite(LED_BUILTIN, LOW);
}

//Handler function for unknown URL 
void handleNotFound() {
  digitalWrite(LED_BUILTIN, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, LOW);
}

void setup(void) {
  
  IPAddress ip(192,168,1,205);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  WiFi.config(ip, gateway, subnet);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop(void) {
  webSocket.loop();
  server.handleClient();
}
