#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define WIFI_AP "Your Wifi SSID here"
#define WIFI_PASSWORD "Your Wifi Password here"

#define TOKEN "Your Device token here"
#define DEVICE_ID "Your Device ID here"

#define trig 13
#define echo 12

long duration, distance;

char thingsboardServer[] = "demo.thingsboard.io"; //Specially for Live Demo users

WiFiClient wifiClient;

//Initialize the ultrasonic distance sensor
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

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void setup()
{
  Serial.begin(115200);
  delay(10);
  InitWiFi();
  client.setServer( thingsboardServer, 1883 );
  lastSend = 0;
}

void loop()
{
  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after 1 seconds
    sendDistanceData();
    lastSend = millis();
  }

  client.loop();
}

void sendDistanceData(){
  String distance = fireSensor();
  Serial.print("distance :");
  Serial.print(distance);

  String payload = "{distance:";
  payload += distance;
  payload += "}";

  //Convert string to character array
  char attributes[100];
  payload.toCharArray(attributes,100);
  client.publish("v1/devices/me/telemetry",attributes);

  Serial.println("Sent this to server");
  Serial.println(attributes);
  
  }

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(DEVICE_ID, TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}
