#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"

//---------------
byte NTCPin = A0;
#define SERIESRESISTOR 10000
#define NOMINAL_RESISTANCE 10000
#define NOMINAL_TEMPERATURE 25
#define BCOEFFICIENT 3950


//communicatino

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASSWORD;

const char* mqtt_server = SECRET_MQTT_IP;
const int   mqtt_port = SECRET_MQTT_PORT;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_password = SECRET_MQTT_PASSWD;

const char* mqtt_value = "SD18/thermostat/temp/val";
const char* mqtt_debug = "SD18/thermostat/temp/debug";

unsigned long ts = 0, new_ts = 0; //timestamp

WiFiClient espClient;
PubSubClient client(espClient);

void publishMQTT(String topic, String incoming) {
  Serial.println("MQTT: " + topic + ":" + incoming);
  char charBuf[incoming.length() + 1];
  incoming.toCharArray(charBuf, incoming.length() + 1);

  char topicBuf[topic.length() + 1];
  topic.toCharArray(topicBuf, topic.length() + 1);
  client.publish(topicBuf, charBuf);
}

void setup_wifi() {
  delay(10);
  //Connect to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect using a random clientid
    String clientId = "LivingTemp-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected as " + clientId);
      // Once connected, publish an announcement...

      // ... and resubscribe
      publishMQTT(mqtt_debug, "online");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  setup_wifi();
  ts = millis();
    //Init MQTT Client
  client.setServer(mqtt_server, mqtt_port);


}
void loop()
{
  new_ts = millis();
  if (new_ts - ts > 1000) {
ts = new_ts;
    float ADCvalue;
    float Resistance;
    ADCvalue = analogRead(NTCPin);
    publishMQTT(mqtt_debug, String(ADCvalue));
    //convert value to resistance
    Resistance = (1023 / ADCvalue) - 1;
    Resistance = SERIESRESISTOR / Resistance;


    float steinhart;
    steinhart = Resistance / NOMINAL_RESISTANCE; // (R/Ro)
    steinhart = log(steinhart); // ln(R/Ro)
    steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
    steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart; // Invert
    steinhart -= 273.15; // convert to C
    publishMQTT(mqtt_value, String(steinhart));
    delay(1000);

  }
  //MQTT Loop
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


}
