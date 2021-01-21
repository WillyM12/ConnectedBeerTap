//Libraries
#include "arduino_secrets.h"
#include "thingProperties.h"
#include <Arduino.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <NTC_Thermistor.h>
#include <AverageThermistor.h>
#include "LiquidCrystal_I2C.h"

#define BEERTAPSTATE_PIN       0
#define BUTTONLEFT_PIN         1
#define BUTTONVALID_PIN        2
#define BUTTONRIGHT_PIN        3
       
#define SENSOR_PIN             A0
#define REFERENCE_RESISTANCE   24000
#define NOMINAL_RESISTANCE     10000
#define NOMINAL_TEMPERATURE    25
#define B_VALUE                3950

#define BATTERYLIFE_PIN        A1
#define MAXVOLTAGE             4.2
#define MINVOLTAGE             3.2
/**
  How many readings are taken to determine a mean temperature.
  The more values, the longer a calibration is performed,
  but the readings will be more accurate.
*/
#define READINGS_NUMBER 50

/**
  Delay time between a temperature readings
  from the temperature sensor (ms).
*/
#define DELAY_TIME 10

//Temperature differential which allow pushing notification
float differential=1.0;

//Battery percent
float percentBatteryLife;

//IFTTT
WiFiSSLClient client;
char serverifttt[] = "maker.ifttt.com";

//Wifi connection
int status = WL_IDLE_STATUS;
WiFiServer server(80);

//Blynk interruption
BlynkTimer timer;

//LCD logos
uint8_t bell[8]  = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
uint8_t empty[8]  = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint8_t clock[8] = {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0};
uint8_t degree[8] = {0xe, 0x11, 0x11, 0x11, 0xe, 0x0, 0x0};
uint8_t duck[8]  = {0x0, 0xc, 0x1d, 0xf, 0xf, 0x6, 0x0};
uint8_t check[8] = {0x0, 0x1 ,0x3, 0x16, 0x1c, 0x8, 0x0};
uint8_t cross[8] = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};
uint8_t retarrow[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};

//Initialization
int step = 0;
bool canPushNotification = false;
Thermistor* thermistor = NULL;
bool buttonLeft = 0;
bool buttonLeftMem = 0;
bool buttonLeftFM = 0;
bool buttonRight = 0;
bool buttonRightMem = 0;
bool buttonRightFM = 0;
bool buttonValid = 0;
bool buttonValidMem = 0;
bool buttonValidFM= 0;

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  delay(1500); 

  pinMode(BEERTAPSTATE_PIN, INPUT);
  pinMode(BUTTONLEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTONVALID_PIN, INPUT_PULLUP);
  pinMode(BUTTONRIGHT_PIN, INPUT_PULLUP);
  //analogReference(AR_EXTERNAL);

  //LCD display
  lcd.init();
  lcd.setBacklight(HIGH);

  lcd.createChar(0, bell);
	lcd.createChar(1, empty);
	lcd.createChar(2, clock);
	lcd.createChar(3, degree);
	lcd.createChar(4, duck);
	lcd.createChar(5, check);
	lcd.createChar(6, cross);
	lcd.createChar(7, retarrow);
  lcd.setCursor(0,0);
  lcd.print("Chargement...");

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
  lcd.clear();
}

void loop() {
  ArduinoCloud.update();
  timer.run();        // run timer every second

  //Battery life
  float batteryPercent = analogRead(BATTERYLIFE_PIN);
  int minValue = (1023 * MINVOLTAGE) / 5;
  int maxValue = (1023 * MAXVOLTAGE) / 5;

  batteryPercent = ((batteryPercent - minValue) / (maxValue - minValue)) * 100;

  if(batteryPercent > 99){
    percentBatteryLife = 99;
  }
  else if(batteryPercent < 0){
    percentBatteryLife = 0;
  }
  else{
    percentBatteryLife = batteryPercent;
  }

  //Rising edge detection
  buttonLeft = digitalRead(BUTTONLEFT_PIN);
  ((buttonLeft != buttonLeftMem) && (buttonLeft==HIGH)) ? buttonLeftFM=true : buttonLeftFM=false;
  buttonLeftMem = buttonLeft;

  buttonRight = digitalRead(BUTTONRIGHT_PIN);
  ((buttonRight != buttonRightMem) && (buttonRight==HIGH)) ? buttonRightFM=true : buttonRightFM=false;
  buttonRightMem = buttonRight;

  buttonValid = digitalRead(BUTTONVALID_PIN);
  ((buttonValid != buttonValidMem) && (buttonValid==HIGH)) ? buttonValidFM=true : buttonValidFM=false;
  buttonValidMem = buttonValid;

  //Steinhart relation
  temperature = thermistor->readCelsius();

  //Notification
  if ((temperature < tempThreshold) && canPushNotification) {
     iftttSend(tempThreshold);
     canPushNotification = false;
  }
  if (temperature > (tempThreshold + differential)){
    canPushNotification = true;
  }

  //Temperature differential and threshold control
  if (buttonValidFM){
    step += 1;
  }
  if (step > 1 ){
    step = 0;
  }
  //State machine
  switch (step)
  {
  case 0:
    lcd.setCursor(15,0);
    lcd.write(7);
    lcd.setCursor(15,1);
    lcd.write(1);
    if(buttonLeftFM){
      differential -= 0.1;
    }
    if(buttonRightFM){
      differential += 0.1;
    }
    break;
  case 1:
    lcd.setCursor(15,1);
    lcd.write(7);
    lcd.setCursor(15,0);
    lcd.write(1);
    if(buttonLeftFM){
      tempThreshold -= 0.1;
    }
    if(buttonRightFM){
      tempThreshold += 0.1;
    }
    break;
  } 

lcdDisplay();
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
  Serial.println("Alarm triggered ! Temperature value is less than : " + str_val);
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
  bool beerTapOn = digitalRead(BEERTAPSTATE_PIN);

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

void lcdDisplay(){
    char bufferTemp[6];
    char bufferTime[9];
    char bufferDif[6];
    char bufferNotif[6];
    char bufferBattery[4];

    //Temperature display
    sprintf(bufferTemp,"%d.%d", (int)temperature, (int)(temperature*10)%10); 
    lcd.setCursor(4,0);
    lcd.print(bufferTemp);
    lcd.setCursor(7,0);
    lcd.write(3);
    lcd.setCursor(8,0);
    lcd.print("C");

    //Time display
    sprintf(bufferTime,"%dJ%dh%dm",timeElapsedinDay,timeElapsedinHou,timeElapsedinMin);
    lcd.setCursor(0,1);
    lcd.print(bufferTime);

    //Differential display
    sprintf(bufferDif,"<%d.%d>", (int)differential, (int)(differential*10)%10);
    lcd.setCursor(9,0);
    lcd.write(4);
    lcd.setCursor(10,0);
    lcd.print(bufferDif);

    //Temeprature threshold display
    sprintf(bufferNotif,"<%d.%d>", (int)tempThreshold, (int)(tempThreshold*10)%10);
    lcd.setCursor(9,1);
    lcd.write(0);
    lcd.setCursor(10,1);
    lcd.print(bufferNotif);

    //Battery life display
    sprintf(bufferBattery,"%d%%", (int)percentBatteryLife);
    lcd.setCursor(0,0);
    lcd.print(bufferBattery);
}