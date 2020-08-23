void taskRestart(int currentMillis, int previousMillisReboot) {
  if (!CHIP_TEST) {
    if (currentMillis - previousMillisReboot > REBOOT_INTERVAL) {
      Serial.println("It`s time to reboot");
      if (!NO_INTERNET && !NO_SERVER) {
        ESP.restart();
      } else {
        Serial.println("But it`s impossible, no internet connection");
      }
    }
  }
}

int taskConfig(int currentMillis, int previousMillisConfig) {
  if (currentMillis - previousMillisConfig > RECONFIG_INTERVAL) {
    getDeviceConfiguration();
    return currentMillis;
  }
}

void mainProcess() {
  if (MODE_SEND_BUFFER) {
    if (bufferReadAndSend("data")) {
      MODE_SEND_BUFFER = false;
    }
  }

  // BME-280
  float t1;
  float h1;
  float p1;
  
  // GY-21
  float t2;
  float h2;

  // BME-280
  if (CHIP_TEST) {
    p1 = 760.25;
    t1 = 25.2;
    h1 = 65.93;    
  } else {
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(p1, t1, h1, tempUnit, presUnit);
    p1 = p1 / 133.3224;
  }
  
  // GY-21
  if (CHIP_TEST) {
    t2 = 20.5;
    h2 = 60.25;
  } else {
    t2 = myHumidity.readTemperature();
    h2 = myHumidity.readHumidity();
  }

  time_t now = time(nullptr);
  String urlString = 
    "token=" + String(TOKEN) + "&" +
    "t1=" + String(t1) + "&" +
    "h1=" + String(h1) + "&" +
    "p1=" + String(p1) + "&" +
    "t2=" + String(t2) + "&" +
    "h2=" + String(h2) + "&" +
    "millis=" + String(millis()) + "&" + 
    "time=" + String(time(&now));  
  actionDo(urlString);

  String string = 
    "t1" + String(t1) +
    "h1" + String(h1) +
    "p1" + String(p1) +
    "t2" + String(t2) +
    "h2" + String(h2) +
    "M" + String(millis()) +
    "T" + String(time(&now));
  if (NO_SERVER) {
    bufferWrite("udp", string);
  } else {
    callServer("D", String(TOKEN), string);
  }
}

void checkFirmwareUpdate() {
  if (!NO_AUTO_UPDATE && !NO_INTERNET) {
    t_httpUpdate_return ret = ESPhttpUpdate.update(TW_UPDATE_SERVER, DEVICE_FIRMWARE);
    
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

void actionDo(String urlString) {
  if (WiFi.status() == WL_CONNECTED) {
    analogWrite(LED_EXTERNAL, 3);
    callToServer(urlString);
    analogWrite(LED_EXTERNAL, LED_BRIGHT);
  } else {
    analogWrite(LED_EXTERNAL, LED_BRIGHT);
    writeLocalBuffer(urlString);
    analogWrite(LED_EXTERNAL, 3);
  }
}

boolean callToServer(String urlString) {
  if (NO_INTERNET) {
    NO_INTERNET = false;
    bufferReadAndSend("udp");
    return bufferReadAndSend("data");
  }
    
  Serial.println(urlString);
  
  HTTPClient http; 
  http.begin(OSMO_HTTP_SERVER_SEND, OsMoSSLFingerprint);
  http.setUserAgent(deviceName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(urlString);
  Serial.println("Sending to server...");
  if (httpCode != HTTP_CODE_OK) {
    NO_SERVER = true;
    bufferWrite("data", urlString);
    return false;
  }
  NO_SERVER = false;
  String payload = http.getString();
  Serial.print(String(httpCode) + ": ");
  Serial.println(payload);
  http.end();

  return true;
}

void pingServer() {
  callServer("P", String(int(millis()/5000)) + "|" + String(WiFi.RSSI()), "");
  Serial.println("Ping");
}

void callServer(String command, String string, String data) {
  udp.beginPacket(OSMO_SERVER_HOST, OSMO_SERVER_PORT);
  if (data != "") {
    udp.print(command + ":" + string + "|" + data);
  } else if (string != "") {
    udp.print(command + ":" + string);
  } else {
    udp.print(command);
  }
  udp.endPacket();
}

boolean writeLocalBuffer(String urlString) {
  if (!NO_INTERNET) {
    NO_INTERNET = true;
    Serial.println("NO INTERNET MODE ACTIVATED");
  }

  return bufferWrite("data", urlString);
}
