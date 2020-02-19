// core functions
bool getDeviceConfiguration() {
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
    ESP.restart();
    return false;
  }
  Serial.println("get device config and set env, result: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK) {
    NO_SERVER = false;
  } else {
    return false;
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

//  if (LED_BRIGHT != doc["led_bright"].as<int>()) {
//    LED_BRIGHT = doc["led_bright"].as<int>();
//    writeCfgFile("led_bright", TOKEN);
//    Serial.print("Led intersivity was updated in SPIFFS");
//  }
  return true;
}

// bufferFile functions
int bufferCount() 
{
  int countLine = 0;
  File bufferFile = SPIFFS.open("/data.buff", "r");
  char buffer[256];
  while (bufferFile.available()) {
    int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[l] = 0;
    countLine++;
  }
  bufferFile.close();
  return countLine;
}

bool bufferWrite(String urlString) {
  analogWrite(LED_EXTERNAL, 0);
  File bufferFile = SPIFFS.open("/data.buff", "a+");
  if (bufferFile) {
    Serial.println("Write to local buffer file...");
    Serial.println(urlString);
    
    bufferFile.println(urlString);
    bufferFile.close();
    analogWrite(LED_EXTERNAL, 3);
    return true;
  }
  Serial.println("Buffer file open failed");
  return false;
}

int bufferReadAndSend() {
  File bufferFile = SPIFFS.open("/data.buff", "r+");
  if (bufferFile) {
    int until = bufferCount();
    
    HTTPClient http; 
    http.begin("https://iot.osmo.mobi/sendPack", OsMoSSLFingerprint);
    http.addHeader("Content-Type", "text/plain");
    http.setTimeout(15000);
    http.setUserAgent(deviceName);
    
    bufferFile.seek(0, SeekSet);
    Serial.print("Buffer size: ");
    Serial.print(bufferFile.size());
    Serial.println();

    int rowsCountAll = 0;
    int rowsCount = 0;
    String toSend = "";
    char buffer[128];
    while (bufferFile.available()) {
      int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
      buffer[l] = 0;

      toSend += buffer + String("\n ");
      rowsCount++;
      rowsCountAll++;
      
      if (rowsCount >= 10 || rowsCountAll >= until) {
        HTTPClient http; 
        http.begin("https://iot.osmo.mobi/sendPack", OsMoSSLFingerprint);
        http.setReuse(false);
        http.addHeader("Content-Type", "text/plain");
        http.setTimeout(15000);
        http.setUserAgent(deviceName);
    
        Serial.println("SEND part of buffer");
        int httpCode = http.POST(toSend);
        Serial.println(httpCode);
        http.end();
        if (httpCode == HTTP_CODE_OK) {
          toSend = "";
          rowsCount = 0;

          if (rowsCountAll >= until) {
            Serial.println("Delete buffer file");
            bufferFile.close();
            SPIFFS.remove("/data.buff");
            return true;
          }
        }
      }
    }
  }

  return -1;
}

// LEDs functions
void tickInternal()
{
  int stateIntervalLed = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !stateIntervalLed);
}

void tickExternal(int mode)
{
  int stateExternalLed = 0;
  switch (mode) {
    case MAIN_MODE_NORMAL:
    case MAIN_MODE_OFFLINE:
    case MAIN_MODE_FAIL:
      stateExternalLed = digitalRead(LED_EXTERNAL);
      digitalWrite(LED_EXTERNAL, !stateExternalLed);
      break;
    default: 
      stateExternalLed = digitalRead(LED_EXTERNAL);
      digitalWrite(LED_EXTERNAL, !stateExternalLed);
  }
}

void tickOffAll()
{
  ticker1.detach();
  ticker2.detach();
}
