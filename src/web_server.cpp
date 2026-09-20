#include "web_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
typedef int socklen_t;
#define close_socket(s) closesocket(s)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#define close_socket(s) close(s)
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#endif

static volatile bool s_server_running = false;
static SOCKET s_listen_sock = INVALID_SOCKET;
static HardwareMetrics s_latest_metrics{};

#ifdef _WIN32
static CRITICAL_SECTION s_metrics_cs;
static HANDLE s_server_thread = NULL;
#else
static pthread_mutex_t s_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t s_server_thread;
#endif

static const char HTML_OVERLAY_PAGE[] =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"<title>PS5 In-Game Overlay Bar</title>\n"
"<style>\n"
"  :root {\n"
"    --bg-bar: rgba(10, 14, 23, 0.88);\n"
"    --border: rgba(0, 162, 255, 0.35);\n"
"    --cpu: #00ff88;\n"
"    --gpu: #b366ff;\n"
"    --ram: #ffaa00;\n"
"    --fan: #00e5ff;\n"
"    --text: #ffffff;\n"
"    --sub: #94a3b8;\n"
"  }\n"
"  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
"  html, body {\n"
"    background: transparent !important;\n"
"    color: var(--text);\n"
"    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n"
"    overflow: hidden;\n"
"  }\n"
"  .overlay-container {\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    padding: 8px;\n"
"    gap: 8px;\n"
"  }\n"
"  .hud-bar {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 12px;\n"
"    background: var(--bg-bar);\n"
"    border: 1px solid var(--border);\n"
"    border-radius: 8px;\n"
"    padding: 6px 14px;\n"
"    box-shadow: 0 4px 18px rgba(0, 0, 0, 0.6);\n"
"    backdrop-filter: blur(10px);\n"
"    font-size: 14px;\n"
"    font-weight: 600;\n"
"    flex-wrap: wrap;\n"
"  }\n"
"  .stat-group {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 6px;\n"
"  }\n"
"  .pill {\n"
"    font-size: 11px;\n"
"    font-weight: 800;\n"
"    padding: 1px 6px;\n"
"    border-radius: 4px;\n"
"    text-transform: uppercase;\n"
"    letter-spacing: 0.5px;\n"
"  }\n"
"  .pill-cpu { background: rgba(0, 255, 136, 0.18); color: var(--cpu); border: 1px solid var(--cpu); }\n"
"  .pill-gpu { background: rgba(179, 102, 255, 0.18); color: var(--gpu); border: 1px solid var(--gpu); }\n"
"  .pill-ram { background: rgba(255, 170, 0, 0.18); color: var(--ram); border: 1px solid var(--ram); }\n"
"  .pill-fan { background: rgba(0, 229, 255, 0.18); color: var(--fan); border: 1px solid var(--fan); }\n"
"  .stat-val { color: #fff; }\n"
"  .stat-sub { color: var(--sub); font-size: 12px; }\n"
"  .sep { color: rgba(255, 255, 255, 0.2); }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"overlay-container\">\n"
"  <div class=\"hud-bar\">\n"
"    <div class=\"stat-group\">\n"
"      <span class=\"pill pill-cpu\">CPU</span>\n"
"      <span class=\"stat-val\" id=\"cpu-temp\">-- &deg;C</span>\n"
"      <span class=\"stat-sub\" id=\"cpu-load\">(--%)</span>\n"
"    </div>\n"
"    <span class=\"sep\">|</span>\n"
"    <div class=\"stat-group\">\n"
"      <span class=\"pill pill-gpu\">GPU</span>\n"
"      <span class=\"stat-val\" id=\"gpu-temp\">-- &deg;C</span>\n"
"      <span class=\"stat-sub\" id=\"gpu-load\">(--%)</span>\n"
"    </div>\n"
"    <span class=\"sep\">|</span>\n"
"    <div class=\"stat-group\">\n"
"      <span class=\"pill pill-ram\">RAM</span>\n"
"      <span class=\"stat-val\" id=\"ram-val\">-- GB</span>\n"
"      <span class=\"stat-sub\" id=\"ram-pct\">(--%)</span>\n"
"    </div>\n"
"    <span class=\"sep\">|</span>\n"
"    <div class=\"stat-group\">\n"
"      <span class=\"pill pill-fan\">FAN</span>\n"
"      <span class=\"stat-val\" id=\"fan-val\">-- %</span>\n"
"    </div>\n"
"  </div>\n"
"</div>\n"
"<script>\n"
"  async function refresh() {\n"
"    try {\n"
"      const r = await fetch('/api/metrics');\n"
"      const d = await r.json();\n"
"      document.getElementById('cpu-temp').textContent = `${d.cpu_temp} °C`;\n"
"      document.getElementById('cpu-load').textContent = `(${d.cpu_usage.toFixed(0)}%)`;\n"
"      document.getElementById('gpu-temp').textContent = `${d.soc_temp} °C`;\n"
"      document.getElementById('gpu-load').textContent = `(${d.vram_pct.toFixed(0)}%)`;\n"
"      const ramGb = (d.ram_used_mb / 1024).toFixed(1);\n"
"      const ramTot = (d.ram_total_mb / 1024).toFixed(1);\n"
"      document.getElementById('ram-val').textContent = `${ramGb}/${ramTot} GB`;\n"
"      document.getElementById('ram-pct').textContent = `(${d.ram_pct.toFixed(0)}%)`;\n"
"      document.getElementById('fan-val').textContent = `${d.fan_duty_pct}%`;\n"
"    } catch(e){}\n"
"  }\n"
"  setInterval(refresh, 1000);\n"
"  refresh();\n"
"</script>\n"
"</body>\n"
"</html>\n";

