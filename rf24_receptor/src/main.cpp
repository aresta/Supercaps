#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
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
// RTCData rtc_data;
unsigned long millisecs_to_dim = 15*1000;

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
  #endif
  // Serial.println("Starting");
  if( !radio.begin()){
    #ifdef DEBUG
      Serial.print("radio.begin() Error");
    #endif
    while(1){};
  }
  #ifdef DEBUG
    Serial.println("RF24 Started");
  #endif
  radio.setChannel(109);
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
    #ifdef DEBUG
      Serial.println("Error starting display");
    #endif
    while (true);
  }

  read_rtc_data();
  display.setTextWrap(false); 
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&FreeSans9pt7b);

  connectWifi(); // also gets time from NTP server
  if( rtc_data.flag == RTC_FLAG){
    display_data( rtc_data.payload_data, rtc_data.last_reception_hour, rtc_data.last_reception_min);
    log_iot( CHANNEL_INFO, "Wakeup");
  } else {
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.setCursor(0,30);
    display.print("Esperant...");
    display.display();
    log_iot( CHANNEL_INFO, "Startup");
    #ifdef DEBUG
      Serial.println("Esperant");
    #endif
  }
}

void loop() {
  if (radio.available()) {
    radio.read( &payload_data, sizeof(payload_data ));
    if( payload_data.flag != RF_FLAG){ // reception wrong
      log_iot( CHANNEL_ERROR, "Reception error");
      #ifdef DEBUG
        Serial.println("Reception error 1");
        Serial.println(payload_data.flag);
      #endif
      delay(100);
      radio.read( &payload_data, sizeof(payload_data )); // reception wrong again
      if( payload_data.flag != RF_FLAG){
        #ifdef DEBUG
          Serial.println("Reception error 2");
          Serial.println(payload_data.flag);
        #endif
        ESP.deepSleep( 5*1000000, WAKE_RF_DEFAULT); // sleep 5 secs
      }
    }
    #ifdef DEBUG
      Serial.print("Temp:"); Serial.println( payload_data.temp);
      Serial.print("Volts:"); Serial.println( payload_data.volts);
      Serial.print("sleep_mins:"); Serial.println( payload_data.sleep_mins);
    #endif
    // connectWifi(); // also gets time from NTP server
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
    send_iot_data( volts, hum, pres, temp);
    store_rtc_data( payload_data, timeinfo->tm_hour, timeinfo->tm_min);

    delay(2000);
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    int sleep_mins = payload_data.sleep_mins ? payload_data.sleep_mins : 5;
    int64_t sleep_microsecs = 60*800000; // million reduced to 80%
    sleep_microsecs *= sleep_mins;
    #ifdef DEBUG
      Serial.print("sleep_microsecs:");
      Serial.println(sleep_microsecs);
    #endif
    log_iot( CHANNEL_INFO, "Sleeping...");
    ESP.deepSleep( sleep_microsecs, WAKE_RF_DEFAULT);
  } else if( millis() >= millisecs_to_dim){
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    delay(50);
  }
}

void display_data( PayloadData payload_data, int last_reception_hour, int last_reception_min)
{
  char last_reception_time[15];
  char temp[8], hum[8], pres[8];
  sprintf( last_reception_time, "Last: %d:%02d", last_reception_hour, last_reception_min);
  #ifdef DEBUG
    Serial.println( last_reception_time);
  #endif
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

void send_iot_data( const char volts[], const char hum[], const char pres[], const char temp[])
{
  StaticJsonDocument<200> doc;
  doc["temp"] = temp;
  doc["hum"] = hum;
  doc["presio"] = pres;
  doc["volts"] = volts;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  connectAWS();
  client.publish( AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  delay(200);
}

void log_iot( int channel, const char msg[])
{
  StaticJsonDocument<200> doc;
  // doc["channel"]  = channel;
  doc.add( msg );
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  connectAWS();
  client.publish( (channel == CHANNEL_ERROR ? AWS_IOT_PUBLISH_LOG_ERROR : AWS_IOT_PUBLISH_LOG_INFO), jsonBuffer);
  delay(200);
}