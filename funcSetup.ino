bool setupWiFiManager() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setMinimumSignalQuality(33);
  wifiManager.setRemoveDuplicateAPs(true);
  wifiManager.setDebugOutput(false);
  wifiManager.setTimeout(300);
  wifiManager.setCustomHeadElement("<style>html{background:#fb7906};</style>");

  return wifiManager.autoConnect(deviceName.c_str());
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
