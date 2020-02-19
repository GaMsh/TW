void setup() 
{  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_EXTERNAL, OUTPUT);
  pinMode(RESET_WIFI, INPUT_PULLUP);

  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}
    
  Serial.println("Device '" + deviceName + "' is starting...");

  checkWiFiConfiguration();
  
  WiFi.hostname(deviceName);
  
  ticker1.attach_ms(100, tickInternal);
  ticker2.attach_ms(100, tickExternal, MAIN_MODE_OFFLINE);

  if (!setupWiFiManager()) {
    Serial.println("failed to connect and hit timeout");
    delay(15000);
    ESP.restart();
  } else {
    Serial.println("WiFi network connected");
    NO_INTERNET = false;

    if (SPIFFS.begin()) {
      Serial.println(F("SPIFFS was mounted"));
    } else {
      Serial.println(F("Error while mounting SPIFFS"));
    }
  
    int customInterval = readCfgFile("interval").toInt();
    if (customInterval > 1) {
      SENS_INTERVAL = customInterval * 1000;
    }
  
    String customSsl = readCfgFile("ssl");
    if (customSsl) {
      OsMoSSLFingerprint = customSsl;
    }
  
    TOKEN = readCfgFile("token");

    getTimeFromInternet();

    getDeviceConfiguration();

    tickOffAll();
    ticker1.attach_ms(100, tickInternal);

    if (bufferCount() > 0) {
      Serial.println();
      Serial.println("Buffer count: " + bufferCount());
      MODE_SEND_BUFFER = true;
    }

    if (!CHIP_TEST) {
      Wire.begin();

      tickOffAll();
      ticker1.attach_ms(2000, tickInternal);
      ticker2.attach_ms(2000, tickExternal, MAIN_MODE_NORMAL);
      while(!bme.begin())
      {
        Serial.println("Could not find BME-280 sensor!");
        delay(2000);
      }
  
      tickOffAll();
      ticker1.attach_ms(4000, tickInternal);
      ticker2.attach_ms(4000, tickExternal, MAIN_MODE_NORMAL);
      myHumidity.begin();
    }

    checkFirmwareUpdate();
  }

  tickOffAll();

  // Завершаем инициализацию устройства, гасим противный синий светодиод!
  
  analogWrite(LED_EXTERNAL, 255);
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  analogWrite(LED_EXTERNAL, 25);
}
