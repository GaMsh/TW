void taskConfig(int currentMillis) {
    if (currentMillis - previousMillisConfig > CONFIG_INTERVAL) {
        getDeviceConfiguration(false);
        previousMillisConfig = currentMillis;
    }
}

void taskPing(int currentMillis) {
    if (currentMillis - previousMillisPing >= PING_INTERVAL) {
        pingServer();
        previousMillisPing = currentMillis;
    }
}

void taskRestart(int currentMillis) {
    if (!CHIP_TEST) {
        if (currentMillis - previousMillisReboot > REBOOT_INTERVAL) {
            Serial.println("It`s time to reboot");
            if (!NO_INTERNET && !NO_SERVER) {
                delay(10000);
                ESP.restart();
            } else {
                previousMillisReboot = currentMillis;
                Serial.println("But it`s impossible, no internet connection");
            }
        }
    }
}

void mainProcess(int currentMillis) {
    previousMillisPing = currentMillis;

    if (MODE_SEND_BUFFER) {
        if (bufferReadAndSend("data")) {
            MODE_SEND_BUFFER = false;
        }
    }

    // OUTDOOR
    float t1;
    float h1;
    float p1;

    // INDOOR1
    float t2;
    float h2;


    // INDOOR2
    float t3;
    float h3;

    ////////////////

    // OUTDOOR
    if (CHIP_TEST) {
        p1 = 760.25;
        t1 = 25.29;
        h1 = 65.93;
    } else {
        BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
        BME280::PresUnit presUnit(BME280::PresUnit_Pa);

        OUTDOOR_SENSOR.read(p1, t1, h1, tempUnit, presUnit);
        p1 = p1 / 133.3224;
    }

    // INDOOR1
    if (CHIP_TEST) {
        t2 = 20.51;
        h2 = 60.25;
    } else {
        t2 = INDOOR_SENSOR1.readTemperature();
        h2 = INDOOR_SENSOR1.readCompensatedHumidity();
    }

    // INDOOR2
    if (CHIP_TEST) {
        t3 = 20.51;
        h3 = 60.25;
    } else {
        INDOOR_SENSOR2.UpdateData();
        t3 = INDOOR_SENSOR2.GetTemperature();
        h3 = INDOOR_SENSOR2.GetRelHumidity();
    }

    time_t now = time(nullptr);

    String urlString = "token=" + String(TOKEN) + "&";
    if (!isnan(p1)) {
        urlString += "t1=" + String(t1) + "&h1=" + String(h1) + "&p1=" + String(p1) + "&";
        STATUS_OUTDOOR_GOOD = true;
    } else {
        STATUS_OUTDOOR_GOOD = false;
    }
    if (t2 != 255 && t2 != 998 && t2 != 999) {
        urlString += "t2=" + String(t2) + "&" + "h2=" + String(h2) + "&";
        STATUS_INDOOR1_GOOD = true;
    } else {
        STATUS_INDOOR1_GOOD = false;
    }
    if (t3 != 255 && t3 != 998 && t3 != 999 && h3 != 0) {
        urlString += "t3=" + String(t3) + "&" + "h3=" + String(h3) + "&";
        STATUS_INDOOR2_GOOD = true;
    } else {
        STATUS_INDOOR2_GOOD = false;
    }

    urlString += "millis=" + String(millis()) + "&" + "time=" + String(time(&now));
    actionDo(urlString);

    if (FULL_MODE) {
        String string = "";
        if (!isnan(p1)) {
            string += "t1" + String(t1) + "h1" + String(h1) + "p1" + String(p1);
            STATUS_OUTDOOR_GOOD = true;
        } else {
            STATUS_OUTDOOR_GOOD = false;
        }
        if (t2 != 255 && t2 != 998 && t2 != 999) {
            string += "t2" + String(t2) + "h2" + String(h2);
            STATUS_INDOOR1_GOOD = true;
        } else {
            STATUS_INDOOR1_GOOD = false;
        }
        string += "M" + String(millis()) + "T" + String(time(&now));

        if (NO_SERVER) {
            bufferWrite("udp", string);
        } else {
            callServer("D", "", string);
        }
    }
}

void checkFirmwareUpdate(bool ignoreConfig) {
    if (ignoreConfig || (!NO_AUTO_UPDATE && !NO_INTERNET && !CHIP_TEST)) {

        WiFiClient wifi;
        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
        t_httpUpdate_return ret = ESPhttpUpdate.update(wifi, FIRMWARE_UPDATE_SERVER, DEVICE_FIRMWARE);

        switch (ret) {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                              ESPhttpUpdate.getLastError(),
                              ESPhttpUpdate.getLastErrorString().c_str());
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
        if (FULL_MODE) {
            bufferReadAndSend("udp");
        }
        return bufferReadAndSend("data");
    }

    Serial.println(urlString);

    WiFiClient wifi;
    HTTPClient http;
    http.begin(wifi, HTTP_SERVER_SEND);
    http.setUserAgent(deviceName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(urlString);
    Serial.println("Sending to server...");
    if (httpCode != HTTP_CODE_OK) {
        NO_SERVER = true;
        bufferWrite("data", urlString);
        return false;
    }
    if (NO_SERVER) {
        NO_SERVER = false;
        bufferReadAndSend("data");
    }
    String payload = http.getString();
    Serial.print(String(httpCode) + ": ");
    Serial.println(payload);
    http.end();

    return true;
}

void pingServer() {
    pingCount++;
    callServer("P", "", "");
    Serial.println("Ping");
}

void callServer(String command, String string, String data) {
    udp.beginPacket(UDP_SERVER_HOST, UDP_SERVER_PORT);
    String query = TOKEN + "__" + command;
    if (string != "") {
        query += ":" + string;
    }
    if (data != "") {
        query += "|" + data;
    }
    udp.print(query);
    udp.endPacket();
}

boolean writeLocalBuffer(String urlString) {
    if (!NO_INTERNET) {
        NO_INTERNET = true;
        Serial.println("NO INTERNET MODE ACTIVATED");
    }

    return bufferWrite("data", urlString);
}

boolean parseCommand(String incomingPacket) {
    String other = "";
    Serial.println(incomingPacket);

    String token = "";
    String command = "";
    String string = "";
    String data = "";

    token = getValue(incomingPacket, '_', 0);
    if (TOKEN != token) {
        return false;
    }

    other = getValue(incomingPacket, '_', 2);

    command = getValue(other, ':', 0);
    other = getValue(other, ':', 1);

    string = getValue(other, '|', 0);
    data = getValue(other, '|', 1);

//  if (command == "RC") {
//    Serial.println("Remote control");
//    if (string == "IL") {
//      Serial.println("Ticker set to " + data);  
//      ticker2.attach_ms(data.toInt(), tickExternal, MAIN_MODE_NORMAL);
//      callServer("RC", "IL", "1");
//    }
//  }

    return true;
}
