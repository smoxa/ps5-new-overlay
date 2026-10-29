# PS5 Overlay (KUS Edition) 🎮⚡

🇷🇺 [Русская версия README](README_RU.md)

**PS5 In-Game Telemetry Overlay** is a standalone, lightweight real-time performance and system telemetry in-game overlay for **PlayStation 5 (Jailbreak)**.

The overlay renders on top of any game and the `SceShellUI` system menu using native PlayStation PUI (Mono) UI components. Management and fine-tuning are handled via a built-in **web interface** directly from your phone or PC.

---

## ✨ Key Features

- 📊 **12 Real-Time Telemetry Metrics**:
  1. **FPS** — Hardware frame rate via display controller flip counter (`/dev/dce`) plus a software fallback using `SceVshAppSystem` hooks.
  2. **CPU** — AMD Zen 2 core temperature (`sceKernelGetCpuTemperature`).
  3. **LOAD** — Total CPU utilization percentage calculated based on thread execution time.
  4. **CCLK** — Current CPU core frequency in MHz (`sceKernelGetCpuCoreClock` / `sceKernelGetCpuFrequency`).
  5. **GPU** — RDNA 2 SoC graphics chip temperature (`sceKernelGetSocSensorTemperature`, channel 0).
  6. **GCLK** — Actual GFXCLK graphics core frequency in MHz (`sceKernelGetSocClock`).
  7. **RAM** — System RAM usage (`get_page_table_stats`).
  8. **VRAM** — Video memory usage of the graphics accelerator (`get_page_table_stats`).
  9. **SSD** — M.2 NVMe storage temperature via SoC controller sensor (`sceKernelGetSocSensorTemperature`, channel 2). Displays `N/A` if the slot is empty.
  10. **FAN** — Current cooling fan duty cycle percentage (`sceKernelGetCurrentFanDuty`).
  11. **PWR** — SoC APU power consumption in Watts (`sceKernelGetSocPowerConsumption`).
  12. **IP** — Local console IPv4 network address.

- 🌐 **Built-in Web Control Panel (Port 8585)**:
  * Accessible from any device on your local network: `http://<PS5_IP>:8585`.
  * **Live Preview** of the HUD right in your browser window.
  * **Sorting & Filtering**: Enable/disable any metric and change display order using ▲ / ▼ buttons.
  * **Color Palette**: Custom HEX color selection for each telemetry parameter.
  * **Positioning**: Choose layout placement (Top / Bottom, Left / Center / Right) with fine X/Y offset tuning.
  * **Appearance**: Customize font size, bar height, background color, and transparency.
  * 🛑 **Hot Unload**: Safe shutdown button to unload the overlay and free memory without rebooting the console.
  * **Auto-Save**: All changes are saved to `/data/overlay/config.ini` and automatically loaded on startup.

- 🚀 **Standalone Loader (All-In-One ELF)**:
  * Single executable file `ps5-overlay-kus_vX.X.X.elf`.
  * Automatically finds the `SceShellUI` process, escalates privileges (`escalate_process_caps`), allocates remote memory via `ptrace`, maps the payload, and starts a working thread with proper Mono domain attachment (`mono_thread_attach`).
  * Safe pre-resolution of system kernel symbols prior to injection prevents crashes and freezes.

---

## 🛠 Quick Start & Usage

### 1. Sending to the Console
Send the compiled ELF to your console via Netcat or Socat to the payload loader port (e.g., 9021 or 9020):

```bash
# Using netcat
nc -q0 192.168.1.100 9021 < bin/ps5-overlay-kus_v1.0.19.elf

# Or using socat
socat - TCP:192.168.1.100:9021 < bin/ps5-overlay-kus_v1.0.19.elf

```

Upon successful injection, a system notification will appear on your screen:

> **"PS5 Overlay Loaded successfully!"**

### 2. Browser Configuration

Open the following address in your smartphone or PC browser:

```
http://<YOUR_PS5_IP>:8585

```

*(If port 8585 is busy, the server will automatically fall back to an alternative port: 8282, 8989, or 9876).*

---

## 🏗 Project Architecture

```
PS5_Overlay/
├── Makefile                # Build rules for payload and loader
├── common/                 # Shared headers, structures, and NIDs
│   ├── bootstrap.h         # Pointer passing arguments from Loader to Payload
│   ├── log.h               # System logging macros
│   └── nid.c               # NID resolution for libkernel_sys
├── loader/                 # Loader / Injector (ps5-overlay-kus.elf)
│   ├── main.c              # Entry point, SceShellUI search
│   ├── injector.c          # Shellcode injector and symbol pre-resolving
│   ├── proc.c              # Process PID search via sysctl
│   ├── ucred.c             # Privilege escalation (jailbreak_process)
│   ├── pt.c                # ptrace wrappers
│   ├── elfldr.c            # Remote ELF loader
│   ├── notify.c            # System toast notification output
│   └── embedded_payload.S  # overlay_payload.elf embedding section
└── payload/                # Payload (injected into SceShellUI)
    ├── main.cpp            # Mono hooks, HUD building, telemetry polling
    ├── config.cpp / .hpp   # INI configuration parser & generator (/data/overlay/config.ini)
    ├── web_server.cpp      # Built-in HTTP web server and REST API
    ├── mono_runtime.cpp    # Mono runtime interaction
    └── payload_log.cpp     # Payload logging

```

---

## ⚙️ Building from Source

A Linux environment with **[ps5-payload-sdk](https://github.com/ps5-payload-dev/ps5-payload-sdk)** installed is required.

```bash
# Clone the repository
git clone [https://github.com/smoxa/ps5-new-overlay.git](https://github.com/smoxa/ps5-new-overlay.git)
cd ps5-new-overlay

# Build the project
make clean && make

# The compiled binary will be available in the bin/ directory:
# bin/ps5-overlay-kus_v1.0.19.elf

```

---

## 📝 Configuration File

All settings are stored on the console at `/data/overlay/config.ini`:

```ini
[overlay]
enabled = true
pos_v = top
pos_h = left
offset_x = 24
offset_y = 0
font_size = 18
bar_height = 34
bg_visible = true
bg_color = #000000
bg_opacity = 0.70
port = 8585

[metrics]
order = fps,cpu,load,cclk,gpu,gclk,ram,vram,ssd,fan,pwr,ip

[fps]
enabled = true
color = #FFEB3B

[ssd]
enabled = true
color = #00E5FF
...

```

---

## Config troubleshooting

The configuration example is an excerpt; the ... line is a placeholder, not an INI setting to copy. Use metric IDs as section names, such as [cpu], [gpu], and [ip] (not [metric_cpu]). The payload reads /data/overlay/config.ini when it starts, so restart or reload the overlay after editing the file manually. To hide the console address, set enabled = false under [ip].



## 🤝 Acknowledgments

* The **ps5-payload-sdk** developers
* **ProsperoMgr** and **ps5-hwinfo** projects for reverse engineering and researching PlayStation 5 SoC APU sensors
* The **OnionHEN** team for baseline work on Mono PUI integration
