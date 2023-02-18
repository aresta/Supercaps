#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <printf.h>
#include <Adafruit_BME280.h>
#include "LowPower.h"
#include "defs.h"

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("--------------");
    // Serial.print("VCC:"); Serial.println( readVcc());
  #endif
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
  radio.setChannel(109);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);
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
  #ifdef DEBUG
    Serial.println("- - - -");
    Serial.println("Loop start");
  #endif
  delay(200);
  bme.takeForcedMeasurement();
  payload_data.temp       = int16_t( bme.readTemperature()*10 + 0.5);  // we keep 3 significant digits in a int, 235 -> 23.5 C
  payload_data.hum        = int16_t( bme.readHumidity()*10 + 0.5);
  payload_data.pres       = int16_t(( bme.readPressure() / 10.0F) + 0.5);
  payload_data.volts      = readVcc();
  payload_data.sleep_mins = ( payload_data.volts > VOLTAGE_THRESHOLD ? SLEEP_MINUTES : SLEEP_MINUTES_LONG );
  payload_data.flag       = RF_FLAG;

  #ifdef DEBUG
    Serial.print("temp:"); Serial.println(payload_data.temp);
    Serial.print("pres:"); Serial.println(payload_data.pres);
    Serial.print("hum:"); Serial.println(payload_data.hum);
    Serial.print("volts:"); Serial.println(payload_data.volts);
    Serial.print("sleep mins:"); Serial.println(payload_data.sleep_mins);
    delay(1000); // time to finish serial.print
  #endif

  delay(100);
  if( !radio.write( &payload_data, sizeof(payload_data))){
    #ifdef DEBUG
      Serial.println("radio write error 1");
    #endif
    delay(1000);
    sleep_secs(30);
    if( !radio.write( &payload_data, sizeof(payload_data))){
      #ifdef DEBUG
        Serial.println("radio write error 2");
      #endif
    }
    delay(1000); // time to finish serial.print
  };
  sleep_secs( payload_data.sleep_mins * 60 );
}

void sleep_secs(int16_t secs)
{
  radio.powerDown();
  int loops = (secs+4)/8;  // aprox
  for(int i=0; i < loops; i++){ // 112 ~ 15 min
    LowPower.powerDown( SLEEP_8S, ADC_OFF, BOD_OFF); 
  }
  radio.powerUp();
}

int16_t readVcc(void) {
  int16_t result;
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
