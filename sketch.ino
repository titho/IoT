/*
   ESP8266 NodeMCU AJAX Demo
   Updates and Gets data from webpage without page refresh
   https://circuits4you.com
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SDS011.h>
#include "RunningAverage.h"
#include "ThingSpeak.h"
#include <Wire.h>  // This library is already built in to the Arduino IDE
#include <LiquidCrystal_I2C.h> //This library you can add via Include Library > Manage Library > 

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient  client;

//SSID and Password of your WiFi router
const char* ssid = "wifi";
const char* password = "pass";

unsigned long myChannelNumber = 795367;
const char * myWriteAPIKey = "G3HGYB1EW0PA03ZU";

float p10, p25;
String av10, av25;
bool isPM = true;
SDS011 sds;
int error;

RunningAverage pm25Stats(10);
RunningAverage pm10Stats(10);

//==============================================================
//                     Get SDS011 data
//==============================================================
void getPM() {
  Serial.println("Calibrating SDS011 (15 sec)");
  for (int i = 0; i < 15; i++)
  {
    error = sds.read(&p25, &p10);

    if (!error && p25 < 999 && p10 < 1999) {
      pm25Stats.addValue(p25);
      pm10Stats.addValue(p10);
    }
    if (digitalRead(0) != 1) {
      Serial.println("Screen changed! ");
      isPM = !isPM;
    }
    Serial.println("Average PM10: " + String(pm10Stats.getAverage()) + ", PM2.5: " + String(pm25Stats.getAverage()));
    delay(1500);
  }
}

//==============================================================
//                  SETUP
//==============================================================
void setup(void) {
  Serial.begin(9600);
  sds.begin(D6, D7);
  pinMode(0, INPUT_PULLUP);

  int temp = digitalRead(0);
  Serial.println(temp);

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  Wire.begin(D2, D1);
  lcd.begin();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight

  ThingSpeak.begin(client);  // Initialize ThingSpeak

  lcd.print("Setup ready!");
}
//==============================================================
//                     LOOP
//==============================================================
void loop(void) {
  pm25Stats.clear();
  pm10Stats.clear();

  sds.wakeup();

  getPM();

  av10 = String(pm10Stats.getAverage());
  av25 = String(pm25Stats.getAverage());

  ThingSpeak.setField(1, av10);
  ThingSpeak.setField(2, av25);

  int statusCodeWrite = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (statusCodeWrite == 200) {
    Serial.println("Channel update successful.");
  }
  else {
    Serial.println("Problem updating channel. HTTP error code " + String(statusCodeWrite));
  }

  // Get last data uploaded in ThingSpeak for PM10
  float lastPM10 = ThingSpeak.readFloatField(myChannelNumber, 1);

  int statusCodeRead10 = ThingSpeak.getLastReadStatus();
  float lastPM25 = ThingSpeak.readFloatField(myChannelNumber, 2);
  int statusCodeRead25 = ThingSpeak.getLastReadStatus();

  // Check the status of the read operation to see if it was successful
  if (statusCodeRead10 == 200 && statusCodeRead25 == 200) {
    Serial.println("PM10: " + String(lastPM10) + ", PM2.5: " + String(lastPM25));
  }
  else {
    Serial.println("Problem reading channel. HTTP error code " + String(statusCodeRead10) + " and " + String(statusCodeRead25));
  }

  if (isPM) {
    lcd.clear();
    lcd.print("PM10: " + String(lastPM10));
    lcd.setCursor(0, 1);
    lcd.print("PM2.5: " + String(lastPM25));
  } else {
    lcd.clear();
    lcd.print("Temperature: nan");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: nan");
  }
  
  Serial.println();
}
