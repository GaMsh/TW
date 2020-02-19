void loop() 
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    previousMillis = currentMillis;
    
    taskRestart(currentMillis, previousMillisReboot);

    previousMillisConfig = taskConfig(currentMillis, previousMillisConfig);

//    tickOffAll();
//    digitalWrite(LED_BUILTIN, HIGH);
//    digitalWrite(LED_EXTERNAL, HIGH);    

    mainProcess();
  }
}
