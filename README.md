# PS5 Hardware Overlay 🎮

Легковесный автономный оверлей мониторинга температур и нагрузки для взломанной PlayStation 5 (поддержка FW 1.xx - 10.xx+).

[English](#english) | [Русский](#русский)

---

## Русский

Специализированное легковесное решение для мониторинга аппаратных показателей PlayStation 5 в реальном времени. В отличие от громоздких наборов вроде onionHEN / etaHEN, данный проект сфокусирован **исключительно на функции оверлея**: он не содержит лишних инструментов, читов, FTP-серверов или тяжелых зависимостей и не рискует вызвать панику ядра на высоких прошивках (включая FW 10.20).

### Основные возможности

- 🌡️ **Температура CPU**: Прямое чтение датчика процессора в реальном времени (°C).
- 🎯 **Загрузка CPU**: Расчет процента нагрузки (общий процент и индикаторы по 8 ядрам Zen 2).
- 🔥 **Температура GPU / APU (SoC)**: Мониторинг нагрева графического кристалла.
- 🎮 **Использование VRAM**: Процент и объем занятой видеопамяти.
- 💾 **Использование RAM**: Занятая и общая системная память (например, `4.8 / 16.0 GB`).
- 🌀 **Обороты кулера (Fan Duty)**: Скорость вращения вентилятора в процентах.
- 🔔 **Нативный In-Game Toast OSD**: Двухстрочные уведомления прямо поверх любой игры каждые 4 секунды (с режимом `OverwriteLatest` — без спама в историю уведомлений).
- 🌐 **Встроенный Web HUD (Порт 8080)**: Стильный веб-дашборд в темной теме PlayStation с живыми графиками и индикаторами.
- ⚙️ **Гибкая конфигурация**: Настройка через файл `/data/ps5_overlay/config.ini`.

---

### Режимы отображения

1. **Экранный OSD (Toast Notifications)**:
   - Работает «из коробки» прямо поверх игр и меню консоли.
   - Каждые 4 секунды в правом верхнем углу плавно обновляется двухстрочная плашка:
     ```text
     CPU: 58°C (24%)  |  GPU: 61°C
     RAM: 4.8 / 16.0 GB (30%)  |  FAN: 35%
     ```
2. **Второй экран (Смартфон / Планшет / ПК)**:
   - Откройте в браузере любого устройства в домашней сети адрес:
     ```text
     http://<IP_ВАШЕЙ_PS5>:8080/
     ```
   - Вы увидите живой дашборд с индикаторами температуры, шкалами нагрузки и частотой опроса 1 раз в секунду.
3. **Закрепить сбоку на PS5 (Pin to Side)**:
   - В браузере PS5 перейдите на `http://127.0.0.1:8080/`.
   - Нажмите кнопку **Options** на контроллере DualSense и выберите **«Закрепить сбоку» (Pin to Side)**.
   - Теперь дашборд будет зафиксирован сбоку экрана прямо во время вашей игры!

---

### Быстрый старт

#### Вариант 1: Запуск через ELF Loader (Port 9020 / 9021)
После активации джейлбрейка отправьте скомпилированный `.elf` на консоль:
```bash
# Через Netcat на порт 9020 (или 9021):
nc -w 3 <IP_PS5> 9020 < ps5_overlay.elf
```
или воспользуйтесь любой программой для отправки пейлоадов (PS5 Payload Sender / Netcat GUI).

#### Вариант 2: Автозапуск через менеджер пейлоадов
Скопируйте `ps5_overlay.elf` в директорию пейлоадов на консоли:
```text
/data/OnionHEN/payloads/ps5_overlay.elf
# или
/data/etaHEN/payloads/ps5_overlay.elf
```
и включите автозапуск в меню консоли.

---

### Настройка (`config.ini`)

Файл конфигурации можно разместить по пути:
```text
/data/ps5_overlay/config.ini
```
Пример настроек (`config.ini.example`):
```ini
[ps5_overlay]
enabled = true
show_cpu_temp = true        ; Температура CPU
show_cpu_load = true        ; Загрузка CPU %
show_all_cores = false      ; Нагрузка по ядрам
show_gpu_temp = true        ; Температура GPU (SOC)
show_gpu_load = true        ; Загрузка VRAM %
show_ram = true             ; Использование RAM
show_fan = true             ; Обороты кулера %
interval_ms = 1000          ; Опрос сенсоров (мс)

toast_notifications = true  ; Экранный OSD в играх (true/false)
toast_interval_sec = 4      ; Интервал обновления OSD (сек)

web_server = true           ; Встроенный Web HUD сервер
web_port = 8080             ; Порт Web HUD (по умолч. 8080)
```

---

<a name="english"></a>
## English

A lightweight, standalone hardware monitoring overlay for jailbroken PlayStation 5 consoles (FW 1.xx - 10.xx+).

Unlike full HEN suites (like onionHEN or etaHEN), this project focuses **exclusively on the overlay experience**: minimal footprint, zero bloat, no cheats, and no risk of kernel panics on newer firmwares (including FW 10.20).

### Features

- 🌡️ **CPU Temperature**: Real-time reading of internal Zen 2 CPU sensors.
- 🎯 **CPU Utilization**: Accurate CPU load percentage and 8-core tracks.
- 🔥 **GPU / SoC Temperature**: APU / SoC thermal monitoring.
- 🎮 **VRAM Usage**: Video memory usage percentage and MB.
- 💾 **System RAM**: Memory footprint in GB and percentage (`4.8 / 16.0 GB`).
- 🌀 **Fan Duty Cycle**: Real-time cooling fan speed percentage.
- 🔔 **Native In-Game Toast OSD**: Dual-line HUD notifications updating smoothly over games (`OverwriteLatest`).
- 🌐 **Embedded Web HUD (Port 8080)**: Dark PlayStation-themed real-time dashboard accessible via smartphone, PC, or PS5 "Pin to Side" browser.
- ⚙️ **Configurable**: Managed via `/data/ps5_overlay/config.ini`.

---

## Сборка и GitHub Releases

Проект настроен для автоматической компиляции и выпуска релизов через **GitHub Actions**:

1. **Автоматическая сборка**: При каждом коммите в ветку `main` GitHub Actions собирает `ps5_overlay.elf` с помощью актуального `ps5-payload-sdk` и сохраняет артефакты.
2. **Публикация релиза**:
   - При создании тега версии:
     ```bash
     git tag v1.0.1
     git push origin v1.0.1
     ```
   - Или вручную через вкладку **Actions** -> **Build and Release PS5 Overlay** -> **Run workflow**.

GitHub Action автоматически скомпилирует бинарный файл, сгенерирует `SHA256SUMS.txt` и прикрепит их к новому релизу в разделе **Releases**.

---

## Лицензия

Распространяется под лицензией MIT / GPLv3.
Отдельная благодарность разработчикам `ps5-payload-dev`, `etaHEN` и сообществу сцены PS5.
