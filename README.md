# vuesp.kc868-a4

KC868-A4: 4 реле, 4 входа, 4 ADC, 2 DAC, RS232, 433 МГц и ИК.

Проект собран на базе [Vuesp](https://github.com/bondrogeen/vuesp) (шаблон `5f4e625`): прошивка **5.0.0** PlatformIO + web UI **5.0.0** на Vue 3 и `vuesp-components@5.2.1`.

## Документация

Подробное описание платы, распиновка, RS232, ИК, ScriptRunner и ограничения — в [DOCS.md](./DOCS.md).

## Особенности

- 4 реле на GPIO2 / 15 / 5 / 4, активный уровень **high**.
- 4 оптоизолированных входа на GPIO36 / 39 / 27 / 14, активный уровень **low**.
- 4 ADC: A1–A4 на GPIO34 / 35 / 32 / 33 (2 × 0–5 В, 2 × 4–20 мА).
- 2 DAC: GPIO25 / 26, выход 0–10 В через ОУ.
- RS232: RXD GPIO16, TXD GPIO17, SP3232EEN, DB9; страница терминала `/device/rs232/`.
- ИК: приёмник GPIO23, передатчик GPIO22; страница `/device/ir/`, приём и отправка NEC/raw.
- DS18B20 на GPIO13, бузер на GPIO18, кнопка на GPIO0.
- 433 МГц: TX GPIO21, RX GPIO19 (порты заведены, протокол — позже).
- Настройки устройства, портов, RS232 и ИК — в `ConfigStore` (`/tmp/config.bin`), протокол — `src/const/protocol.h`.
- Диагностика загрузки и loop-метрик, обновление прошивки и LittleFS через веб.

## Плата и окружение

- PlatformIO env: `esp32`
- Прошивка: `platformio.ini`
- Web UI: `vue/`
- Файловая система: LittleFS, веб-сборка попадает в `data/www`

## Сборка

```bash
cd vue
npm ci
npm run protocol
npm run build-web
```

Полная сборка (web + прошивка):

```bash
cd vue
npm run build
```

Либо отдельно:

```bash
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run -t buildfs
```

Заливка прошивки и веб-сборки:

```bash
~/.platformio/penv/bin/pio run -t upload
~/.platformio/penv/bin/pio run -t uploadfs
```

## Возможности

- локальный веб-интерфейс без облака;
- настройка Wi-Fi, безопасности, GPIO и файловой системы;
- dashboard с плитками реле, входов, ADC, DAC и бузера;
- терминал RS232 и страница ИК;
- сценарии и автоматизация;
- обновление прошивки и файловой системы через веб-интерфейс.

## CI

GitHub Actions собирает web UI, прошивку и LittleFS для `esp32`, запускает тесты ScriptRunner и публикует артефакты в релиз при пуше тега.

## Репозиторий

<https://github.com/bondrogeen/vuesp.kc868-a4>
