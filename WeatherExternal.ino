#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>

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

//for LED status
#include <Ticker.h>
Ticker ticker;

#define SERIAL_BAUD 115200 // скорость Serial порта, менять не надо
#define CHIP_TEST 0 // если нужно протестировать плату и ключи без подключения датчиков

int SENS_INTERVAL = 60000; // интервал опроса датчиков по умолчанию
int REBOOT_INTERVAL = 86400000; // интервал принудительной перезагрузки устройства

boolean NO_INTERNET = true;
int BUFFER_COUNT = 0;

const char *DEVICE_MODEL = "GaM_TW1";
const char *DEVICE_REVISION = "khas";
const char *DEVICE_ID = "MZU5";

unsigned long previousMillis = SENS_INTERVAL * -2;

HTU21D myHumidity;
BME280I2C bme;
File bufferFile;

void tickBack()
{
  int stateBack = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !stateBack);
}

void tickFront()
{
  int stateFront = digitalRead(14);
  digitalWrite(14, !stateFront);
}

void configModeCallback(WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach_ms(500, tickBack);
  ticker.attach_ms(500, tickFront);
}

void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(14, OUTPUT);

  String deviceName = String(DEVICE_MODEL) + "_" + String(DEVICE_ID);

  WiFi.hostname(deviceName);
  
  ticker.attach_ms(100, tickBack);
  ticker.attach_ms(100, tickFront);
  
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}

  Serial.println("Device '" + deviceName + "' is starting...");

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setMinimumSignalQuality(42);
  wifiManager.setRemoveDuplicateAPs(true);
  wifiManager.setDebugOutput(false);
  wifiManager.setTimeout(300);

  if (!wifiManager.autoConnect(deviceName.c_str())) {
    Serial.println("failed to connect and hit timeout");
//    ESP.reset();
    ESP.restart();
    delay(1000);
  } else {
    Serial.println("connected...yeey :)");
    NO_INTERNET = false;

    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  
    Serial.println("Syncing time...");
    configTime(0, 0, "pool.ntp.org");  
    setenv("TZ", "GMT+0", 0);
    while(time(nullptr) <= 100000) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();

    StaticJsonDocument<1024> jb;
    String postData = 
      "token=" + String(DEVICE_REVISION) + String(DEVICE_ID) + "&" +
      "model=" + String(DEVICE_MODEL) + "&" +
      "ip=" + WiFi.localIP().toString() + "&" +
      "mac=" + String(WiFi.macAddress()) + "&" +
      "ssid=" + String(WiFi.SSID()) + "&" +
      "rssi=" + String(WiFi.RSSI());

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(10) + 60;
    DynamicJsonDocument doc(capacity);

    HTTPClient http;
    http.begin("http://iot.osmo.mobi/device");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    if (httpCode != 200 && !CHIP_TEST) {
        ticker.attach_ms(200, tickBack);
        ticker.attach_ms(500, tickFront);

        while(1) {
            Serial.println("-");
            delay(1000);
        }
    }
    Serial.println("get device config and set env, result: " + String(httpCode));
    String payload = http.getString();
    deserializeJson(doc, payload);
    http.end();

    Serial.print(F("Interval: "));
    Serial.println(doc["interval"].as<int>());
    if (doc["interval"].as<int>() > 10) {
      SENS_INTERVAL = doc["interval"].as<int>() * 1000;
    }
    
    ticker.detach();
    ticker.attach_ms(100, tickBack);    

    if (!CHIP_TEST) {
      Wire.begin();
          
      ticker.detach();
      ticker.attach_ms(2000, tickBack);
      ticker.attach_ms(2000, tickFront);
      while(!bme.begin())
      {
        Serial.println("Could not find BME-280 sensor!");
        Serial.println(millis());
        delay(2000);
      }
  
      ticker.detach();
      ticker.attach_ms(3000, tickBack);
      ticker.attach_ms(3000, tickFront);
      myHumidity.begin();
    }
  }

  if (!SPIFFS.begin()) {
    Serial.println("Error while mounting SPIFFS");
    return;
  }
  ticker.detach();

  digitalWrite(BUILTIN_LED, HIGH);
}

void loop() 
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENS_INTERVAL) {
    ticker.detach();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(14, LOW);

    time_t now = time(nullptr);
    Serial.println(ctime(&now));
    
    previousMillis = currentMillis;

    float pressure;
    float tempC;
    float temp;
    float humdC;
    float humd;    

    if (CHIP_TEST) {
      humd = 95.1;
      temp = 22.2;
    } else {
      humd = myHumidity.readHumidity();
      temp = myHumidity.readTemperature();
    }
  
    ///////////

    if (CHIP_TEST) {
      pressure = 762.23;
      tempC = 25.2;
      humdC = 65.932;    
    } else {
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  
      bme.read(pressure, tempC, humdC, tempUnit, presUnit);
      pressure = pressure / 133.3224;
    }

    String postData = 
      "token=" + String(DEVICE_REVISION) + String(DEVICE_ID) + "&" +
      "t1=" + String(temp) + "&" +
      "h1=" + String(humd) + "&" +
      "p1=" + String(pressure) + "&" +
      "t2=" + String(tempC) + "&" +
      "h2=" + String(humdC) + "&" +
      "millis=" + millis();
    
    ///////////
    if (WiFi.status() == WL_CONNECTED) {
      if (NO_INTERNET) {
        NO_INTERNET = false;
  
        Serial.println("NO INTERNET MODE DEACTIVATED");
        bufferFile.seek(0, SeekSet);
        Serial.print("Buffer size: ");
        Serial.print(bufferFile.size());
        Serial.println();

        String toSend = "";
        char buffer[256];
        while (bufferFile.available()) {
          int l = bufferFile.readBytesUntil('\n', buffer, sizeof(buffer));
          buffer[l] = 0;
          toSend += buffer + String("\n");
        }

        Serial.println(toSend);
        Serial.println();
        
        bufferFile.close();

        HTTPClient http; 
        http.begin("http://iot.osmo.mobi/sendPack");
        http.addHeader("Content-Type", "text/plain");
      
        int httpCode = http.POST(toSend);
        String payload = http.getString();
        Serial.println(payload);
        http.end();

        return;
      }
      
      Serial.println("Sending to server...");
      Serial.println(postData);
      
      HTTPClient http; 
      http.begin("http://iot.osmo.mobi/send");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
      int httpCode = http.POST(postData);
      String payload = http.getString();
      Serial.println(payload);
      http.end();

      digitalWrite(LED_BUILTIN, LOW);
      ticker.attach_ms(2000, tickFront);
    } else {
      if (!NO_INTERNET) {
        NO_INTERNET = true;
  
        Serial.println("NO INTERNET MODE ACTIVATED");
        bufferFile = SPIFFS.open("/buffer.txt", "w+");
      }
      if (bufferFile) {
        Serial.println("Write to local buffer file...");
        Serial.println(postData);
        
        bufferFile.println(postData + "&time=" + String(time(&now)));
      } else {
        Serial.println("Buffer file open failed");
      }
    }
  }
}
