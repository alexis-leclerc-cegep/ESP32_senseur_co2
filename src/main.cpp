#include <Arduino.h>
#include <Adafruit_CCS811.h>
#include <Wire.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <SPI.h>


// https://github.com/khoih-prog/ESP_WiFiManager#why-do-we-need-the-new-async-espasync_wifimanager-library
// Using Multiwifi and auto(re)connect feature
#define ONBOARD_LED 2

Adafruit_CCS811 ccs;
WiFiClient espClient;
PubSubClient MQTT_CLIENT(espClient);

const int mq2_pin = 39;

const int interval = 30000;
unsigned long previousMillis = 0;

const char* mqtt_server = "172.16.5.101";
const int mqtt_port = 1883;

int timer = 0;

void reset() ;
void callback(char* topic, byte* payload, unsigned int length);

void setup() {

    // Initialize the serial port
    Serial.begin(115200);

    pinMode(ONBOARD_LED, OUTPUT);

    if(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        while(true);
    }

    Serial.print("Starting wifi manager");

    // Connect to WiFi
    WiFiManager wifiManager;

    wifiManager.setConnectTimeout(30);



    if (!wifiManager.autoConnect("ESPAlexis")){
        Serial.println("Erreur de connexion.");
    }
    else {
        Serial.println("Connexion Établie.");
    }

    Serial.println("WiFi connection Successful");
    Serial.print("The IP Address of ESP32 Module is: ");
    Serial.println(WiFi.localIP());// Print the IP address

    MQTT_CLIENT.setServer(mqtt_server, mqtt_port);

    if (MQTT_CLIENT.connect(""))
    {
      Serial.println("Connection has been established, well done");
    }
    else
    {
      Serial.println("Looks like the server connection failed...");
    }

    MQTT_CLIENT.setCallback(callback);

    MQTT_CLIENT.subscribe("alexis/lumiere");

    while(!ccs.available());
}

void loop() {
    MQTT_CLIENT.loop();

    if (timer % 1000 == 0){
	    unsigned long currentMillis = millis();
	    if((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)){
		previousMillis = currentMillis;
		reset();
	    }
	    if(ccs.available()){
		if(!ccs.readData()){
		    char buffer[64];
		    char buffer2[64];
		    char buffer3[64];

		    float co2 = ccs.geteCO2();
		    float tvoc = ccs.getTVOC();
		    float co = analogRead(mq2_pin);

		    utoa(co2, buffer, 10);
		    utoa(tvoc, buffer2, 10);
		    utoa(co, buffer3, 10);

		    if(MQTT_CLIENT.publish("alexis/co2", buffer) &
		       MQTT_CLIENT.publish("alexis/tvoc", buffer2) &
		       MQTT_CLIENT.publish("alexis/co", buffer3))
		    {
			Serial.println("Publish message success");
		    }
		    else
		    {
			Serial.println("Could not send message :(");
			reset();
		    }

		    Serial.println(co2);
		    Serial.println(tvoc);
		}

	    }
	
    }

    timer += 10;
    delay(10);

}

void callback(char* topic, byte* payload, unsigned int length){
	Serial.print("Message");

	digitalWrite(ONBOARD_LED, (payload[0] == '1') ? HIGH : LOW);
	Serial.println((char)payload[0]);

	/*
  	for (int i = 0; i < length; i++) {
	    Serial.print((char)payload[i]);
  	}
	*/

}

void reset(){
    Serial.println("Tentative de reconnexion");
    MQTT_CLIENT.disconnect();
    WiFi.disconnect();
    WiFi.reconnect();
    MQTT_CLIENT.connect("");
}

