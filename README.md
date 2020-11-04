# Самый Честный Прогноз

Прошивка для первого и второго (на печатной плате) поколения устройств проекта "Самый Честный Прогноз" - GaM_TW1 [kitkat]
Прошивка работает с сервером bigapi.ru

Поддерживаемые датчики: 
  - BME-280
  - GY-21 или HTU-21

# Для начала работы:
  - Скрафтите себе плату
  - Воткните туда WEMoS D1 mini
  - Подключите датчики 
  - Прошейте устройство
  - Подключитесь к точке доступа GaM_TW_***_SETUP и укажите данные вашей WiFi-сети


При запуске устройство проверяет наличие новой прошивки на сервере обновления и, если это необходимо, обновляется. Делается системный перезапуск раз в 8 часов (при отсутствии проблем с сетью).

Если, во время работы пропадает WiFi-сеть или сервер перестал отвечать, то данные записываются в локальный файл data.buff через LittleFS. После появления связи, они отправляются несколькими блоками на сервер и обрабатываются. При перезагрузке устройства файл проверяется на наличие содержимого и также отправляется. После успешной отправки он удаляется.

> Вопросы? Пишите gam@gamsh.ru с пометкой TW в теме.
