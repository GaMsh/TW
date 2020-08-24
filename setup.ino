void setup() 
{  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_EXTERNAL, OUTPUT);
  pinMode(RESET_WIFI, INPUT_PULLUP);

  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}
    
  Serial.println("Device '" + deviceName + "' is starting...");
  Serial.println("Voltage: " + String(ESP.getVcc()));

  checkWiFiConfiguration();
  
  WiFi.hostname(deviceName);
  
  ticker1.attach_ms(100, tickInternal);
  ticker2.attach_ms(100, tickExternal, MAIN_MODE_OFFLINE);

  if (!setupWiFiManager()) {
    Serial.println("failed to connect and hit timeout");
    delay(15000);
    ESP.restart();
  } else {
    Serial.println("WiFi network connected (" + String(WiFi.RSSI()) + ")");
    NO_INTERNET = false;

    checkFirmwareUpdate();

    if (LittleFS.begin()) {
      Serial.println(F("LittleFS was mounted"));
    } else {
      Serial.println(F("Error while mounting LittleFS"));
    }

    ///// Config
    int customInterval = readCfgFile("interval").toInt();
    if (customInterval > 1) {
      SENS_INTERVAL = customInterval * 1000;
    }
  
    String customSsl = readCfgFile("ssl");
    if (customSsl) {
      OsMoSSLFingerprint = customSsl;
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
    udp.begin(LOCAL_PORT);
    
    ///// UPnP
    Serial.println("UPnP start");
    boolean portMappingAdded = false;
    tinyUPnP.addPortMappingConfig(WiFi.localIP(), LOCAL_PORT, RULE_PROTOCOL_UDP, 30000, deviceName);
    while (!portMappingAdded) {
      portMappingAdded = tinyUPnP.commitPortMappings();
      if (portMappingAdded) {
        UPnP = true;
      }
    
//      if (!portMappingAdded) {
//        // for debugging, you can see this in your router too under forwarding or UPnP
//        tinyUPnP.printAllPortMappings();
//        Serial.println(F("This was printed because adding the required port mapping failed"));
//        delay(30000);  // 30 seconds before trying again
//      }
    }
    Serial.println("UPnP done");

    ///// Final
    TOKEN = readCfgFile("token");
    callServer("I", TOKEN, "");
   
    ticker2.attach_ms(500, tickExternal, MAIN_MODE_OFFLINE);

    getTimeFromInternet();

    getDeviceConfiguration(UPnP);

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
      int tryBMERemaining = 6;
      while(!bme.begin())
      {
        if (tryBMERemaining == 0) {
          STATUS_BME280_GOOD = false;
          break;
        }
        
        Serial.println("Could not find BME-280 sensor!");
        delay(1000);
        tryBMERemaining--;
      }
  
      tickOffAll();
      ticker1.attach_ms(4000, tickInternal);
      ticker2.attach_ms(4000, tickExternal, MAIN_MODE_NORMAL);
      myHumidity.begin();
    }
  }

  tickOffAll();

  // Завершаем инициализацию устройства, регулируем яркость светодиода по конфигу
  
  analogWrite(LED_EXTERNAL, 255);
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  analogWrite(LED_EXTERNAL, LED_BRIGHT);
}
