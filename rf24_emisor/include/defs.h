#define DEBUG
#define SLEEP_MINUTES 15
#define SLEEP_MINUTES_LONG 60
#define VOLTAGE_THRESHOLD 2900
#define RF_FLAG 2020

const byte address[6] = "00001";
RF24 radio(9, 10); // CE, CSN
Adafruit_BME280 bme;

struct {
  int16_t temp;
  int16_t hum;
  int16_t pres;
  int16_t sleep_mins;
  int16_t volts;
  int16_t flag;
  } payload_data;

int16_t readVcc(void);
void sleep_secs(int16_t secs);