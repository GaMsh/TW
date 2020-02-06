// bufferFile functions
int bufferCount() 
{
  int countLine = 0;
  bufferFile = SPIFFS.open("/data.buff", "r");
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
  bufferFile = SPIFFS.open("/data.buff", "a+");
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
  bufferFile = SPIFFS.open("/data.buff", "r");
  if (bufferFile) {
    bufferFile.seek(0, SeekSet);
    Serial.print("Buffer size: ");
    Serial.print(bufferFile.size());
    Serial.println();

    String toSend = "";
    char buffer[256];
    while (bufferFile.available()) {
      int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
      buffer[l] = 0;
      toSend += buffer + String("\n");
    }

    Serial.println(toSend);
    Serial.println();

    bufferFile.close();

    HTTPClient http; 
    http.begin("https://iot.osmo.mobi/sendPack", OsMoSSLFingerprint);
    http.addHeader("Content-Type", "text/plain");
  
    int httpCode = http.POST(toSend);
    if (httpCode == 200) {
      SPIFFS.remove("/buffer.txt");
    }
    String payload = http.getString();
    Serial.print(String(httpCode) + ": ");
    Serial.println(payload);
    http.end();
  }
}

// LEDs functions
void tickBack()
{
  int stateBack = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !stateBack);
}

void tickFront(int mode)
{
  int stateFront = 0;
  switch (mode) {
    case MAIN_MODE_NORMAL:
      stateFront = digitalRead(LED_GREEN);
      digitalWrite(LED_GREEN, !stateFront);
      break;
    case MAIN_MODE_OFFLINE:
//      stateFront = digitalRead(LED_YELLOW);
//      digitalWrite(LED_YELLOW, !stateFront);
      break;
    case MAIN_MODE_FAIL:
//      stateFront = digitalRead(LED_RED);
//      digitalWrite(LED_RED, !stateFront);
      break;
    default: 
      stateFront = digitalRead(LED_GREEN);
      digitalWrite(LED_GREEN, !stateFront);
  }
}

void tickOffAll()
{
  ticker.detach();
  tickerBack.detach();
}
