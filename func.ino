// LEDs functions
void tickBack()
{
  int stateBack = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !stateBack);
}

void tickFront(int mode)
{
  int stateFront = 0;
  switch (mode) {
    case MAIN_MODE_NORMAL:
      stateFront = digitalRead(LED_GREEN);
      digitalWrite(LED_GREEN, !stateFront);
      break;
    case MAIN_MODE_OFFLINE:
      stateFront = digitalRead(LED_YELLOW);
      digitalWrite(LED_YELLOW, !stateFront);
      break;
    case MAIN_MODE_FAIL:
      stateFront = digitalRead(LED_RED);
      digitalWrite(LED_RED, !stateFront);
      break;
    default: 
      stateFront = digitalRead(LED_GREEN);
      digitalWrite(LED_GREEN, !stateFront);
  }
}

void tickOffAll() {
  ticker.detach();
  tickerBack.detach();
}
