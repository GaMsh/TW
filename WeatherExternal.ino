#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

//needed for local file system SFIFFS working
#include <FS.h>
#include <ArduinoJson.h>

//needed for library WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//needed for sensors
#include <Wire.h>
#include <SparkFunHTU21D.h>
#include <BME280I2C.h>
#include <EnvironmentCalculations.h>

//needed for LED status
#include <Ticker.h>

// // // это был длииинный список библиотек для запуска этой штуки :)))

ADC_MODE(ADC_VCC); // чтобы измерять self-voltage level 3.3V

Ticker ticker;
Ticker tickerBack;
HTU21D myHumidity;
BME280I2C bme;

enum TempUnit
{
   TempUnit_Celsius,
   TempUnit_Fahrenheit
};

// инициализируем файлы
File bufferFile;

#define SERIAL_BAUD 115200 // скорость Serial порта, менять нет надобности
#define CHIP_TEST 0 // если нужно протестировать плату без подключения датчиков
#define NO_AUTO_UPDATE 0 // если нужно собрать свою прошивку и не получить перезатирание через OTA

#define MAIN_MODE_NORMAL 100 // всё нормально, связь и работа в норме
#define MAIN_MODE_OFFLINE 200 // система работает, но испытывает проблемы с передачей данных
#define MAIN_MODE_FAIL 300 // что-то пошло не так, система не может функционировать без вмешательства прямых рук

int SENS_INTERVAL = 60000; // интервал опроса датчиков по умолчанию
int REBOOT_INTERVAL = 2 * 60 * 60000; // интервал принудительной перезагрузки устройства, мы не перезагружаемся, если нет сети

boolean NO_INTERNET = true; // флаг состояния, поднимается если отвалилась wifi сеть
boolean NO_SERVER = true; // флаг состояния, поднимается если отвалился сервер
int BUFFER_COUNT = 0; // счётчик строк в буферном файле не отправленных на сервер
int MODE_RESET_WIFI = 0;
int MODE_SEND_BUFFER = 0;

const char* DEVICE_MODEL = "GaM_TW1";
const char* DEVICE_REVISION = "oksana"; 
const char* DEVICE_FIRMWARE = "1.5.0";

const int RESET_WIFI = 0; // PIN D3

const int LED_EXTERNAL  = 14; // PIN D5
//const int LED_YELLOW  = 12; // PIN D6
//const int LED_RED     = 13; // PIN D7

unsigned long previousMillis = SENS_INTERVAL * -2; //Чтобы начинать отправлять данные сразу после запуска
unsigned long previousMillisReboot = 0;

String deviceName = String(DEVICE_MODEL) + "_" + String(DEVICE_FIRMWARE);

String OsMoSSLFingerprint = ""; //69 3B 2D 26 B2 A7 96 5E 10 E4 2F 84 63 56 CE ED E2 EC DA A3
String TOKEN = "";

int bytesWriten = 0;



// WifiManager callback
void configModeCallback(WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach_ms(500, tickBack);
  ticker.attach_ms(1000, tickFront, MAIN_MODE_NORMAL);
  ticker.attach_ms(500, tickFront, MAIN_MODE_OFFLINE);
}
