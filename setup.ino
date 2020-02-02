void setup() 
{  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  WiFi.hostname(deviceName);
  
  ticker.attach_ms(100, tickBack);
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

    checkFirmwareUpdate();

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
      "token=" + String(DEVICE_REVISION) + String(DEVICE_ID) + "&" +
      "model=" + String(DEVICE_MODEL) + "&" +
      "ip=" + WiFi.localIP().toString() + "&" +
      "mac=" + String(WiFi.macAddress()) + "&" +
      "apmac=" + String(WiFi.softAPmacAddress()) + "&" +
      "ssid=" + String(WiFi.SSID()) + "&" +
      "rssi=" + String(WiFi.RSSI()) + "&" +
      "chipid=" + String(ESP.getFlashChipId()) + "&" +
      "vcc=" + String(ESP.getVcc()/1000.0);

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(10) + 60;
    DynamicJsonDocument doc(capacity);

    HTTPClient http;
    http.begin("http://iot.osmo.mobi/device");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    if (httpCode != 200 && !CHIP_TEST) {
        ticker.attach_ms(200, tickBack);
        ticker.attach_ms(500, tickFront, MAIN_MODE_OFFLINE);
        
        Serial.println("Error init device from OsMo.mobi");
        while(1) {
          delay(60000);
        }
    }
    if (httpCode == 200) {
      NO_SERVER = false;
    }
    Serial.println("get device config and set env, result: " + String(httpCode));
    String payload = http.getString();
    deserializeJson(doc, payload);
    http.end();

    Serial.print(F("Interval: "));
    Serial.println(doc["interval"].as<int>());
    if (doc["interval"].as<int>() > 4) {
      SENS_INTERVAL = doc["interval"].as<int>() * 1000;

      writeCfgFile("interval", doc["interval"].as<String>());

//      intervalFile = SPIFFS.open(F("/interval.cfg"), "w");
//      if (intervalFile) {
//        Serial.println("Write file content!");
//        bytesWriten = intervalFile.print(SENS_INTERVAL);
//        if (bytesWriten > 0) {
//          Serial.print("File was written: ");
//          Serial.println(bytesWriten);
//        } else {
//          Serial.println("File write failed");
//        }
//        intervalFile.close();
//      }
    }

    Serial.print("SHA-1 FingerPrint for SSL KEY: ");
    Serial.println(doc["tlsFinger"].as<String>());
    OsMoSSLFingerprint = doc["tlsFinger"].as<String>();
    writeCfgFile("ssl", OsMoSSLFingerprint);
//    sslFingerprintFile = SPIFFS.open(F("/ssl.cfg"), "w");
//    if (sslFingerprintFile) {
//      Serial.println("Write file content!");
//      bytesWriten = sslFingerprintFile.print(OsMoSSLFingerprint);
//      if (bytesWriten > 0) {
//        Serial.print("File was written: ");
//        Serial.println(bytesWriten);
//      } else {
//        Serial.println("File write failed");
//      }
//      sslFingerprintFile.close();
//    }


    Serial.print("TOKEN: ");
    Serial.println(doc["token"].as<String>());
    TOKEN = doc["token"].as<String>();
    writeCfgFile("token", TOKEN);
    
//    tokenFile = SPIFFS.open(F("/token.cfg"), "w");
//    if (tokenFile) {
//      Serial.println("Write file content!");
//      bytesWriten = tokenFile.print(TOKEN);
//      if (bytesWriten > 0) {
//        Serial.print("File was written: ");
//        Serial.println(bytesWriten);
//      } else {
//        Serial.println("File write failed");
//      }
//      tokenFile.close();
//    }

    ticker.detach();
    ticker.attach_ms(100, tickBack);

    if (!CHIP_TEST) {
      Wire.begin();

      ticker.detach();
      ticker.attach_ms(2000, tickBack);
      ticker.attach_ms(2000, tickFront, MAIN_MODE_NORMAL);
      while(!bme.begin())
      {
        Serial.println("Could not find BME-280 sensor!");
        Serial.println(millis());
        delay(2000);
      }
  
      ticker.detach();
      ticker.attach_ms(3000, tickBack);
      ticker.attach_ms(3000, tickFront, MAIN_MODE_NORMAL);
      myHumidity.begin();
    }
  }

  ticker.detach();

  digitalWrite(BUILTIN_LED, HIGH);
}