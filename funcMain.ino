void checkFirmwareUpdate() {
  if (!NO_AUTO_UPDATE && !NO_INTERNET) {
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
    analogWrite(LED_EXTERNAL, 33);
    callToServer(urlString);
    analogWrite(LED_EXTERNAL, 3);
  } else {
    writeLocalBuffer(urlString);
  }
}

boolean callToServer(String urlString) {
  if (NO_INTERNET) {
    NO_INTERNET = false;

    return bufferReadAndSend();
  }
    
  Serial.println(urlString);
  
  HTTPClient http; 
  http.begin("https://iot.osmo.mobi/send", OsMoSSLFingerprint);
  http.setUserAgent(deviceName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(urlString);
  Serial.println("Sending to server...");
  if (httpCode != HTTP_CODE_OK) {
    NO_SERVER = true;
    bufferWrite(urlString);
    return false;
  }
  NO_SERVER = false;
  String payload = http.getString();
  Serial.print(String(httpCode) + ": ");
  Serial.println(payload);
  http.end();

//  digitalWrite(LED_BUILTIN, LOW);
//  ticker2.attach_ms(6000, tickExternal, MAIN_MODE_NORMAL);

  return true;
}

boolean writeLocalBuffer(String urlString) {
  if (!NO_INTERNET) {
    NO_INTERNET = true;

    Serial.println("NO INTERNET MODE ACTIVATED");
  }

  return bufferWrite(urlString);
}
