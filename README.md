# PS5 Hardware Overlay 🎮

Легковесный автономный оверлей мониторинга температур и нагрузки для взломанной PlayStation 5 (поддержка FW 1.xx - 10.xx+).

[English](#english) | [Русский](#русский)

---

## Русский

Специализированное легковесное решение для мониторинга аппаратных показателей PlayStation 5 в реальном времени. В отличие от громоздких наборов вроде onionHEN / etaHEN, данный проект сфокусирован **исключительно на функции оверлея**: он не содержит лишних инструментов, читов, FTP-серверов или тяжелых зависимостей и не рискует вызвать панику ядра на высоких прошивках (включая FW 10.20).

### Основные возможности

- ⚡ **Аппаратный счётчик кадров (FPS Counter)**: Замер реальной частоты вертикальной синхронизации и смены кадров дисплея через Display Controller Engine (`/dev/dce`) или секлок OnionHEN (`/system_tmp/fps_sample`).
- 🖥️ **Нативный экранный оверлей (In-Game HUD)**: Настоящая полоса мониторинга, отрисовываемая прямо поверх игр через внедрение в `SceShellUI` (с активным `kstuff`).
- 🎨 **Цветовая дифференциация метрик**:
  - **FPS**: золотисто-жёлтый (`#FFEB3B`)
  - **CPU**: зеленый (`#66FF66`)
  - **GPU**: фиолетовый (`#B366FF`)
  - **RAM**: оранжевый (`#FFB34D`)
  - **FAN**: бирюзовый (`#33E0FF`)
- 🔕 **Без спама уведомлениями**: Постоянно всплывающие тосты отключены по умолчанию, чтобы не мешать погружению в игру.
- 🌡️ **Температура CPU & GPU (SoC)**: Прямое чтение сенсоров в реальном времени (°C).
- 💾 **Использование RAM & VRAM**: Объем занятой системной и видеопамяти.
- 🌀 **Обороты кулера (Fan Duty)**: Скорость вращения вентилятора в процентах.
- ⚙️ **Гибкая конфигурация**: Настройка через файл `/data/ps5_overlay/config.ini`.

---

### Отображение в играх

- **Наэкранный HUD в играх (SceShellUI)**:
  - Внедряется автоматически в системный процесс `SceShellUI`.
  - В верхней части экрана во время любой игры отображается аккуратная полупрозрачная плашка с цветными показателями:
    ```text
    FPS: 60  |  CPU: 58°C  |  GPU: 61°C  |  RAM: 4.8 GB  |  FAN: 35%
    ```

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
show_fps = true             ; Счётчик FPS (Hardware DCE / OnionHEN)
show_cpu_temp = true        ; Температура CPU
show_cpu_load = true        ; Загрузка CPU %
show_all_cores = false      ; Нагрузка по ядрам
show_gpu_temp = true        ; Температура GPU (SOC)
show_gpu_load = true        ; Загрузка VRAM %
show_ram = true             ; Использование RAM
show_fan = true             ; Обороты кулера %
interval_ms = 1000          ; Опрос сенсоров (мс)

toast_notifications = false ; Экранный OSD в играх (true/false)
toast_interval_sec = 10     ; Интервал обновления OSD (сек)
```

---

<a name="english"></a>
## English

A lightweight, standalone hardware monitoring overlay for jailbroken PlayStation 5 consoles (FW 1.xx - 10.xx+).

Unlike full HEN suites (like onionHEN or etaHEN), this project focuses **exclusively on the overlay experience**: minimal footprint, zero bloat, no cheats, and no risk of kernel panics on newer firmwares (including FW 10.20).

### Features

- ⚡ **Real-Time In-Game FPS Counter**: Accurate framerate monitoring sampled via Display Controller Engine (`/dev/dce`) hardware flip deltas or OnionHEN seqlock sampler (`/system_tmp/fps_sample`).
- 🖥️ **Native In-Game HUD**: Translucent top-screen status bar rendered seamlessly over active gameplay via Mono PUI in `SceShellUI`.
- 🌡️ **CPU Temperature**: Real-time reading of internal Zen 2 CPU sensors.
- 🎯 **CPU Utilization**: Accurate CPU load percentage and 8-core tracks.
- 🔥 **GPU / SoC Temperature**: APU / SoC thermal monitoring.
- 🎮 **VRAM Usage**: Video memory usage percentage and MB.
- 💾 **System RAM**: Memory footprint in GB and percentage (`4.8 / 16.0 GB`).
- 🌀 **Fan Duty Cycle**: Real-time cooling fan speed percentage.
- 🔕 **Zero Toast Spam**: Notification popups disabled by default to protect gameplay immersion.
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