static const char HTML_PAGE[] =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"<title>PS5 Hardware Overlay HUD</title>\n"
"<style>\n"
"  :root {\n"
"    --bg: #070a12;\n"
"    --card-bg: rgba(18, 25, 41, 0.85);\n"
"    --card-border: rgba(0, 162, 255, 0.25);\n"
"    --ps-blue: #0070d1;\n"
"    --ps-cyan: #00c8ff;\n"
"    --ps-green: #00e676;\n"
"    --ps-amber: #ffab00;\n"
"    --ps-red: #ff1744;\n"
"    --text: #e2e8f0;\n"
"    --text-dim: #8ba2c4;\n"
"  }\n"
"  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
"  body {\n"
"    background: var(--bg);\n"
"    background-image: radial-gradient(circle at 50% 0%, #152238 0%, #070a12 75%);\n"
"    color: var(--text);\n"
"    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
"    padding: 20px;\n"
"    min-height: 100vh;\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    align-items: center;\n"
"  }\n"
"  .container {\n"
"    width: 100%;\n"
"    max-width: 900px;\n"
"  }\n"
"  header {\n"
"    display: flex;\n"
"    justify-content: space-between;\n"
"    align-items: center;\n"
"    margin-bottom: 24px;\n"
"    padding-bottom: 14px;\n"
"    border-bottom: 1px solid var(--card-border);\n"
"  }\n"
"  .logo {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 12px;\n"
"  }\n"
"  .logo h1 {\n"
"    font-size: 1.4rem;\n"
"    font-weight: 700;\n"
"    letter-spacing: 1.5px;\n"
"    color: #fff;\n"
"    text-transform: uppercase;\n"
"  }\n"
"  .logo span {\n"
"    color: var(--ps-cyan);\n"
"  }\n"
"  .badge {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"    background: rgba(0, 200, 255, 0.12);\n"
"    border: 1px solid var(--ps-cyan);\n"
"    color: var(--ps-cyan);\n"
"    padding: 4px 12px;\n"
"    border-radius: 20px;\n"
"    font-size: 0.75rem;\n"
"    font-weight: 600;\n"
"    letter-spacing: 1px;\n"
"  }\n"
"  .pulse-dot {\n"
"    width: 8px;\n"
"    height: 8px;\n"
"    border-radius: 50%;\n"
"    background: var(--ps-green);\n"
"    box-shadow: 0 0 8px var(--ps-green);\n"
"    animation: pulse 1.8s infinite;\n"
"  }\n"
"  @keyframes pulse {\n"
"    0% { transform: scale(0.9); opacity: 0.8; }\n"
"    50% { transform: scale(1.3); opacity: 1; }\n"
"    100% { transform: scale(0.9); opacity: 0.8; }\n"
"  }\n"
"  .grid {\n"
"    display: grid;\n"
"    grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));\n"
"    gap: 18px;\n"
"    margin-bottom: 24px;\n"
"  }\n"
"  .card {\n"
"    background: var(--card-bg);\n"
"    border: 1px solid var(--card-border);\n"
"    border-radius: 12px;\n"
"    padding: 18px;\n"
"    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.45);\n"
"    backdrop-filter: blur(8px);\n"
"    transition: transform 0.2s ease, border-color 0.2s ease;\n"
"  }\n"
"  .card:hover {\n"
"    border-color: var(--ps-cyan);\n"
"    transform: translateY(-2px);\n"
"  }\n"
"  .card-header {\n"
"    display: flex;\n"
"    justify-content: space-between;\n"
"    align-items: center;\n"
"    margin-bottom: 12px;\n"
"  }\n"
"  .card-title {\n"
"    font-size: 0.85rem;\n"
"    font-weight: 600;\n"
"    text-transform: uppercase;\n"
"    color: var(--text-dim);\n"
"    letter-spacing: 1px;\n"
"  }\n"
"  .card-val-large {\n"
"    font-size: 2.2rem;\n"
"    font-weight: 800;\n"
"    line-height: 1.1;\n"
"    margin-bottom: 8px;\n"
"    color: #fff;\n"
"    letter-spacing: -0.5px;\n"
"  }\n"
"  .card-val-sub {\n"
"    font-size: 0.9rem;\n"
"    color: var(--text-dim);\n"
"    margin-bottom: 14px;\n"
"  }\n"
"  .bar-container {\n"
"    width: 100%;\n"
"    height: 8px;\n"
"    background: rgba(255, 255, 255, 0.08);\n"
"    border-radius: 4px;\n"
"    overflow: hidden;\n"
"    margin-bottom: 6px;\n"
"  }\n"
"  .bar-fill {\n"
"    height: 100%;\n"
"    width: 0%;\n"
"    background: linear-gradient(90deg, var(--ps-blue), var(--ps-cyan));\n"
"    border-radius: 4px;\n"
"    transition: width 0.4s ease, background 0.4s ease;\n"
"  }\n"
"  .cores-grid {\n"
"    display: grid;\n"
"    grid-template-columns: repeat(8, 1fr);\n"
"    gap: 4px;\n"
"    margin-top: 14px;\n"
"  }\n"
"  .core-bar-wrapper {\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    align-items: center;\n"
"    gap: 4px;\n"
"  }\n"
"  .core-track {\n"
"    width: 100%;\n"
"    height: 36px;\n"
"    background: rgba(255, 255, 255, 0.06);\n"
"    border-radius: 3px;\n"
"    display: flex;\n"
"    align-items: flex-end;\n"
"    overflow: hidden;\n"
"  }\n"
"  .core-fill {\n"
"    width: 100%;\n"
"    height: 0%;\n"
"    background: var(--ps-cyan);\n"
"    border-radius: 2px;\n"
"    transition: height 0.3s ease;\n"
"  }\n"
"  .core-label {\n"
"    font-size: 0.65rem;\n"
"    color: var(--text-dim);\n"
"  }\n"
"  footer {\n"
"    text-align: center;\n"
"    font-size: 0.75rem;\n"
"    color: var(--text-dim);\n"
"    margin-top: 20px;\n"
"  }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"container\">\n"
"  <header>\n"
"    <div class=\"logo\">\n"
"      <h1>PS5 <span>OVERLAY HUD</span></h1>\n"
"    </div>\n"
"    <div style=\"display:flex;align-items:center;gap:10px;\">\n"
"      <a href=\"/overlay\" style=\"color:var(--ps-cyan);text-decoration:none;font-size:0.75rem;font-weight:700;border:1px solid var(--card-border);padding:4px 12px;border-radius:20px;letter-spacing:1px;background:rgba(0,162,255,0.08);\">OVERLAY BAR</a>\n"
"      <div class=\"badge\" id=\"conn-badge\">\n"
"        <div class=\"pulse-dot\" id=\"pulse-dot\"></div>\n"
"        <span id=\"conn-text\">LIVE</span>\n"
"      </div>\n"
"    </div>\n"
"  </header>\n"
"\n"
"  <div class=\"grid\">\n"
"    <!-- CPU Card -->\n"
"    <div class=\"card\">\n"
"      <div class=\"card-header\">\n"
"        <span class=\"card-title\">CPU (Zen 2)</span>\n"
"        <span id=\"cpu-temp-badge\" style=\"color: var(--ps-cyan); font-weight:700;\">-- &deg;C</span>\n"
"      </div>\n"
"      <div class=\"card-val-large\" id=\"cpu-temp\">-- &deg;C</div>\n"
"      <div class=\"card-val-sub\">Load: <strong id=\"cpu-usage\" style=\"color:#fff;\">-- %</strong></div>\n"
"      <div class=\"bar-container\">\n"
"        <div class=\"bar-fill\" id=\"cpu-bar\"></div>\n"
"      </div>\n"
"      <div class=\"cores-grid\" id=\"cores-grid\">\n"
"        <!-- 8 CPU Cores -->\n"
"      </div>\n"
"    </div>\n"
"\n"
"    <!-- GPU / SoC Card -->\n"
"    <div class=\"card\">\n"
"      <div class=\"card-header\">\n"
"        <span class=\"card-title\">GPU (RDNA 2)</span>\n"
"        <span id=\"gpu-temp-badge\" style=\"color: var(--ps-cyan); font-weight:700;\">-- &deg;C</span>\n"
"      </div>\n"
"      <div class=\"card-val-large\" id=\"gpu-temp\">-- &deg;C</div>\n"
"      <div class=\"card-val-sub\">VRAM: <span id=\"vram-info\">-- / -- GB</span> (<strong id=\"vram-pct\" style=\"color:#fff;\">--%</strong>)</div>\n"
"      <div class=\"bar-container\">\n"
"        <div class=\"bar-fill\" id=\"vram-bar\"></div>\n"
"      </div>\n"
"    </div>\n"
"\n"
"    <!-- RAM Card -->\n"
"    <div class=\"card\">\n"
"      <div class=\"card-header\">\n"
"        <span class=\"card-title\">System RAM</span>\n"
"        <span id=\"ram-pct-badge\" style=\"color: var(--ps-cyan); font-weight:700;\">-- %</span>\n"
"      </div>\n"
"      <div class=\"card-val-large\" id=\"ram-used\">-- GB</div>\n"
"      <div class=\"card-val-sub\">Total: <span id=\"ram-total\">-- GB</span> (<strong id=\"ram-pct\" style=\"color:#fff;\">--%</strong>)</div>\n"
"      <div class=\"bar-container\">\n"
"        <div class=\"bar-fill\" id=\"ram-bar\"></div>\n"
"      </div>\n"
"    </div>\n"
"\n"
"    <!-- Fan / Cooling Card -->\n"
"    <div class=\"card\">\n"
"      <div class=\"card-header\">\n"
"        <span class=\"card-title\">Cooling Fan</span>\n"
"        <span id=\"fan-duty-badge\" style=\"color: var(--ps-cyan); font-weight:700;\">-- %</span>\n"
"      </div>\n"
"      <div class=\"card-val-large\" id=\"fan-duty\">-- %</div>\n"
"      <div class=\"card-val-sub\">Duty Cycle: <strong id=\"fan-status\" style=\"color:#fff;\">Normal</strong></div>\n"
"      <div class=\"bar-container\">\n"
"        <div class=\"bar-fill\" id=\"fan-bar\"></div>\n"
"      </div>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <footer>PS5 Hardware Overlay &bull; Real-time Monitoring &bull; Port 8080</footer>\n"
"</div>\n"
"\n"
"<script>\n"
"  /* Initialize core tracks */\n"
"  const coresGrid = document.getElementById('cores-grid');\n"
"  for (let i = 0; i < 8; i++) {\n"
"    const wrapper = document.createElement('div');\n"
"    wrapper.className = 'core-bar-wrapper';\n"
"    wrapper.innerHTML = `<div class=\"core-track\"><div class=\"core-fill\" id=\"core-fill-${i}\"></div></div><div class=\"core-label\">C${i}</div>`;\n"
"    coresGrid.appendChild(wrapper);\n"
"  }\n"
"\n"
"  function getTempColor(temp) {\n"
"    if (temp >= 75) return 'var(--ps-red)';\n"
"    if (temp >= 65) return 'var(--ps-amber)';\n"
"    return 'var(--ps-cyan)';\n"
"  }\n"
"\n"
"  async function fetchMetrics() {\n"
"    try {\n"
"      const res = await fetch('/api/metrics');\n"
"      if (!res.ok) throw new Error('HTTP ' + res.status);\n"
"      const d = await res.json();\n"
"\n"
"      /* CPU */\n"
"      const cpuTempEl = document.getElementById('cpu-temp');\n"
"      cpuTempEl.textContent = `${d.cpu_temp} °C`;\n"
"      cpuTempEl.style.color = getTempColor(d.cpu_temp);\n"
"      document.getElementById('cpu-temp-badge').textContent = `${d.cpu_temp} °C`;\n"
"      document.getElementById('cpu-usage').textContent = `${d.cpu_usage.toFixed(0)}%`;\n"
"      document.getElementById('cpu-bar').style.width = `${Math.min(100, Math.max(0, d.cpu_usage))}%`;\n"
"\n"
"      if (d.cpu_cores && Array.isArray(d.cpu_cores)) {\n"
"        for (let i = 0; i < 8 && i < d.cpu_cores.length; i++) {\n"
"          const cf = document.getElementById(`core-fill-${i}`);\n"
"          if (cf) cf.style.height = `${Math.min(100, Math.max(0, d.cpu_cores[i]))}%`;\n"
"        }\n"
"      }\n"
"\n"
"      /* GPU */\n"
"      const gpuTempEl = document.getElementById('gpu-temp');\n"
"      gpuTempEl.textContent = `${d.soc_temp} °C`;\n"
"      gpuTempEl.style.color = getTempColor(d.soc_temp);\n"
"      document.getElementById('gpu-temp-badge').textContent = `${d.soc_temp} °C`;\n"
"      const vramUsedGb = (d.vram_used_mb / 1024).toFixed(1);\n"
"      const vramTotGb = (d.vram_total_mb / 1024).toFixed(1);\n"
"      document.getElementById('vram-info').textContent = `${vramUsedGb} / ${vramTotGb} GB`;\n"
"      document.getElementById('vram-pct').textContent = `${d.vram_pct.toFixed(0)}%`;\n"
"      document.getElementById('vram-bar').style.width = `${Math.min(100, Math.max(0, d.vram_pct))}%`;\n"
"\n"
"      /* RAM */\n"
"      const ramUsedGb = (d.ram_used_mb / 1024).toFixed(1);\n"
"      const ramTotGb = (d.ram_total_mb / 1024).toFixed(1);\n"
"      document.getElementById('ram-used').textContent = `${ramUsedGb} GB`;\n"
"      document.getElementById('ram-total').textContent = `${ramTotGb} GB`;\n"
"      document.getElementById('ram-pct').textContent = `${d.ram_pct.toFixed(0)}%`;\n"
"      document.getElementById('ram-pct-badge').textContent = `${d.ram_pct.toFixed(0)}%`;\n"
"      document.getElementById('ram-bar').style.width = `${Math.min(100, Math.max(0, d.ram_pct))}%`;\n"
"\n"
"      /* Fan */\n"
"      document.getElementById('fan-duty').textContent = `${d.fan_duty_pct}%`;\n"
"      document.getElementById('fan-duty-badge').textContent = `${d.fan_duty_pct}%`;\n"
"      document.getElementById('fan-bar').style.width = `${Math.min(100, Math.max(0, d.fan_duty_pct))}%`;\n"
"      document.getElementById('fan-status').textContent = d.fan_duty_pct > 60 ? 'High' : (d.fan_duty_pct > 40 ? 'Moderate' : 'Quiet');\n"
"\n"
"      document.getElementById('conn-text').textContent = 'LIVE';\n"
"      document.getElementById('conn-badge').style.borderColor = 'var(--ps-cyan)';\n"
"      document.getElementById('pulse-dot').style.background = 'var(--ps-green)';\n"
"    } catch (e) {\n"
"      document.getElementById('conn-text').textContent = 'OFFLINE';\n"
"      document.getElementById('conn-badge').style.borderColor = 'var(--ps-red)';\n"
"      document.getElementById('pulse-dot').style.background = 'var(--ps-red)';\n"
"    }\n"
"  }\n"
"\n"
"  setInterval(fetchMetrics, 1000);\n"
"  fetchMetrics();\n"
"</script>\n"
"</body>\n"
"</html>\n";

