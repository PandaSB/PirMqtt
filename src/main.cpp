#include <Arduino.h>

#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <gsender.h>
#include "config_local.h"


/*
EXAMPLE config_local.h

#ifndef _CONFIG_LOCAL_H_
#define _CONFIG_LOCAL_H_

#define GMAIL_LOGIN  "encoded64 login gmail" 
#define GMAIL_PASSWD "encoded64 passwordgmail"
#define GMAIL_EMAIL  "xxxxxx@gmail.com"

#define WIFI_SSID    "SSID"
#define WIFI_PASSWD  "passwd"

#define MQTT_SERVER  "192.168.0.233"

#endif 
*/



const int idxPIR     = 46;

const char* ssid     = WIFI_SSID;         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = WIFI_PASSWD;     // The password of the Wi-Fi network


// MQTT configuration
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_topic = "domoticz/in";

const int pin_PIR = D6 ; 

int CurrentPIRStatus = 0 ; 
int OldPIRStatus = 0 ;

char mqttbuffer[60] =  {0};



// Handle recieved MQTT message, just print it
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void setup() {
  pinMode(pin_PIR,INPUT) ; 
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer


}

void loop() {
  delay(1000); 

  CurrentPIRStatus = digitalRead(pin_PIR) ; 
  if ( CurrentPIRStatus != OldPIRStatus)
  {
    WiFiClient espClient;
    //WiFiClientSecure espClient ; 
    PubSubClient clientmqtt(espClient);

    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);
  
    timeClient.begin();
    timeClient.update();
    String TimeNTP = timeClient.getFormattedDate();
    Serial.println(TimeNTP);


    clientmqtt.setServer(mqtt_server, 1883);

    while (!clientmqtt.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (clientmqtt.connect("D1Mini_PIR")) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.print(clientmqtt.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }

    sprintf(mqttbuffer, "{ \"idx\" : %d, \"nvalue\" : %d, \"svalue\" : \"0\" }", idxPIR, CurrentPIRStatus);
    // send temperature and humidity to the MQTT topic
    clientmqtt.publish(mqtt_topic, mqttbuffer);
    Serial.print ("MQTT Msg : ") ;
    Serial.println (mqttbuffer) ;  

    if (CurrentPIRStatus != 0) {
      Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
      String subject = "Motion Detection";
      String Msg = "Motion Detection @ Home :  " +TimeNTP ; 
      if(gsender->Subject(subject)->Send("barthelemy.steph@gmail.com", Msg)) {
        Serial.println("Message send.");
      } else {
        Serial.print("Error sending message: ");
        Serial.println(gsender->getError());
      }
    }

  }
  Serial.print ("Current Status : ") ; 
  Serial.println (CurrentPIRStatus) ; 
  OldPIRStatus = CurrentPIRStatus ; 
}