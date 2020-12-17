#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

#define THING_ID "70486205-4b43-4f41-97ae-e83e6dc12b29"

const char SSID[] = SECRET_SSID;    // Network SSID (name)
const char PASS[] = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

float temperature;

//BeerTap On counter
int timeElapsedinSec;
int timeElapsedinMin;
int timeElapsedinHou;
int timeElapsedinDay;

//Reset counter
bool resetCpt;

//Temperature differential
float tempThreshold;

void initProperties() {
  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(temperature, READ, ON_CHANGE, NULL, 0);
  ArduinoCloud.addProperty(timeElapsedinHou, READ, ON_CHANGE, NULL, 0);
  ArduinoCloud.addProperty(timeElapsedinDay, READ, ON_CHANGE, NULL, 0);
  ArduinoCloud.addProperty(resetCpt, READWRITE, ON_CHANGE);
  ArduinoCloud.addProperty(tempThreshold, READWRITE, ON_CHANGE);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID,PASS);
