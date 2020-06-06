void loop() 
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    previousMillis = currentMillis;
    
    taskRestart(currentMillis, previousMillisReboot);

    previousMillisConfig = taskConfig(currentMillis, previousMillisConfig);

    mainProcess();
  }
}
