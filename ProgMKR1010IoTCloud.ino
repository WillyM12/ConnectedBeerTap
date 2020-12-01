//Libraries
#include <Arduino.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include "thingProperties.h"
#include <HCSR04.h>

//Distance sensor
const int trigPin = 2;
const int echoPin = 3;

//Temperature threshole
int tempThreshole = 4;

//IFTTT
WiFiSSLClient client;
char serverifttt[] = "maker.ifttt.com";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

UltraSonicDistanceSensor distanceSensor(trigPin, echoPin);
BlynkTimer timer;
bool canPushNotification = false;

float voltage;
int ADCValue;
float resistance;

void setup() {
  Serial.begin(9600);
  delay(1500); 

  pinMode(6, INPUT);
  
  //Check the wifi connection
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.print("WiFi shield not present");
    while (true);
  }

  //Attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named : ");
    Serial.println(ssid);

    //Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
    //Wait 10 seconds for connection
    delay(10000);
  }

  server.begin();         //Start the web server on port 80
  printWifiStatus();      //You are connected now, so print out the status

  //This function takes care of connecting your sketch variables to the ArduinoIoTCloud object
  initProperties();

  //Initialize Arduino IoT Cloud library
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(DBG_INFO);
  ArduinoCloud.printDebugInfo();

  timer.setInterval(1000L, beerTapOnCpt); 
}

void loop() {
  ArduinoCloud.update();
  timer.run();        // run timer every second
  ADCValue = analogRead(A0);
  voltage = ADCValue * (12 / 1023);
  Serial.println(voltage);
  if ((distance < tempThreshole) && canPushNotification) {
     iftttSend(tempThreshole);
     canPushNotification = false;
  }
  if (distance > (tempThreshole + tempDiff)){
    canPushNotification = true;
  }
}

void printWifiStatus() {
  //Print rhe SSID of the net work you are attached to
  Serial.print("SSID : ");
  Serial.println(WiFi.SSID());

  //Print your WiFi shield's IP adress
  IPAddress ip = WiFi.localIP();
  Serial.print("IP address : ");
  Serial.println(ip);

  //Print the received signal strenght
  long rssi = WiFi.RSSI();
  Serial.print("Signal strenght (RSSI) : ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void iftttSend(int val) {
  String str_val = String(val);
  Serial.println("Alarm triggered ! Distance value is up to: " + str_val);
  String data = "{\"value1\":\"" + str_val + "\"}";
  // Connexion au serveur IFTTT
  Serial.println("Starting connection to server...");
  if (client.connectSSL(serverifttt, 443)) {
    Serial.println("Connected to server IFTTT, ready to trigger alarm...");
    // Make a HTTP request:
    client.println("POST /trigger/BeerTap/with/key/dlBLq2tfIELgiTqr9EWK9h/ HTTP/1.1"); // Replace  by your own IFTTT key
    client.println("Host: maker.ifttt.com");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    Serial.println("IFTTT alarm triggered !");
  }
  else {
    Serial.println("Connection at IFTTT failed");
  }
}

void beerTapOnCpt(){
  bool beerTapOn = digitalRead(6);
  
  if (beerTapOn && !resetCpt){
    timeElapsedinSec += 1;
    if (timeElapsedinSec == 60){
      timeElapsedinMin += 1;
      timeElapsedinSec = 0;
      if (timeElapsedinMin == 60){
        timeElapsedinHou += 1;
        timeElapsedinMin =0;
        if (timeElapsedinHou == 24){
          timeElapsedinDay += 1;
          timeElapsedinHou = 0;
        }
      }
    }
  }
  else{
    timeElapsedinSec = 0;
    timeElapsedinMin = 0;
    timeElapsedinHou = 0;
    timeElapsedinDay = 0;
  }
}
