Прошивка для первого поколения устройств проекта "Самый Честный Прогноз" - GaM_TW1 [oksana]

Прошивка исправно отправляет данные с температурных датчиков GY-21 и BME-280 на сервер IoT.OsMo.mobi
Если во время работы пропадает WiFi-сеть или сервер перестал отвечать,
то данные записываются в локальный файл buffer.txt в область SPIFFS. После появления связи,
они отправляются одним пакетом на сервер и обрабатываются.

При запуске устройство проверяет наличие новой прошивки на сервере обновления и, если это необходимо, обновляется.

Известные проблемы:
- устройство не может быть запущено, если интернет недоступен (необходима синхронизация часов)