static void lock_metrics() {
#ifdef _WIN32
    EnterCriticalSection(&s_metrics_cs);
#else
    pthread_mutex_lock(&s_metrics_mutex);
#endif
}

static void unlock_metrics() {
#ifdef _WIN32
    LeaveCriticalSection(&s_metrics_cs);
#else
    pthread_mutex_unlock(&s_metrics_mutex);
#endif
}

void web_server_update_metrics(const HardwareMetrics* metrics) {
    if (!metrics) return;
    lock_metrics();
    s_latest_metrics = *metrics;
    unlock_metrics();
}

static void handle_client(SOCKET client_sock) {
    char req[1024];
    int bytes = recv(client_sock, req, sizeof(req) - 1, 0);
    if (bytes <= 0) {
        close_socket(client_sock);
        return;
    }
    req[bytes] = '\0';

    if (strncmp(req, "GET /api/metrics", 16) == 0) {
        HardwareMetrics m{};
        lock_metrics();
        m = s_latest_metrics;
        unlock_metrics();

        char json[1024];
        int json_len = snprintf(json, sizeof(json),
            "{\n"
            "  \"cpu_temp\": %d,\n"
            "  \"soc_temp\": %d,\n"
            "  \"cpu_usage\": %.1f,\n"
            "  \"cpu_cores\": [%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f],\n"
            "  \"ram_used_mb\": %d,\n"
            "  \"ram_total_mb\": %d,\n"
            "  \"ram_pct\": %.1f,\n"
            "  \"vram_used_mb\": %d,\n"
            "  \"vram_total_mb\": %d,\n"
            "  \"vram_pct\": %.1f,\n"
            "  \"fan_duty_pct\": %d\n"
            "}",
            m.cpu_temp,
            m.soc_temp,
            m.cpu_usage,
            m.cpu_core_usage[0], m.cpu_core_usage[1], m.cpu_core_usage[2], m.cpu_core_usage[3],
            m.cpu_core_usage[4], m.cpu_core_usage[5], m.cpu_core_usage[6], m.cpu_core_usage[7],
            m.ram_used_mb,
            m.ram_total_mb,
            m.ram_percentage,
            m.vram_used_mb,
            m.vram_total_mb,
            m.vram_percentage,
            m.fan_duty_percent
        );

        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            json_len
        );

        send(client_sock, header, header_len, 0);
        send(client_sock, json, json_len, 0);
    } else if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /index.html", 15) == 0) {
        int html_len = (int)strlen(HTML_PAGE);
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n",
            html_len
        );

        send(client_sock, header, header_len, 0);
        send(client_sock, HTML_PAGE, html_len, 0);
    } else if (strncmp(req, "GET /overlay", 12) == 0 || strncmp(req, "GET /bar", 8) == 0) {
        int html_len = (int)strlen(HTML_OVERLAY_PAGE);
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n",
            html_len
        );

        send(client_sock, header, header_len, 0);
        send(client_sock, HTML_OVERLAY_PAGE, html_len, 0);
    } else if (strncmp(req, "GET /log", 8) == 0) {
        char buf[8192] = "No log file found at /system_tmp/ps5_overlay.log\n";
        FILE* f = fopen("/system_tmp/ps5_overlay.log", "r");
        if (f) {
            size_t bytes = fread(buf, 1, sizeof(buf) - 1, f);
            buf[bytes] = '\0';
            fclose(f);
        }
        int log_len = (int)strlen(buf);
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n",
            log_len
        );
        send(client_sock, header, header_len, 0);
        send(client_sock, buf, log_len, 0);
    } else {
        const char not_found[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "Connection: close\r\n\r\n"
            "Not Found";
        send(client_sock, not_found, (int)strlen(not_found), 0);
    }

    close_socket(client_sock);
}

