void loop() 
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    char incomingPacket[255];
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
    int len = udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
  }
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    previousMillis = currentMillis;
    
    taskRestart(currentMillis, previousMillisReboot);

    previousMillisConfig = taskConfig(currentMillis, previousMillisConfig);

    mainProcess();
  }

  if (currentMillis - previousMillisPing >= 4000) {
    previousMillisPing = currentMillis;
    
    pingServer();
  }
}
