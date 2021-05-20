void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(RESET_WIFI, INPUT_PULLUP);

    Serial.begin(SERIAL_BAUD);
    while (!Serial) {}

    Serial.println("Device '" + deviceName + "' is starting...");
    Serial.println("Voltage: " + String(ESP.getVcc()));

    checkWiFiConfiguration();

    if (CHIP_TEST) {
        Serial.println("CHIP TEST mode is activated. No real data from sensors in this mode");
    }
    if (NO_AUTO_UPDATE) {
        Serial.println(
                "NO AUTO UPDATE firmware mode is activated! You can manually update, by RESET_WIFI pin to LOW on boot");
        manualCheckFirmwareUpdate();
    }

    WiFi.hostname(deviceName);

    ticker1.attach_ms(100, tickInternal);
    ticker2.attach_ms(100, tickExternal, MAIN_MODE_OFFLINE);

    if (!setupWiFiManager()) {
        delay(15000);
        ESP.restart();
    } else {
        Serial.println("WiFi network connected (" + String(WiFi.RSSI()) + ")");
        NO_INTERNET = false;

        checkFirmwareUpdate(false);

        if (LittleFS.begin()) {
            Serial.println(F("LittleFS was mounted"));
        } else {
            Serial.println(F("Error while mounting LittleFS"));
        }

        ///// Config
        int customInterval = readCfgFile("interval").toInt();
        if (customInterval > 0) {
            SENS_INTERVAL = customInterval * 1000;
        }

        int customLedBright = readCfgFile("led_bright").toInt();
        if (customLedBright > 0) {
            LED_BRIGHT = customLedBright;
        }

        int customLocalPort = readCfgFile("local_port").toInt();
        if (customLocalPort > 0) {
            LOCAL_PORT = customLocalPort;
        }
        ///// UDP
        if (FULL_MODE) {
            udp.begin(LOCAL_PORT);
        }

        ///// Final
        TOKEN = readCfgFile("token");
        callServer("I", "I", "I");

        ticker2.attach_ms(500, tickExternal, MAIN_MODE_OFFLINE);

        getDeviceConfiguration(true);

        tickOffAll();
        ticker1.attach_ms(100, tickInternal);

        if (bufferCount("data") > 0) {
            Serial.println();
            Serial.println("Buffer count: " + bufferCount("data"));
            MODE_SEND_BUFFER = true;
        }

        if (!CHIP_TEST) {
            Wire.begin();

            tickOffAll();
            ticker1.attach_ms(2000, tickInternal);
            ticker2.attach_ms(2000, tickExternal, MAIN_MODE_NORMAL);
            int tryBMERemaining = 11;
            while (!OUTDOOR_SENSOR.begin()) {
                if (tryBMERemaining == 0) {
                    STATUS_OUTDOOR_GOOD = false;
                    break;
                }

                Serial.println("Could not find BME-280 sensor!");
                delay(910);
                tryBMERemaining--;
            }

            tickOffAll();
            ticker1.attach_ms(4000, tickInternal);
            ticker2.attach_ms(4000, tickExternal, MAIN_MODE_NORMAL);

            INDOOR_SENSOR1.begin();

//            if (INDOOR_SENSOR1.readCompensatedHumidity() >= 80) {
//                callServer("S", "HEAT", "GY21");
//                Serial.println("Heating started...");
//                myHumidity.setHeater(HTU21D_ON);
//                delay(4200);
//                myHumidity.setHeater(HTU21D_OFF);
//            }

            tickOffAll();
            ticker1.attach_ms(6000, tickInternal);
            ticker2.attach_ms(6000, tickExternal, MAIN_MODE_NORMAL);
            INDOOR_SENSOR2.Begin();
        }
    }

    tickOffAll();

    // Завершаем инициализацию устройства, регулируем яркость светодиода по конфигу

    analogWrite(LED_EXTERNAL, 255);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    analogWrite(LED_EXTERNAL, LED_BRIGHT);
}
