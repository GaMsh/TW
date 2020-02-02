void checkFirmwareUpdate() {
  if (!CHIP_TEST && !NO_INTERNET) {
    t_httpUpdate_return ret = ESPhttpUpdate.update("http://tw.gamsh.ru", DEVICE_FIRMWARE);
    
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[update] Update no Update.");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("[update] Update ok.");
        ESP.restart();
        break;
    }
  }
}

void mainProcess(String urlString) {
  if (WiFi.status() == WL_CONNECTED) {
    callToServer(urlString);
  } else {
    writeLocalBuffer(urlString);
  }
}

boolean callToServer(String urlString) {
  if (NO_INTERNET) {
    NO_INTERNET = false;

    Serial.println("NO INTERNET MODE DEACTIVATED");
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
    String payload = http.getString();
    Serial.print(String(httpCode) + ": ");
    Serial.println(payload);
    http.end();

    return true;
  }
    
  Serial.println("Sending to server...");
  Serial.println(urlString);
  
  HTTPClient http; 
  http.begin("https://iot.osmo.mobi/send", OsMoSSLFingerprint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(urlString);
  String payload = http.getString();
  Serial.print(String(httpCode) + ": ");
  Serial.println(payload);
  http.end();

  digitalWrite(LED_BUILTIN, LOW);
  ticker.attach_ms(2000, tickFront, MAIN_MODE_NORMAL);

  return true;
}

boolean writeLocalBuffer(String urlString) {
  if (!NO_INTERNET) {
    NO_INTERNET = true;

    Serial.println("NO INTERNET MODE ACTIVATED");
    bufferFile = SPIFFS.open("/buffer.txt", "a+");
  }
  if (bufferFile) {
    Serial.println("Write to local buffer file...");
    Serial.println(urlString);
    
    bufferFile.println(urlString);
  } else {
    Serial.println("Buffer file open failed");
    return false;
  }

  return true;
}

String readAndClearLocalBuffer() {

}
