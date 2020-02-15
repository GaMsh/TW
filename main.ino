void loop() 
{
  unsigned long currentMillis = millis();

  if (!CHIP_TEST) {
    if (currentMillis - previousMillisReboot > REBOOT_INTERVAL) {
      Serial.println("It`s time to reboot");
      if (!NO_INTERNET && !NO_SERVER) { // && BUFFER_COUNT == 0) {
        ESP.restart();
      } else {
        Serial.println("But it`s impossible, no internet connection");
      }
    }
  }

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    ticker.detach();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_EXTERNAL, LOW);
    
    previousMillis = currentMillis;

    if (MODE_SEND_BUFFER) {
      if (bufferReadAndSend()) {
        MODE_SEND_BUFFER = 0;
      }
    }

    float t1;
    float h1;
    float p1;
    float t2;
    float h2;

    if (CHIP_TEST) {
      t2 = 20.5;
      h2 = 60.25;
    } else {
      t2 = myHumidity.readTemperature();
      h2 = myHumidity.readHumidity();
    }
  
    ///////////

    if (CHIP_TEST) {
      p1 = 760.25;
      t1 = 25.2;
      h1 = 65.93;    
    } else {
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  
      bme.read(p1, t1, h1, tempUnit, presUnit);
      p1 = p1 / 133.3224;

//      EnvironmentCalculations::TempUnit envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;
//      hi1 = EnvironmentCalculations::HeatIndex(t1, h1, envTempUnit);
//      dp1 = EnvironmentCalculations::DewPoint(t1, h1, envTempUnit);
//      ah1 = EnvironmentCalculations::AbsoluteHumidity(t1, h1, envTempUnit);
    }
    ///////////

    time_t now = time(nullptr);
    String urlString = 
      "token=" + String(TOKEN) + "&" +
      "t1=" + String(t1) + "&" +
      "h1=" + String(h1) + "&" +
      "p1=" + String(p1) + "&" +
      "t2=" + String(t2) + "&" +
      "h2=" + String(h2) + "&" +
      "millis=" + millis() + "&" + 
      "time=" + String(time(&now));
    
    mainProcess(urlString);
  }
}
