#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "defs.h"
#include "functions.h"

RF24 radio( D2, D1); // CE, CSN
const byte address[6] = "00001";
PayloadData payload_data = PayloadData();

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTCData rtc_data;
long millisecs_to_dim = 15*1000;

void setup() {
  Serial.begin(9600);
  // Serial.println("Starting");
  if( !radio.begin()){
    Serial.print("radio.begin() Error");
    while(1){};
  }
  Serial.println("RF24 Started");
  radio.setChannel(2);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data
  Wire.begin( D3, D4);
  if( !display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error starting display");
    while (true);
  }

  read_rtc_data();
  display.setTextWrap(false); 
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&FreeSans9pt7b);
  if( rtc_data.flag == RTC_FLAG){
    display_data( rtc_data.payload_data, rtc_data.last_reception_hour, rtc_data.last_reception_min);
  } else {
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.setCursor(0,30);
    display.print("Esperant...");
    display.display();
    Serial.println("Esperant");
  }
}

void loop() {
  if (radio.available()) {
    radio.read( &payload_data, sizeof(payload_data ));
    Serial.print("Temp:"); Serial.println( payload_data.temp);
    Serial.print("Volts:"); Serial.println( payload_data.volts);
    Serial.print("sleep_mins:"); Serial.println( payload_data.sleep_mins);

    connectWifi(); // also gets time from NTP server
    time_t now;
    now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime( &now);
    display_data( payload_data, timeinfo->tm_hour, timeinfo->tm_min); // to comment out

    char temp[8], hum[8], pres[8], volts[8];
    sprintf( temp, "%.1f", payload_data.temp/10.0F);
    sprintf( hum, "%.1f", payload_data.hum/10.0F);
    sprintf( pres, "%.1f", payload_data.pres/10.0F);
    sprintf( volts, "%.3f", payload_data.volts/1000.0F);
    StaticJsonDocument<200> doc;
    doc["time"] = millis();
    doc["temp"] = temp;
    doc["hum"] = hum;
    doc["presio"] = pres;
    doc["volts"] = volts;
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer); // print to client
    connectAWS();
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
    Serial.println("Message sent");
    Serial.println( jsonBuffer);

    store_rtc_data( payload_data, timeinfo->tm_hour, timeinfo->tm_min);

    delay(5000);
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    int sleep_mins = payload_data.sleep_mins ? payload_data.sleep_mins : 5;
    int64_t sleep_microsecs = 60*850000; // million reduced to 85%
    sleep_microsecs *= sleep_mins;
    Serial.print("sleep_microsecs:");
    Serial.println(sleep_microsecs);
    
    ESP.deepSleep( sleep_microsecs, WAKE_RF_DEFAULT);
  } else if( millis() >= millisecs_to_dim){
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    delay(500);
  }
}

void display_data( PayloadData payload_data, int last_reception_hour, int last_reception_min)
{
  char last_reception_time[15];
  char temp[8], hum[8], pres[8];
  sprintf( last_reception_time, "Last: %d:%02d", last_reception_hour, last_reception_min);
  Serial.println( last_reception_time);
  sprintf( temp, "%.1f", payload_data.temp/10.0F);
  sprintf( hum, "%.1f", payload_data.hum/10.0F);
  sprintf( pres, "%.1f", payload_data.pres/10.0F);

  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setCursor(0,15);
  display.print("Temp:"); display.println( temp);
  display.setCursor(0,30);
  display.print("Hum: "); display.println( hum);
  display.setCursor(0,45);
  display.print("Pres:"); display.println( pres);
  display.setCursor(0,60);
  display.println( last_reception_time);
  display.display();
}

void store_rtc_data( PayloadData payload_data, int last_reception_hour, int last_reception_min)
{
  rtc_data = {
    .flag = RTC_FLAG,
    .payload_data = payload_data,
    .last_reception_hour = last_reception_hour,
    .last_reception_min = last_reception_min
  };
  ESP.rtcUserMemoryWrite( 0, (uint32_t *)&rtc_data, sizeof( rtc_data));
}

void read_rtc_data()
{
  ESP.rtcUserMemoryRead( 0, (uint32_t *)&rtc_data, sizeof( rtc_data));
}
