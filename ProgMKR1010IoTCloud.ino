//Libraries
#include "arduino_secrets.h"
#include <Arduino.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include "thingProperties.h"
#include <HCSR04.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <AverageThermistor.h>
       
#define SENSOR_PIN             A1
#define REFERENCE_RESISTANCE   24000
#define NOMINAL_RESISTANCE     10000
#define NOMINAL_TEMPERATURE    25
#define B_VALUE                3950
/**
  How many readings are taken to determine a mean temperature.
  The more values, the longer a calibration is performed,
  but the readings will be more accurate.
*/
#define READINGS_NUMBER 200

/**
  Delay time between a temperature readings
  from the temperature sensor (ms).
*/
#define DELAY_TIME 10

//Temperature differential which allow pushing alarm
#define DIFFERENTIAL 1

//IFTTT
WiFiSSLClient client;
char serverifttt[] = "maker.ifttt.com";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

BlynkTimer timer;
bool canPushNotification = false;

Thermistor* thermistor = NULL;

void setup() {
  Serial.begin(9600);
  delay(1500); 

  pinMode(6, INPUT);
  
  //Check the wifi connection
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.print("WiFi shield not present");
    while (true);
  }
  //Test
  //Attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named : ");
    Serial.println(SSID);

    //Connect to WPA/WPA2 network
    status = WiFi.begin(SSID, PASS);
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

  thermistor = new AverageThermistor(
      new NTC_Thermistor(
      SENSOR_PIN,
      REFERENCE_RESISTANCE,
      NOMINAL_RESISTANCE,
      NOMINAL_TEMPERATURE,
      B_VALUE
    ),
    READINGS_NUMBER,
    DELAY_TIME
  );

  timer.setInterval(1000L, beerTapOnCpt); 
}

void loop() {
  ArduinoCloud.update();
  timer.run();        // run timer every second

  //Steinhart relation
  temperature = thermistor->readCelsius();

  if ((temperature < tempThreshole) && canPushNotification) {
     iftttSend(tempThreshole);
     canPushNotification = false;
  }
  if (temperature > (tempThreshole + DIFFERENTIAL)){
    canPushNotification = true;
  }
  Serial.println(temperature);
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
  // Serveur IFTTT connection
  Serial.println("Starting connection to server...");
  if (client.connectSSL(serverifttt, 443)) {
    Serial.println("Connected to server IFTTT, ready to trigger alarm...");
    // Make a HTTP request:
    client.println("POST /trigger/BeerTap/with/key/dlBLq2tfIELgiTqr9EWK9h/ HTTP/1.1");
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

float temperatureCalcul(){
  
}