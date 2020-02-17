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
  File bufferFile = SPIFFS.open("/data.buff", "a+");
  if (bufferFile) {
    Serial.println("Write to local buffer file...");
    Serial.println(urlString);
    
    bufferFile.println(urlString);
    bufferFile.close();
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
