#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <time.h>
#include <printf.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include "LowPower.h"
#include "defs.h"

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.print("VCC:"); Serial.println( readVcc());
  #endif
  // sleep_secs(2*60); // to avoid startup rippling
  // Serial.println("Starting RF24");
  if( !radio.begin()){
    #ifdef DEBUG
      Serial.println("RF24 Start error!");
      printf_begin(); 
      radio.printPrettyDetails();
    #endif
    while(1){};
  }
  #ifdef DEBUG
    Serial.println("RF24 Started!");
  #endif

  radio.openWritingPipe( address);
  radio.setChannel(2);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printPrettyDetails(); // (larger) function that prints human readable data
  bme.begin(0x76);   // 0x77 per l'altre sensor
  bme.setSampling(
    Adafruit_BME280::MODE_FORCED,
    Adafruit_BME280::SAMPLING_X1, // temperature
    Adafruit_BME280::SAMPLING_X1, // pressure
    Adafruit_BME280::SAMPLING_X1, // humidity
    Adafruit_BME280::FILTER_OFF );
}

void loop() {
  int sleep_time;
  #ifdef DEBUG
    Serial.print("VCC:"); Serial.println( readVcc());
  #endif
  delay(100);
  bme.takeForcedMeasurement();
  payload_data.temp       = int( bme.readTemperature()*10 + 0.5);  // we keep 3 significant digits in a int, 235 -> 23.5 C
  payload_data.hum        = int( bme.readHumidity()*10 + 0.5);
  payload_data.pres       = int(( bme.readPressure() / 10.0F) + 0.5);
  payload_data.volts      = readVcc();
  sleep_time = payload_data.volts > VOLTAGE_THRESHOLD ? SLEEP_MINUTES : SLEEP_MINUTES_LONG;
  payload_data.sleep_mins = sleep_time;

  #ifdef DEBUG
    Serial.print("temp:"); Serial.println(payload_data.temp);
    Serial.print("pres:"); Serial.println(payload_data.pres);
    Serial.print("hum:"); Serial.println(payload_data.hum);
    Serial.print("volts:"); Serial.println(payload_data.volts);
    Serial.print("sleep mins:"); Serial.println(payload_data.sleep_mins);
    Serial.println("--------------");
    Serial.print("size of:"); Serial.println(sizeof(payload_data));
    Serial.print("temp2:"); Serial.println(payload_data.temp);
  #endif

  delay(300);
  if( radio.write( &payload_data, sizeof(payload_data))){
    Serial.println("radio write ok");
  } else {
    Serial.println("radio write error");
  };
  #ifdef DEBUG
    Serial.println("---------------------------");
    Serial.print("Temp:"); Serial.println( payload_data.temp);
    Serial.print("VCC:"); Serial.println( readVcc());
    Serial.print("temp:"); Serial.println(payload_data.temp);
    Serial.print("pres:"); Serial.println(payload_data.pres);
    Serial.print("hum:"); Serial.println(payload_data.hum);
    Serial.print("volts:"); Serial.println(payload_data.volts);
    Serial.print("sleep mins:"); Serial.println(payload_data.sleep_mins);
  #endif
  sleep_secs( sleep_time * 60 );
}

void sleep_secs(int secs)
{
  radio.powerDown();
  int loops = (secs+4)/8;  // aprox
  for(int i=0; i < loops; i++){ // 112 ~ 15 min
    LowPower.powerDown( SLEEP_8S, ADC_OFF, BOD_OFF); 
  }
  radio.powerUp();
}

int readVcc(void) {
  int result;
  ADCSRA = (1<<ADEN);  //enable and
  ADCSRA |= (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);  // set prescaler to 128

  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = (1<<REFS0) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1);
  
  delay(5); // Wait for ADC and Vref to settle
  ADCSRA |= (1<<ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // wait until done
  result = ADC;
  
  // second time is a charm
  ADCSRA |= (1<<ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // wait until done
  result = ADC;
  
  // must be individually calibrated for EACH BOARD
  result = 1124942UL / (unsigned long)result; //1126400 = 1.1*1024*1000
  return result; // Vcc in millivolts
}
