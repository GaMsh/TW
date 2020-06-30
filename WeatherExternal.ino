#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// needed for local file system SFIFFS working
#include <FS.h>
#include <ArduinoJson.h>

// needed for library WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <MyWiFiManager.h>          // modified https://github.com/tzapu/WiFiManager
// важно знать! используется изменённая библиотека WiFiManager 0.15, 
// с русским переводом, блокировкой сброса точки в случае длительного отсуствия и парой баг фиксов

// needed for sensors
#include <Wire.h>
#include <HTU21D.h>
#include <BME280I2C.h>
#include <EnvironmentCalculations.h>

// needed for statuses LED
#include <Ticker.h>

// // // это был длииинный список библиотек для запуска этой штуки :)))

ADC_MODE(ADC_VCC); // чтобы измерять self-voltage level 3.3V

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);
HTU21D myHumidity;
BME280I2C bme(settings);

Ticker ticker1;
Ticker ticker2;

#define SERIAL_BAUD 115200 // скорость Serial порта, менять нет надобности
#define CHIP_TEST 0 // если нужно протестировать плату без подключения датчиков, задайте 1
#define NO_AUTO_UPDATE 0 // если нужно собрать свою прошивку и не получить перезатирание через OTA, задайте 1

#define MAIN_MODE_NORMAL 100 // всё нормально, связь и работа в норме
#define MAIN_MODE_OFFLINE 200 // система работает, но испытывает проблемы с передачей данных
#define MAIN_MODE_FAIL 300 // что-то пошло не так, система не может функционировать без вмешательства прямых рук

int LED_BRIGHT = 75; // яркость внешнего светодиода в режиме ожидания
int SENS_INTERVAL = 60000; // интервал опроса датчиков по умолчанию
int RECONFIG_INTERVAL = 30 * 60000; // интервал обновления конфигурации устройства с сервера
int REBOOT_INTERVAL = 24 * 60 * 60000; // интервал принудительной перезагрузки устройства, мы не перезагружаемся, если нет сети, чтобы не потерять буфер и время

boolean NO_INTERNET = true; // флаг состояния, поднимается если отвалилась wifi сеть
boolean NO_SERVER = true; // флаг состояния, поднимается если отвалился сервер
boolean MODE_SEND_BUFFER = false; // флаг означающий, что необходимо сделать опустошение буфера
int MODE_RESET_WIFI = 0; // флаг означающий, что пользователем инициирован процесс очистки настроек WiFi

int BUFFER_COUNT = 0; // счётчик строк в буферном файле не отправленных на сервер

const char* DEVICE_MODEL = "GaM_TW";
const char* DEVICE_REVISION = "foxxy"; 
const char* DEVICE_FIRMWARE = "1.7.2";

const int RESET_WIFI = 0; // PIN D3

const int LED_EXTERNAL = 14; // PIN D5

unsigned long previousMillis = SENS_INTERVAL * -2; // Чтобы начинать отправлять данные сразу после запуска
unsigned long previousMillisReboot = 0;
unsigned long previousMillisConfig = 0;

String deviceName = String(DEVICE_MODEL) + "_" + String(DEVICE_FIRMWARE);

String OsMoSSLFingerprint = "";
String TOKEN = "";

int bytesWriten = 0;

// WifiManager callback
void configModeCallback(WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println(WiFi.macAddress());
  ticker1.attach_ms(500, tickInternal);
  ticker2.attach_ms(1000, tickExternal, MAIN_MODE_NORMAL);
}