#ifdef _WIN32
static unsigned __stdcall server_thread_func(void* arg) {
    int port = (int)(intptr_t)arg;
#else
static void* server_thread_func(void* arg) {
    int port = (int)(intptr_t)arg;
#endif

    s_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "[WEB] Failed to create socket\n");
#ifdef _WIN32
        return 0;
#else
        return nullptr;
#endif
    }

    int opt = 1;
    setsockopt(s_listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(s_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[WEB] Failed to bind to port %d\n", port);
        close_socket(s_listen_sock);
        s_listen_sock = INVALID_SOCKET;
#ifdef _WIN32
        return 0;
#else
        return nullptr;
#endif
    }

    if (listen(s_listen_sock, 10) == SOCKET_ERROR) {
        fprintf(stderr, "[WEB] Failed to listen on socket\n");
        close_socket(s_listen_sock);
        s_listen_sock = INVALID_SOCKET;
#ifdef _WIN32
        return 0;
#else
        return nullptr;
#endif
    }

    printf("[WEB] Real-time Web HUD active at http://0.0.0.0:%d/\n", port);

    while (s_server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(s_listen_sock, &readfds);

        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 300000; /* 300 ms */

        int sel = select((int)s_listen_sock + 1, &readfds, nullptr, nullptr, &tv);
        if (sel > 0 && FD_ISSET(s_listen_sock, &readfds)) {
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            SOCKET client = accept(s_listen_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client != INVALID_SOCKET) {
                handle_client(client);
            }
        }
    }

    if (s_listen_sock != INVALID_SOCKET) {
        close_socket(s_listen_sock);
        s_listen_sock = INVALID_SOCKET;
    }

#ifdef _WIN32
    return 0;
#else
    return nullptr;
#endif
}

bool web_server_start(int port) {
    if (s_server_running) return true;

#ifdef _WIN32
    InitializeCriticalSection(&s_metrics_cs);
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    s_server_running = true;

#ifdef _WIN32
    s_server_thread = (HANDLE)_beginthreadex(nullptr, 0, server_thread_func, (void*)(intptr_t)port, 0, nullptr);
    return (s_server_thread != NULL);
#else
    int ret = pthread_create(&s_server_thread, nullptr, server_thread_func, (void*)(intptr_t)port);
    return (ret == 0);
#endif
}

void web_server_stop(void) {
    if (!s_server_running) return;
    s_server_running = false;

    if (s_listen_sock != INVALID_SOCKET) {
        close_socket(s_listen_sock);
        s_listen_sock = INVALID_SOCKET;
    }

#ifdef _WIN32
    if (s_server_thread) {
        WaitForSingleObject(s_server_thread, 2000);
        CloseHandle(s_server_thread);
        s_server_thread = NULL;
    }
    WSACleanup();
    DeleteCriticalSection(&s_metrics_cs);
#else
    pthread_join(s_server_thread, nullptr);
#endif
}
