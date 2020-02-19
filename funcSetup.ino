void getTimeFromInternet() {
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
}

void resetWiFiSettings() {
  ticker1.attach_ms(512, tickInternal);
  Serial.println("WiFi reset by special PIN");
  WiFi.disconnect(true);
  delay(2000);
  ESP.reset();
  delay(1000);
  ESP.restart();
}

void checkWiFiConfiguration() {
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
}

bool setupWiFiManager() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setMinimumSignalQuality(33);
  wifiManager.setRemoveDuplicateAPs(true);
  wifiManager.setDebugOutput(false);
  wifiManager.setCustomHeadElement("<style>html{background:#fb7906};</style>");
  
  ////STATIC IP (if needed)
//  IPAddress ip(192, 168, 0, 125);
//  IPAddress gateway(192, 168, 0, 1);
//  IPAddress subnet(255, 255, 255, 0);
//  IPAddress dns1(8, 8, 8, 8);
//  IPAddress dns2(8, 8, 4, 4);
//  wifiManager.setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns1, IPAddress dns2)
  ////STATIC IP (if needed)

  if (wifiManager.autoConnect()) {
    return true;
  }

  if (WiFi.SSID() == "") {
    String wifiPortalSsid = deviceName + "_SETUP";
    wifiManager.startConfigPortal(wifiPortalSsid.c_str());
  }

  return false;
}

String readCfgFile(String configVar) {
  String result = "";
  
  File file = SPIFFS.open("/" + configVar + ".cfg", "r");
  if (file) {
    result = file.readString();
    Serial.print(configVar + ": ");
    Serial.println(result);
    file.close();
  } else {
    Serial.println("Problem read " + configVar + ".cfg");
  }
  
  return result;
}

int writeCfgFile(String configVar, String value) {
  File file = SPIFFS.open("/" + configVar + ".cfg", "w");
  if (file) {
    Serial.println("Write file " + configVar + ".cfg");
    bytesWriten = file.print(value);
    if (bytesWriten > 0) {
      Serial.print("File was written: ");
      Serial.println(bytesWriten);
      return bytesWriten;
    } else {
      Serial.println("File write failed");
      return 0;
    }
    file.close();
  }

  return -1;
}
