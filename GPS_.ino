/*
 * Multiwii pro V2 + MTK GPS sending NMEA CGGA & GPHDT 
 * to ladybug3 camera for ability to get COMPASS (HMC5883)
 * accurate heading + lat, lon, alt etc ...
 * author : kinoki.fr 
 */

#include <Adafruit_GPS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

#define GPSSerial Serial2
#define GPSECHO false

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
Adafruit_GPS GPS(&GPSSerial);

uint32_t timer = millis();
String cap = "";


void displayheading(void){
  float headingDegrees = 0;
  sensors_event_t event;
  mag.getEvent(&event);
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  float declinationAngle = 0.4;
  heading += declinationAngle;

  if (heading < 0)
    heading += 2 * PI;

  if (heading > 2 * PI)
    heading -= 2 * PI;

  uint8_t ByteDatum = random(10, 99);
  headingDegrees =  heading * 180 / M_PI;
  headingDegrees = (int)headingDegrees;

  if ( headingDegrees < 100 ) {
    cap = "0" + (String)headingDegrees;
  } else {
    cap = headingDegrees;
  }

}


void setup(){

  Serial.begin(115200);
  if (!mag.begin()){
   
    Serial.println("No HMC5883 detected");
    while (1);
  }

  Serial.println("GPS start");
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPSSerial.println(PMTK_Q_RELEASE);

}



void loop() {
  char c = GPS.read();
  if (GPSECHO)
    if (c) Serial.print(c);

  if (GPS.newNMEAreceived()) {
    Serial.println(GPS.lastNMEA());

    if (!GPS.parse(GPS.lastNMEA()))
      return;

    // $GPHDT,175.58,T*0B
    String stringOne = "GPHDT," + cap + ",T";
    char string[50];
    stringOne.toCharArray(string, 50);

    int XOR;
    int i;
    int c;
    for (XOR = 0, i = 0; i < strlen(string); i++){
      c = (unsigned char) string[i];
      if (c == '*') break;               
      if (c != '$') XOR ^= c;               
    }
    Serial.print("$");
    Serial.print(stringOne);
    Serial.print("*0");
    Serial.println(XOR, HEX);
  }
  if (timer > millis()) timer = millis();

  if (millis() - timer > 2000) {
    timer = millis();
    displayheading();
  }
}
