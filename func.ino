// core functions
bool getDeviceConfiguration(bool first) {
    StaticJsonDocument<1024> jb;
    String postData =
            "token=" + TOKEN + "&" +
            "revision=" + String(DEVICE_REVISION) + "&" +
            "model=" + String(DEVICE_MODEL) + "&" +
            "firmware=" + String(DEVICE_FIRMWARE) + "&"
                                                    "ip=" + WiFi.localIP().toString() + "&" +
            "port=" + String(LOCAL_PORT) + "&" +
            "mac=" + String(WiFi.macAddress()) + "&" +
            "ssid=" + String(WiFi.SSID()) + "&" +
            "rssi=" + String(WiFi.RSSI()) + "&" +
            "vcc=" + String(ESP.getVcc()) + "&" +
            "nu=" + String(NO_AUTO_UPDATE) + "&" +
            "ct=" + String(CHIP_TEST) + "&" +
            "bufferCount=" + String(bufferCount("data"));
    Serial.println(postData);

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(10) + 60;
    DynamicJsonDocument doc(capacity);

    WiFiClient wifi;
    HTTPClient http;
    http.begin(wifi, OSMO_HTTP_SERVER_DEVICE);
    http.setUserAgent(deviceName);
    http.setTimeout(30000);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    if (!first && httpCode < 0) {
        NO_SERVER = true;
        return false;
    } else {
        if (httpCode != HTTP_CODE_OK && !CHIP_TEST) {
            ticker1.attach_ms(200, tickInternal);
            ticker2.attach_ms(500, tickExternal, MAIN_MODE_OFFLINE);
            Serial.println("Failed to initialize the device using the server");
            Serial.println(httpCode);
            delay(15000);
            ESP.restart();
            return false;
        }
    }
    Serial.println("Get device configuration from server, HTTP Code: " + String(httpCode));
    if (httpCode == HTTP_CODE_OK) {
        NO_SERVER = false;
    } else {
        return false;
    }

    String payload = http.getString();
    deserializeJson(doc, payload);
    http.end();

    int serverTime = doc["time"].as<int>();
    Serial.println(serverTime);

    struct timeval tv;
    tv.tv_sec = serverTime;
    settimeofday(&tv, NULL); // Применяем время сервера, как локальное

    if (doc["interval"].as<int>() > 0) {
        int SENS_INTERVAL_NEW = doc["interval"].as<int>() * 1000;
        if (SENS_INTERVAL != SENS_INTERVAL_NEW) {
            SENS_INTERVAL = SENS_INTERVAL_NEW;
            writeCfgFile("interval", doc["interval"].as<String>());
            Serial.println("Sending interval was updated in store");
        }
    }

    if (TOKEN != doc["token"].as<String>()) {
        TOKEN = doc["token"].as<String>();
        writeCfgFile("token", TOKEN);
        Serial.println("Device token was updated in store");
    }

    if (doc["led_bright"].as<int>() > 0) {
        int LED_BRIGHT_NEW = doc["led_bright"].as<int>();
        if (LED_BRIGHT != LED_BRIGHT_NEW) {
            LED_BRIGHT = LED_BRIGHT_NEW;
            writeCfgFile("led_bright", doc["led_bright"].as<String>());
            Serial.println("Led brightness was updated in store");
        }
    }

    if (doc["local_port"].as<int>() > 0) {
        int LOCAL_PORT_NEW = doc["local_port"].as<int>();
        if (LOCAL_PORT != LOCAL_PORT_NEW) {
            LOCAL_PORT = LOCAL_PORT_NEW;
            writeCfgFile("local_port", doc["local_port"].as<String>());
            Serial.println("Local port was updated in store");
        }
    }

    if (FULL_MODE) {
        WiFi.scanNetworks(true);
    }

    return true;
}

// bufferFile functions
int bufferCount(String filename) {
    int countLine = 0;
    File bufferFile = LittleFS.open("/" + filename + ".buff", "r");
    char buffer[256];
    while (bufferFile.available()) {
        int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
        buffer[l] = 0;
        countLine++;
    }
    bufferFile.close();
    return countLine;
}

bool bufferWrite(String filename, String urlString) {
    File bufferFile = LittleFS.open("/" + filename + ".buff", "a+");
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

int bufferReadAndSend(String filename) {
    File bufferFile = LittleFS.open("/" + filename + ".buff", "r+");
    if (bufferFile) {
        int until = bufferCount(filename);

        WiFiClient wifi;
        HTTPClient http;
        http.begin(wifi, OSMO_HTTP_SERVER_SEND_PACK);
        http.setUserAgent(deviceName);
        http.addHeader("Content-Type", "text/plain");
        http.setTimeout(15000);

        bufferFile.seek(0, SeekSet);
        Serial.print("Buffer size: ");
        Serial.print(bufferFile.size());
        Serial.println();

        int rowsCountAll = 0;
        int rowsCount = 0;
        String toSend = "";
        char buffer[128];
        while (bufferFile.available()) {
            int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
            buffer[l] = 0;

            toSend += buffer + String("\n ");
            rowsCount++;
            rowsCountAll++;

            if (rowsCount >= 10 || rowsCountAll >= until) {
                Serial.println("SEND part of buffer");
                int httpCode = http.POST(toSend);
                Serial.println(httpCode);
                http.end();
                if (httpCode == HTTP_CODE_OK) {
                    toSend = "";
                    rowsCount = 0;

                    if (rowsCountAll >= until) {
                        Serial.println("Delete buffer file");
                        bufferFile.close();
                        LittleFS.remove("/data.buff");
                        return true;
                    }
                }
            }
        }
    }

    return -1;
}

// LEDs functions
void tickInternal() {
    int stateIntervalLed = digitalRead(LED_BUILTIN);
    digitalWrite(LED_BUILTIN, !stateIntervalLed);
}

void tickExternal(int mode) {
    int stateExternalLed = 0;
    switch (mode) {
        case MAIN_MODE_NORMAL:
        case MAIN_MODE_OFFLINE:
        case MAIN_MODE_FAIL:
            stateExternalLed = digitalRead(LED_EXTERNAL);
            digitalWrite(LED_EXTERNAL, !stateExternalLed);
            break;
        default:
            stateExternalLed = digitalRead(LED_EXTERNAL);
            digitalWrite(LED_EXTERNAL, !stateExternalLed);
    }
}

void tickOffAll() {
    ticker1.detach();
    ticker2.detach();
}

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
