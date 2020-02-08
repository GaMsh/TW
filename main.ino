void loop() 
{
  unsigned long currentMillis = millis();

  if (!CHIP_TEST) {
    if (currentMillis - previousMillisReboot > REBOOT_INTERVAL) {
      Serial.println("It`s time to reboot");
      if (!NO_INTERNET && !NO_SERVER) { // && BUFFER_COUNT == 0) {
        ESP.restart();
      } else {
        Serial.println("But it`s impossible");
      }
    }
  }

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    ticker.detach();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_EXTERNAL, LOW);
    
    previousMillis = currentMillis;

    float pressure;
    float tempC;
    float temp;
    float humdC;
    float humd;    

    if (CHIP_TEST) {
      humd = 90.5;
      temp = 20.5;
    } else {
      humd = myHumidity.readHumidity();
      temp = myHumidity.readTemperature();
    }
  
    ///////////

    if (CHIP_TEST) {
      pressure = 760.25;
      tempC = 25.2;
      humdC = 65.93;    
    } else {
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  
      bme.read(pressure, tempC, humdC, tempUnit, presUnit);
      pressure = pressure / 133.3224;
    }
    ///////////

    time_t now = time(nullptr);
    String urlString = 
      "token=" + String(TOKEN) + "&" +
      "t1=" + String(temp) + "&" +
      "h1=" + String(humd) + "&" +
      "p1=" + String(pressure) + "&" +
      "t2=" + String(tempC) + "&" +
      "h2=" + String(humdC) + "&" +
      "millis=" + millis() + "&" + 
      "time=" + String(time(&now));
    
    mainProcess(urlString);
  }
}
