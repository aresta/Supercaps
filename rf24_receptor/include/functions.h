void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  #ifdef DEBUG
    Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
  #endif
  while (WiFi.status() != WL_CONNECTED)
  {
    #ifdef DEBUG
      Serial.print(".");
    #endif
    delay(1000);
  }
  NTPConnect();
  wifiClient.setTrustAnchors( &cert_ca);
  wifiClient.setClientRSACert( &client_crt, &key);
}

void connectAWS()
{
  if(client.connected()) return;
  client.setServer( AWS_MQTT_HOST, 8883);
  // client.setCallback( messageReceived);
 
  #ifdef DEBUG
    Serial.println("Connecting to AWS IOT");
  #endif
  while (!client.connect( THINGNAME)) {
    // Serial.print(".");
    char err_buf[256];
    wifiClient.getLastSSLError(err_buf, sizeof(err_buf));
    #ifdef DEBUG
      Serial.println(err_buf);
    #endif
    delay(800);
  }
 
  if (!client.connected()) {
    #ifdef DEBUG
      Serial.println("AWS IoT Timeout!");
    #endif
    return;
  }
  // Subscribe to a topic
  // client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  #ifdef DEBUG
    Serial.println("AWS IoT Connected!");
  #endif
}

void NTPConnect(void)
{
  #ifdef DEBUG
    Serial.print("Setting time using SNTP");
  #endif
  configTime( TIME_ZONE * 3600, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  configTime( 1, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  time_t now;
  time_t nowish = 1510592825;
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
    now = time(nullptr);
  }
  #ifdef DEBUG
    Serial.println(" done!");
  #endif
  struct tm timeinfo;
  gmtime_r( &now, &timeinfo);
  #ifdef DEBUG
    Serial.print("Current time: ");
    Serial.print( asctime( &timeinfo));
  #endif
}

// void messageReceived(char *topic, byte *payload, unsigned int length)
// {
//   Serial.print("Received [");
//   Serial.print(topic);
//   Serial.print("]: ");
//   for (int i = 0; i < length; i++)
//   {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println();
// }