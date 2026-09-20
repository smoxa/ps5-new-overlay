# PS5 Hardware Overlay 🎮

Легковесный автономный оверлей мониторинга температур и нагрузки для взломанной PlayStation 5.

[English](#english) | [Русский](#русский)

---

## Русский

Специализированное легковесное решение для постоянного отображения аппаратных показателей PlayStation 5 поверх игр и системного меню. В отличие от полных наборов вроде onionHEN / etaHEN, данный проект сфокусирован **только на функции оверлея**: он не содержит лишних инструментов, читов, FTP-серверов или тяжелых зависимостей.

### Основные возможности

- 🌡️ **Температура CPU**: Прямое чтение датчика процессора в реальном времени.
- 🎯 **Загрузка CPU**: Расчет процента нагрузки (средний или по каждому из 8 ядер).
- 🔥 **Температура GPU / APU (SOC)**: Мониторинг нагрева графического чипа.
- 🎮 **Использование VRAM**: Процент занятой видеопамяти.
- 💾 **Использование RAM**: Занятая и общая системная память в МБ и процентах.
- 🌀 **Обороты кулера (Fan Duty)**: Скорость вращения вентилятора в процентах.
- 📺 **HUD-бар**: Тонкая полупрозрачная полоса сверху или снизу экрана.
- ⚙️ **Гибкая конфигурация**: Настройка через файл `config.ini`.

---

### Быстрый старт

#### Вариант 1: Запуск через ELF Loader (Port 9020 / 9021)
После активации джейлбрейка и запуска загрузчика пейлоадов отправьте скомпилированный файл на консоль:
```bash
# Через Netcat на порт 9020 (или 9021):
nc -w 3 <IP_PS5> 9020 < ps5_overlay.elf
```
или воспользуйтесь любой утилитой для отправки пейлоадов (PS5 Payload Sender / Netcat GUI).

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
position = top          ; "top" или "bottom"
show_cpu_temp = true    ; Температура CPU
show_cpu_load = true    ; Загрузка CPU %
show_all_cores = false  ; Нагрузка по ядрам
show_gpu_temp = true    ; Температура GPU (SOC)
show_gpu_load = true    ; Загрузка VRAM %
show_ram = true         ; Использование RAM
show_fan = true         ; Обороты кулера %
background = true       ; Полупрозрачная полоса
font_size = 18          ; Размер шрифта
interval_ms = 1000      ; Интервал обновления (мс)
```

---

<a name="english"></a>
## English

A lightweight, standalone hardware monitoring overlay for jailbroken PlayStation 5 consoles.

Unlike full HEN suites (like onionHEN or etaHEN), this project focuses **exclusively on the overlay experience**: minimal footprint, zero bloat, no cheats, and no heavy background payloads.

### Features

- 🌡️ **CPU Temperature**: Real-time reading of internal CPU temperature sensors.
- 🎯 **CPU Utilization**: Accurate CPU load percentage (overall average or per-core).
- 🔥 **GPU / SOC Temperature**: APU / SOC thermal monitoring.
- 🎮 **VRAM Usage**: Video memory usage percentage.
- 💾 **System RAM**: Memory footprint in MB and percentage.
- 🌀 **Fan Duty Cycle**: Real-time cooling fan speed percentage.
- 📺 **Non-intrusive HUD**: Displays smoothly over games and ShellUI.
- ⚙️ **Configurable**: Managed via `/data/ps5_overlay/config.ini`.

---

## Сборка и GitHub Releases

Проект настроен для автоматической компиляции и выпуска релизов через **GitHub Actions**:

1. **Автоматическая сборка**: При каждом коммите в ветку `main` GitHub Actions собирает `ps5_overlay.elf` с помощью актуального `ps5-payload-sdk` и сохраняет артефакты.
2. **Публикация релиза**:
   - При создании тега версии:
     ```bash
     git tag v1.0.0
     git push origin v1.0.0
     ```
   - Или вручную через вкладку **Actions** -> **Build and Release PS5 Overlay** -> **Run workflow** (с флагом *Publish a GitHub Release*).
   GitHub Action автоматически скомпилирует бинарный файл, сгенерирует `SHA256SUMS.txt` и прикрепит их к новому релизу в разделе **Releases**.

---

## Лицензия

Распространяется под лицензией MIT / GPLv3.
Отдельная благодарность разработчикам `ps5-payload-dev`, `etaHEN` и сообществу сцены PS5 за исследования системных вызовов Prospero.
