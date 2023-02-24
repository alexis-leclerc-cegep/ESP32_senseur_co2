#include <Arduino.h>
#include <Adafruit_CCS811.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

Adafruit_CCS811 ccs;
WiFiClient espClient;
PubSubClient MQTT_CLIENT(espClient);

const char* ssid = "EcoleDuWeb2.4g";
const char* password = "EcoleDuWEB";

const int mq2_pin = 39;

const int interval = 30000;
unsigned long previousMillis = 0;

const char* mqtt_server = "172.16.5.101";
const int mqtt_port = 1883;

void setup() {

    // Initialize the serial port
    Serial.begin(115200);

    if(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        while(1);
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print("*");
    }

    Serial.println("WiFi connection Successful");
    Serial.println("The IP Address of ESP32 Module is: ");
    Serial.println(WiFi.localIP());// Print the IP address

    Serial.print("\n");

    MQTT_CLIENT.setServer(mqtt_server, mqtt_port);

    if (MQTT_CLIENT.connect(""))
    {
      Serial.println("Connection has been established, well done");
    }
    else
    {
      Serial.println("Looks like the server connection failed...");
    }

    while(!ccs.available());
}

void loop() {
    unsigned long currentMillis = millis();
    if((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)){
        Serial.println('Tentative de reconnexion');
        MQTT_CLIENT.disconnect();
        WiFi.disconnect();
        WiFi.reconnect();
        MQTT_CLIENT.connect("");
        previousMillis = currentMillis;
    }
    MQTT_CLIENT.loop();
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
            }

            Serial.println(co2);
            Serial.println(tvoc);
        }

    }

  delay(5000);
}

