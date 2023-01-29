void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  NTPConnect();
  wifiClient.setTrustAnchors( &cert_ca);
  wifiClient.setClientRSACert( &client_crt, &key);
}

void connectAWS()
{
  client.setServer( AWS_MQTT_HOST, 8883);
  // client.setCallback( messageReceived);
 
  Serial.println("Connecting to AWS IOT");
  while (!client.connect( THINGNAME)) {
    // Serial.print(".");
    char err_buf[256];
    wifiClient.getLastSSLError(err_buf, sizeof(err_buf));
    Serial.println(err_buf);
    delay(1000);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  // client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime( TIME_ZONE * 3600, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  configTime( 1, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  time_t now;
  time_t nowish = 1510592825;
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" done!");
  struct tm timeinfo;
  gmtime_r( &now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print( asctime( &timeinfo));
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