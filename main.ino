void loop() {
    if (FULL_MODE) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            digitalWrite(LED_BUILTIN, HIGH);
            char incomingPacket[1024];
            Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(),
                          udp.remotePort());
            int len = udp.read(incomingPacket, 1024);
            if (len > 0) {
                incomingPacket[len] = 0;
            }
            parseCommand(incomingPacket);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }

    unsigned long currentMillis = millis();

    taskConfig(currentMillis);
    if (FULL_MODE) {
        taskPing(currentMillis);
    }
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
        if (!STATUS_OUTDOOR_GOOD) {
            callServer("E", "", "OUTDOOR");
        }
        if (!STATUS_INDOOR1_GOOD) {
            callServer("E", "", "INDOOR1");
        }
        if (!STATUS_INDOOR2_GOOD) {
            callServer("E", "", "INDOOR2");
        }
        STATUS_REPORT_SEND = true;
    }

    if (FULL_MODE) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            String wifiList = "";
            Serial.printf("%d network(s) found\n", n);
            for (int i = 0; i < n; i++) {
                Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(),
                              WiFi.channel(i), WiFi.RSSI(i),
                              WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
                wifiList += String(WiFi.SSID(i).c_str()) + ":" + String(WiFi.channel(i)) +
                            ":" + String(WiFi.RSSI(i)) + ":" +
                            String(WiFi.encryptionType(i)) + ";";
            }
            callServer("W", "", wifiList);
            WiFi.scanDelete();
        }
    }
}
