void loop() 
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    char incomingPacket[255];
//    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
    int len = udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("Incoming UDP packet (%d): %s\n", packetSize, incomingPacket);
  }
  
  unsigned long currentMillis = millis();

  taskConfig(currentMillis);
  taskPing(currentMillis);
  taskRestart(currentMillis);

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    previousMillis = currentMillis;
    mainProcess(currentMillis);
  }

  if (currentMillis - previousMillisReport >= REPORT_INTERVAL) {
    previousMillisReport = currentMillis;
    STATUS_REPORT_SEND = false;
  }

  if (!STATUS_REPORT_SEND) {
    if (!STATUS_BME280_GOOD) {
      callServer("E", "", "BME280");
    }
    if (!STATUS_GY21_GOOD) {
      callServer("E", "", "GY21");
    }
    
    STATUS_REPORT_SEND = true;
  }

  int n = WiFi.scanComplete();
  if(n >= 0)
  {
    String wifiList = "";
    Serial.printf("%d network(s) found\n", n);
    for (int i = 0; i < n; i++)
    {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
      wifiList += String(WiFi.SSID(i).c_str()) + ":" + String(WiFi.channel(i)) + ":" + String(WiFi.RSSI(i)) + ":" + String(WiFi.encryptionType(i)) + ";";
    }
    callServer("W", "", wifiList);
    WiFi.scanDelete();
  }

  tinyUPnP.updatePortMappings(600000);
}
