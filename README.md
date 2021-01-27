# Самый Честный Прогноз

Прошивка для первого и второго (на печатной плате) поколения устройств проекта "Самый Честный Прогноз" - GaM_TW [kitkat]
Прошивка работает с сервером OsMo.mobi / bigapi.ru

Поддерживаемые датчики: 
  - BME-280 (давление, влажность, температура)
  - HTU21D/SHT21/Si7021 (влажность, температура))

# Для начала работы:
  - Скрафтите себе печатную или макетную плату по схеме
  - Воткните туда WEMoS D1 mini
  - Подключите датчики через клеммы
  - Загрузите прошивку в контроллер (параметры Arduino IDE: xxx, yyy, zzz)
  - После успешной прошивки, пПри включении устройство создаст wifi точку доступа для настройки
  - Подключитесь (желательно смартфоном) к wifi-точке с названием GaM_TW_***_SETUP и укажите данные вашей WiFi-сети на портале конфигураторе
    - если портал не открылся автоматически, то откройте в браузере адрес http://192.168.4.1/
  - Для достижения максимального качества данных датчики должны быть размещены в метре от здания в специальном метео-боксе

# Об индикации на устройстве:
Для отображения статуса работы в устройстве используется 2 светодиода. 
Первый - это синий, встроенный в плату контроллера WEMoS он используется для отображения режима настроек и ошибок. При штатной работе устройства, в регулярном режиме, он погашен.
Второй - это распаянный на "материнской" плате светодиод какого-то цвета (как повезёт).
При загрузке и работе устройства работа светодиодов показывает текущий статус системы, как аппаратный, так и программный.
  - быстрое мигание СД 1 в течении примерно 900 мс - ожидание сброса настроек WiFi (только в этот момент он возможен)
  - мигание СД 1 частотой примерно 5 раз в секунду в течении примерно 900 мс - ожидание ручного запроса на загрузку прошивки с сервера (такое мигание появляется только в случае, если в контроллере отключено автоматическое обновление прошивки по интернету)
  - "синхронное" - мигание СД 1 и 2 с частотой примерно 10 раз в секунду (Гц) - устанавливается подключение к WiFi-сети
  - после подключения к интернету, загрузки настроек и проверки обновления прошивки мигание СД 2 меняет частоту на 2 Гц
  - после успешной инициализации устройства на сервере СД 2 гаснет
  - подключаются датчики, СД 1 и 2 начинают мигать с частотой 0.5 Гц и в течении 6 секунд ожидается ответ от BME-280, если он ответит быстрее - следующий шаг, если нет, просто ждём до конца таймера
  - СД 1 и 2 переключаются на мигание 0.25 Гц и ожидается ответ от GY-21, если происходит критическая ошибка с этим датчиком, устройство скорее всего останется на этом этапе и надо проверять физическое соединение
  - после успешной загрузки, все СД перестают мигать, СД 1 - гаснет
  - в рабочем режиме без проблем с сетью СД 2 горит постоянно в соответствии с настройками яркости (их может менять владелец устройства на сайте) и притухает на момент отправки данных на сервер (обычно не более секунды)
  - в рабочем режиме без сети (когда данный собираются в локальный буфер), СД 2 горит тускло, вспыхивая ярко, когда данные записываются в файл (обычно не более 200 мс)


При запуске устройство проверяет наличие новой прошивки на сервере обновления и, если это необходимо, обновляется. Делается системный перезапуск раз в 8 часов (при отсутствии проблем с сетью).

Если, во время работы пропадает WiFi-сеть или сервер перестал отвечать, то данные записываются в локальный файл data.buff через LittleFS. После появления связи, они отправляются несколькими блоками на сервер и обрабатываются. При перезагрузке устройства файл проверяется на наличие содержимого и также отправляется. После успешной отправки он удаляется.

> Вопросы? Пишите gam@gamsh.ru с пометкой TW в теме.
