void setup() 
{  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_EXTERNAL, OUTPUT);
  pinMode(RESET_WIFI, INPUT_PULLUP);

  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}
    
  Serial.println("Device '" + deviceName + "' is starting...");

  if (WiFi.SSID() != "") {
    Serial.print("Current Saved WiFi SSID: ");
    Serial.println(WiFi.SSID());

    // reset wifi by RESET_WIFI pin to GROUND
    int resetCycle = 0;
    ticker1.attach_ms(35, tickInternal);
    while (resetCycle < 50) {
      MODE_RESET_WIFI = digitalRead(RESET_WIFI);
      if (MODE_RESET_WIFI == LOW) {
        resetWiFiSettings();
        break;
      }
      resetCycle++;
      delay(36);
    }
    // end reset wifi
  } else {
    Serial.println("We dont have saved WiFi settings, need configure");
  }
  
  WiFi.hostname(deviceName);
  
  ticker1.attach_ms(100, tickInternal);
  ticker2.attach_ms(100, tickExternal, MAIN_MODE_OFFLINE);

  if (!setupWiFiManager()) {
    Serial.println("failed to connect and hit timeout");
    delay(15000);
    ESP.restart();
  } else {
    Serial.println("WiFi network connected");
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
    int syncSecs = 0;
    configTime(0, 0, "pool.ntp.org");  
    setenv("TZ", "GMT+0", 0);
    while(time(nullptr) <= 100000) {
      if (syncSecs > 15) {
        return ESP.restart();
      }
      
      Serial.print(" .");
      syncSecs++;
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
      "vcc=" + String(ESP.getVcc()) + "&" +
      "bufferCount=" + String(bufferCount());
    Serial.println(postData);

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(10) + 60;
    DynamicJsonDocument doc(capacity);

    HTTPClient http;
    http.begin("http://iot.osmo.mobi/device");
    http.setUserAgent(deviceName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    if (httpCode != HTTP_CODE_OK && !CHIP_TEST) {
      ticker1.attach_ms(200, tickInternal);
      ticker2.attach_ms(500, tickExternal, MAIN_MODE_OFFLINE);
      
      Serial.println("Error init device from OsMo.mobi");
      delay(15000);
      return ESP.restart();
    }
    Serial.println("get device config and set env, result: " + String(httpCode));
    if (httpCode == HTTP_CODE_OK) {
      NO_SERVER = false;
    }
    
    String payload = http.getString();
    deserializeJson(doc, payload);
    http.end();

    if (doc["interval"].as<int>() > 4) {
      int SENS_INTERVAL_NEW = doc["interval"].as<int>() * 1000;
      if (SENS_INTERVAL != SENS_INTERVAL_NEW) {
        SENS_INTERVAL = SENS_INTERVAL_NEW;
        writeCfgFile("interval", doc["interval"].as<String>());
      }
    }

    if (OsMoSSLFingerprint != doc["tlsFinger"].as<String>()) {
      OsMoSSLFingerprint = doc["tlsFinger"].as<String>();
      writeCfgFile("ssl", OsMoSSLFingerprint);
      Serial.print("tlsFinger was updated in SPIFFS");
    }

    if (TOKEN != doc["token"].as<String>()) {
      TOKEN = doc["token"].as<String>();
      writeCfgFile("token", TOKEN);
      Serial.print("Token was updated in SPIFFS");
    }

//    if (LED_BRIGHT != doc["led_bright"].as<int>()) {
//      LED_BRIGHT = doc["led_bright"].as<int>();
//      writeCfgFile("led_bright", TOKEN);
//      Serial.print("Led intersivity was updated in SPIFFS");
//    }

    tickOffAll();
    ticker1.attach_ms(100, tickInternal);

    if (bufferCount() > 0) {
      Serial.println();
      Serial.println("Buffer count: " + bufferCount());
      MODE_SEND_BUFFER = true;
    }

    if (!CHIP_TEST) {
      Wire.begin();

      tickOffAll();
      ticker1.attach_ms(2000, tickInternal);
      ticker2.attach_ms(2000, tickExternal, MAIN_MODE_NORMAL);
      while(!bme.begin())
      {
        Serial.println("Could not find BME-280 sensor!");
        delay(2000);
      }
  
      tickOffAll();
      ticker1.attach_ms(4000, tickInternal);
      ticker2.attach_ms(4000, tickExternal, MAIN_MODE_NORMAL);
      myHumidity.begin();
    }

    checkFirmwareUpdate();
  }

  tickOffAll();
  
  analogWrite(LED_EXTERNAL, 255);
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  analogWrite(LED_EXTERNAL, 25);
}
