#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino (2.7.4)
#include <ESP8266HTTPClient.h>    // https://github.com/esp8266/Arduino
#include <ESP8266httpUpdate.h>    // https://github.com/esp8266/Arduino
#include <WiFiUdp.h>              // https://github.com/esp8266/Arduino

// для работы локального хранилища
#include <LittleFS.h>             // https://github.com/esp8266/Arduino
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson

// для удобной настройки WiFi
#include <DNSServer.h>            // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>     // https://github.com/esp8266/Arduino
#include <MyWiFiManager.h>        // https://github.com/tzapu/WiFiManager
// важно знать! используется изменённая библиотека WiFiManager 0.15,
// с русским переводом, блокировкой сброса точки в случае длительного отсутствия и парой баг фиксов

// для работы датчиков климата
#include <Wire.h>                 // https://github.com/esp8266/Arduino
#include <HTU21D.h>               // https://github.com/enjoyneering/HTU21D
#include <BME280I2C.h>            // https://github.com/finitespace/BME280
#include <SHT3x.h>                // https://github.com/Risele/SHT3x

// для работы статусных светодиодов
#include <Ticker.h>               // https://github.com/esp8266/Arduino

// // //

ADC_MODE(ADC_VCC);

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
BME280I2C OUTDOOR_SENSOR(settings);
HTU21D INDOOR_SENSOR1(HTU21D_RES_RH12_TEMP14);
SHT3x INDOOR_SENSOR2;

Ticker ticker1;
Ticker ticker2;

WiFiUDP udp;

#define SERIAL_BAUD 115200 // скорость Serial порта, менять нет надобности
#define CHIP_TEST 0 // если нужно протестировать плату без подключения датчиков, задайте 1
#define NO_AUTO_UPDATE 0 // если нужно собрать свою прошивку и не получить перезатирание через OTA, задайте 1

#define MAIN_MODE_NORMAL 100 // всё нормально, связь и работа устройства в норме
#define MAIN_MODE_OFFLINE 200 // устройство работает, но испытывает проблемы с передачей данных
#define MAIN_MODE_FAIL 300 // что-то пошло не так, устройство не может функционировать без вмешательства прямых рук

#define FIRMWARE_UPDATE_SERVER "http://tw.bigapi.ru"
#define HTTP_SERVER_DEVICE "http://iot.osmo.mobi/device"
#define HTTP_SERVER_SEND "http://iot.osmo.mobi/send"
#define HTTP_SERVER_SEND_PACK "http://iot.osmo.mobi/sendPack"
#define UDP_SERVER_HOST "bigapi.ru"
#define UDP_SERVER_PORT 25827

boolean STATUS_OUTDOOR_GOOD = true;
boolean STATUS_INDOOR1_GOOD = true;
boolean STATUS_INDOOR2_GOOD = true;
boolean STATUS_REPORT_SEND = false;

boolean FULL_MODE = true; // переключение устройства в режим "постоянной" связи через UDP

int LOCAL_PORT = 10125; // локальный порт для UDP
int PING_INTERVAL = 3000; // интервал пинга сервера по UDP по умолчанию
int LED_BRIGHT = 125; // яркость внешнего светодиода в режиме ожидания
int SENS_INTERVAL = 60000; // интервал опроса датчиков
int REBOOT_INTERVAL = 4 * 60 * 60000; // интервал принудительной перезагрузки устройства, мы не перезагружаемся, если нет сети, чтобы не потерять время и возможность накапливать буфер
int CONFIG_INTERVAL = 60 * 60000; // интервал обновления конфигурации устройства с сервера
int REPORT_INTERVAL = 60 * 60000; // интервал повтора отправки отчёта о проблемах (если проблема актуальна)

boolean NO_INTERNET = true; // флаг состояния, поднимается если отвалилась wifi сеть
boolean NO_SERVER = true; // флаг состояния, поднимается если отвалился сервер
boolean MODE_SEND_BUFFER = false; // флаг означающий, что необходимо сделать опустошение буфера

int MODE_RESET_WIFI = 0; // флаг означающий, что пользователем инициирован процесс очистки настроек WiFi

const char *DEVICE_MODEL = "HCS";
const char *DEVICE_REVISION = "ohta";
const char *DEVICE_FIRMWARE = "3.2.0";

const int RESET_WIFI = 0; // D3
const int LED_EXTERNAL = 14; // D5

unsigned long previousMillis = SENS_INTERVAL * -2; // Чтобы начинать отправлять данные сразу после запуска
unsigned long previousMillisConfig = 0;
unsigned long previousMillisPing = 0;
unsigned long previousMillisReboot = 0;
unsigned long previousMillisReport = 0;

String deviceName = String(DEVICE_MODEL) + "_" + String(DEVICE_FIRMWARE);

String TOKEN = "";

int bytesWriten = 0;
int pingCount = 0;

void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.println(WiFi.macAddress());
    ticker1.attach_ms(500, tickInternal);
    ticker2.attach_ms(1000, tickExternal, MAIN_MODE_NORMAL);
}
