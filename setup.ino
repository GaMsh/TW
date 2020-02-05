void setup() 
{  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  WiFi.hostname(deviceName);
  
  tickerBack.attach_ms(100, tickBack);
  ticker.attach_ms(100, tickFront, MAIN_MODE_OFFLINE);
  
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}
  
  Serial.println("Device '" + deviceName + "' is starting...");

  if (!setupWiFiManager()) {
    Serial.println("failed to connect and hit timeout");
//    ESP.reset();
    ESP.restart();
    delay(1000);
  } else {
    Serial.println("connected...yeey :)");
    NO_INTERNET = false;

    if (SPIFFS.begin()) {
      Serial.println(F("SPIFFS was mounted"));
    } else {
      Serial.println(F("Error while mounting SPIFFS"));
    }
  
    int customInterval = readCfgFile("interval").toInt();
    if (customInterval > 4) {
      SENS_INTERVAL = customInterval * 1000;
    }
  
    String customSsl = readCfgFile("ssl");
    if (customSsl) {
      OsMoSSLFingerprint = customSsl;
    }
  
    TOKEN = readCfgFile("token");

    Serial.println("Syncing time...");
    configTime(0, 0, "pool.ntp.org");  
    setenv("TZ", "GMT+0", 0);
    while(time(nullptr) <= 100000) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();

    StaticJsonDocument<1024> jb;
    String postData = 
      "token=" + TOKEN + "&" +
      "revision=" + String(DEVICE_REVISION) + "&" +
      "model=" + String(DEVICE_MODEL) + "&" +
      "firmware=" + String(DEVICE_FIRMWARE) + "&"
      "ip=" + WiFi.localIP().toString() + "&" +
      "mac=" + String(WiFi.macAddress()) + "&" +
      "ssid=" + String(WiFi.SSID()) + "&" +
      "rssi=" + String(WiFi.RSSI()) + "&" +
      "chipid=" + String(ESP.getFlashChipId()) + "&" +
      "vcc=" + String(ESP.getVcc());
    Serial.println(postData);

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(10) + 60;
    DynamicJsonDocument doc(capacity);

    HTTPClient http;
    http.begin("http://iot.osmo.mobi/device");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    if (httpCode != 200 && !CHIP_TEST) {
        tickerBack.attach_ms(200, tickBack);
        ticker.attach_ms(500, tickFront, MAIN_MODE_OFFLINE);
        
        Serial.println("Error init device from OsMo.mobi");
        while(1) {
          delay(60000);
        }
    }
    Serial.println("get device config and set env, result: " + String(httpCode));
    if (httpCode == 200) {
      NO_SERVER = false;
    }
    
    String payload = http.getString();
    deserializeJson(doc, payload);
    http.end();

    Serial.print(F("Interval: "));
    Serial.println(doc["interval"].as<int>());
    if (doc["interval"].as<int>() > 4) {
      SENS_INTERVAL = doc["interval"].as<int>() * 1000;
      writeCfgFile("interval", doc["interval"].as<String>());
    }

    Serial.print("SHA-1 FingerPrint for SSL KEY: ");
    Serial.println(doc["tlsFinger"].as<String>());
    OsMoSSLFingerprint = doc["tlsFinger"].as<String>();
    writeCfgFile("ssl", OsMoSSLFingerprint);

    Serial.print("TOKEN: ");
    Serial.println(doc["token"].as<String>());
    TOKEN = doc["token"].as<String>();
    writeCfgFile("token", TOKEN);

    tickOffAll();
    tickerBack.attach_ms(100, tickBack);

    if (bufferCount() > 0) {
      Serial.println("");
      Serial.println("Buffer count: " + bufferCount());
    }

    if (!CHIP_TEST) {
      Wire.begin();

      tickOffAll();
      tickerBack.attach_ms(2000, tickBack);
      ticker.attach_ms(2000, tickFront, MAIN_MODE_NORMAL);
      while(!bme.begin())
      {
        Serial.println("Could not find BME-280 sensor!");
        delay(2000);
      }
  
      tickOffAll();
      tickerBack.attach_ms(3000, tickBack);
      ticker.attach_ms(3000, tickFront, MAIN_MODE_NORMAL);
      myHumidity.begin();
    }

    checkFirmwareUpdate();
  }

  tickOffAll();

  digitalWrite(BUILTIN_LED, HIGH);
}
