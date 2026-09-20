#pragma once

static const char HTML_THEMES_STUDIO[] = R"rawliteral(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hardware Overlay - 10 Design Variations</title>
  <!-- Tailwind CSS CDN -->
  <script src="https://cdn.tailwindcss.com"></script>
  <!-- Google Fonts: Inter, Rajdhani (Cyberpunk/Sci-Fi), JetBrains Mono (Terminal), Space Grotesk -->
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&family=JetBrains+Mono:wght@400;500;700&family=Orbitron:wght@500;700;900&family=Rajdhani:wght@500;600;700&family=Space+Grotesk:wght@500;700&display=swap" rel="stylesheet">
  
  <script>
    tailwind.config = {
      darkMode: 'class',
      theme: {
        extend: {
          fontFamily: {
            sans: ['Inter', 'sans-serif'],
            mono: ['"JetBrains Mono"', 'monospace'],
            cyber: ['Rajdhani', 'sans-serif'],
            orbitron: ['Orbitron', 'sans-serif'],
            space: ['"Space Grotesk"', 'sans-serif']
          },
          colors: {
            deck: {
              bg: '#0c131d',
              card: '#121e2d',
              border: '#1d2f44',
              accent: '#00a3ff',
              cyan: '#38bdf8'
            },
            cyber: {
              neonYellow: '#fcee0a',
              neonCyan: '#00f0ff',
              neonPink: '#ff0055',
              darkBg: '#090a10',
              panel: '#101424'
            },
            rog: {
              red: '#ff1744',
              crimson: '#99001b',
              dark: '#0e0e11',
              panel: '#18191f'
            }
          }
        }
      }
    }
  </script>
  <style>
    /* Custom cyber cutouts & HUD styling */
    .cyber-border {
      clip-path: polygon(0 0, calc(100% - 14px) 0, 100% 14px, 100% 100%, 14px 100%, 0 calc(100% - 14px));
    }
    .cyber-chamfer {
      clip-path: polygon(12px 0, 100% 0, 100% calc(100% - 12px), calc(100% - 12px) 100%, 0 100%, 0 12px);
    }
    .crt-glow {
      text-shadow: 0 0 8px rgba(34, 197, 94, 0.7);
    }
    .neon-text-cyan {
      text-shadow: 0 0 12px rgba(56, 189, 248, 0.75);
    }
    .neon-text-yellow {
      text-shadow: 0 0 12px rgba(252, 238, 10, 0.7);
    }
    .neon-text-red {
      text-shadow: 0 0 12px rgba(255, 23, 68, 0.8);
    }
    .glass-frosted {
      background: rgba(14, 23, 38, 0.68);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid rgba(255, 255, 255, 0.08);
    }
    .glass-bright {
      background: rgba(255, 255, 255, 0.05);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border: 1px solid rgba(255, 255, 255, 0.12);
    }
    /* Custom Scrollbar for demo list */
    ::-webkit-scrollbar {
      width: 6px;
      height: 6px;
    }
    ::-webkit-scrollbar-track {
      background: #0b0f19;
    }
    ::-webkit-scrollbar-thumb {
      background: #1e293b;
      border-radius: 9999px;
    }
    ::-webkit-scrollbar-thumb:hover {
      background: #334155;
    }
  </style>
</head>
<body class="bg-[#080b11] text-slate-100 min-h-screen font-sans antialiased overflow-x-hidden select-none">
  <header class="border-b border-slate-800 bg-[#0c101a]/95 backdrop-blur-md sticky top-0 z-50">
    <div class="max-w-7xl mx-auto px-4 py-3 flex flex-wrap items-center justify-between gap-4">
      <div class="flex items-center gap-3">
        <div class="w-9 h-9 rounded-xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center shadow-lg shadow-cyan-500/20">
          <svg class="w-5 h-5 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
          </svg>
        </div>
        <div>
          <h1 class="font-bold text-base tracking-wide flex items-center gap-2 text-white">
            Overlay Studio <span class="text-xs px-2 py-0.5 rounded-full bg-cyan-500/20 text-cyan-400 border border-cyan-500/30">10 Вариантов</span>
          </h1>
          <p class="text-xs text-slate-400">Дизайн телеметрии для игр, стримов и портативных консолей</p>
        </div>
      </div>

      <!-- Live Controls -->
      <div class="flex flex-wrap items-center gap-2 sm:gap-3 text-xs">
        <!-- Background selector -->
        <div class="flex items-center bg-slate-900 border border-slate-700/70 rounded-lg p-0.5">
          <button id="bgBtnGame" onclick="setBg('game')" class="px-2.5 py-1 rounded-md transition font-medium bg-cyan-500 text-white">
            🎮 Игра
          </button>
          <button id="bgBtnDark" onclick="setBg('dark')" class="px-2.5 py-1 rounded-md transition font-medium text-slate-400 hover:text-white">
            🌑 Темный
          </button>
          <button id="bgBtnObs" onclick="setBg('obs')" class="px-2.5 py-1 rounded-md transition font-medium text-slate-400 hover:text-white" title="Сетка прозрачности для стримеров (OBS)">
            🏁 OBS
          </button>
        </div>

        <!-- Simulation Load Toggle -->
        <button id="simBtn" onclick="toggleSim()" class="flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-200 transition">
          <span id="simDot" class="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
          <span id="simText">Режим: Простой (Idle)</span>
        </button>

        <!-- View Mode (Single vs All) -->
        <button id="viewAllBtn" onclick="toggleViewAll()" class="px-3 py-1.5 rounded-lg border border-cyan-500/40 bg-cyan-950/30 text-cyan-300 hover:bg-cyan-900/40 transition font-medium">
          Показать все 10 разом
        </button>
      </div>
    </div>

    
    <!-- TV Overlay Control Bar -->
    <div class="max-w-7xl mx-auto px-4 py-2.5 bg-gradient-to-r from-blue-950/80 via-slate-900/90 to-cyan-950/80 border-t border-cyan-500/20 flex flex-wrap items-center justify-between gap-3 text-xs">
      <div class="flex items-center gap-2">
        <span class="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse"></span>
        <span class="font-bold uppercase tracking-wider text-cyan-200">📺 PlayStation 5 In-Game HUD:</span>
        <span id="tvStatusTag" class="px-2 py-0.5 rounded-md font-mono bg-cyan-900/60 text-cyan-300 border border-cyan-700/50">Загрузка...</span>
      </div>

      <div class="flex flex-wrap items-center gap-2">
        <label class="text-slate-300 font-medium flex items-center gap-1">
          <span>Позиция:</span>
          <select id="tvPosSelect" onchange="onPosChanged()" class="bg-slate-900 text-cyan-300 border border-slate-700 rounded px-2 py-1 focus:outline-none focus:border-cyan-400 font-medium">
            <option value="top">Вверху (Top)</option>
            <option value="top-right">Вверху справа (Top-Right)</option>
            <option value="bottom">Внизу (Bottom)</option>
            <option value="bottom-right">Внизу справа (Bottom-Right)</option>
          </select>
        </label>

        <button onclick="applyCurrentVariantToTv()" class="px-3 py-1.5 rounded-lg bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-white font-bold transition shadow-lg shadow-cyan-500/25 flex items-center gap-1.5">
          <span>🚀 Применить на ТВ</span>
        </button>

        <button onclick="turnOffTvOverlay()" class="px-2.5 py-1.5 rounded-lg bg-red-950/70 hover:bg-red-900/80 text-red-300 border border-red-800/50 font-medium transition">
          ❌ Выключить на ТВ
        </button>
      </div>
    </div>

    <!-- Variant selector pills -->
    <div class="max-w-7xl mx-auto px-4 py-2 border-t border-slate-800/80 overflow-x-auto flex gap-1.5 no-scrollbar">
      <button onclick="selectVariant(1)" class="var-tab-btn active px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap bg-cyan-500 text-white" data-var="1">1. Deck SteamOS Refined</button>
      <button onclick="selectVariant(2)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="2">2. Cyber HUD 2077</button>
      <button onclick="selectVariant(3)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="3">3. Esports Top Bar (Minimal)</button>
      <button onclick="selectVariant(4)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="4">4. Streamer Vertical Dock</button>
      <button onclick="selectVariant(5)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="5">5. Modern Bento Glass</button>
      <button onclick="selectVariant(6)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="6">6. Retro Hacker TUI / CLI</button>
      <button onclick="selectVariant(7)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="7">7. Radial Gauges (Sim Racing)</button>
      <button onclick="selectVariant(8)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="8">8. Floating Micro-Pills</button>
      <button onclick="selectVariant(9)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="9">9. Tactical Stealth (ROG/Razer)</button>
      <button onclick="selectVariant(10)" class="var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800" data-var="10">10. Neon Prism Gradient</button>
    </div>
  </header>
  <main class="max-w-7xl mx-auto p-4 sm:p-6">
    <!-- Preview Area with dynamic background simulation -->
    <div id="previewCanvas" class="relative rounded-2xl overflow-hidden border border-slate-800 min-h-[640px] flex items-center justify-center p-4 sm:p-8 transition-all duration-300"
         style="background: linear-gradient(rgba(10, 14, 23, 0.78), rgba(6, 9, 15, 0.88)), url('https://images.unsplash.com/photo-1542751371-adc38448a05e?q=80&w=1600&auto=format&fit=crop') center/cover no-repeat;">
      
      <!-- Watermark label in canvas -->
      <div class="absolute top-4 right-4 text-[11px] font-mono uppercase tracking-widest text-white/30 pointer-events-none flex items-center gap-2">
        <span class="w-1.5 h-1.5 rounded-full bg-cyan-400"></span>
        Overlay Preview Canvas
      </div>

      <!-- CONTAINER FOR SINGLE VARIANT VIEW -->
      <div id="singleViewWrapper" class="w-full flex justify-center items-center py-6">
        <!-- ========================================================
             VARIANT 1: DECK STEAMOS REFINED (Direct evolution of user's original)
             ======================================================== -->
        <div id="variant-1" class="overlay-card w-full max-w-4xl space-y-4">
          <div class="text-xs uppercase tracking-wider text-cyan-400 font-bold mb-2 flex items-center justify-between">
            <span>Вариант 1: Deck OS Refined (Улучшенный оригинал) <button onclick="applyThemeToTv(\'deck\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></span>
            <span class="text-slate-400 text-[11px] font-normal lowercase">мягкое свечение, ясная иерархия и чистые карточки</span>
          </div>

          <!-- Top row 3 cards -->
          <div class="grid grid-cols-1 md:grid-cols-3 gap-3.5">
            <!-- CPU Card -->
            <div class="bg-[#0f1725]/90 border border-[#1e2f47] rounded-xl p-4 shadow-xl backdrop-blur-md relative overflow-hidden">
              <div class="flex justify-between items-start mb-1">
                <div class="text-xs font-bold text-slate-300 tracking-wider">CPU (ZEN 2)</div>
                <span class="text-sm font-extrabold text-cyan-400 cpu-temp-text">54 °C</span>
              </div>
              <div class="text-3xl font-extrabold text-cyan-400 tracking-tight mb-2 cpu-temp-main">54 °C</div>
              
              <div class="flex justify-between items-center text-xs text-slate-300 font-medium mb-1.5">
                <span>Load: <strong class="text-white cpu-load-text">0%</strong></span>
                <span class="text-[10px] text-slate-500 font-mono">8 Cores</span>
              </div>
              
              <!-- Main CPU load bar -->
              <div class="w-full bg-[#162338] h-1.5 rounded-full overflow-hidden mb-3">
                <div class="bg-gradient-to-r from-cyan-400 to-blue-500 h-full rounded-full transition-all duration-300 cpu-load-bar" style="width: 2%;"></div>
              </div>

              <!-- 8 CPU Cores Grid -->
              <div class="grid grid-cols-8 gap-1 pt-1 border-t border-slate-800/80">
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 10%"></div></div><span class="text-[9px] text-slate-400 font-mono">C0</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 5%"></div></div><span class="text-[9px] text-slate-400 font-mono">C1</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 0%"></div></div><span class="text-[9px] text-slate-400 font-mono">C2</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 8%"></div></div><span class="text-[9px] text-slate-400 font-mono">C3</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 0%"></div></div><span class="text-[9px] text-slate-400 font-mono">C4</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 0%"></div></div><span class="text-[9px] text-slate-400 font-mono">C5</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 2%"></div></div><span class="text-[9px] text-slate-400 font-mono">C6</span></div>
                <div class="text-center"><div class="h-6 rounded bg-[#18263a] core-box flex items-end p-0.5"><div class="w-full bg-cyan-400/80 rounded-sm core-fill" style="height: 0%"></div></div><span class="text-[9px] text-slate-400 font-mono">C7</span></div>
              </div>
            </div>

            <!-- GPU Card -->
            <div class="bg-[#0f1725]/90 border border-[#1e2f47] rounded-xl p-4 shadow-xl backdrop-blur-md">
              <div class="flex justify-between items-start mb-1">
                <div class="text-xs font-bold text-slate-300 tracking-wider">GPU (RDNA 2)</div>
                <span class="text-sm font-extrabold text-cyan-400 gpu-temp-text">47 °C</span>
              </div>
              <div class="text-3xl font-extrabold text-cyan-400 tracking-tight mb-2 gpu-temp-main">47 °C</div>
              <div class="text-xs text-slate-300 font-medium mb-2 flex items-center justify-between">
                <span>VRAM: <strong class="text-white vram-text">0.3 / 16.0 GB</strong></span>
                <span class="text-cyan-400 font-bold vram-pct">2%</span>
              </div>
              <div class="w-full bg-[#162338] h-1.5 rounded-full overflow-hidden mt-3">
                <div class="bg-gradient-to-r from-cyan-400 to-blue-500 h-full rounded-full transition-all duration-300 vram-bar" style="width: 2%;"></div>
              </div>
              <div class="mt-4 flex justify-between items-center text-[10px] text-slate-400 border-t border-slate-800/80 pt-2">
                <span>Core Clock: ~350 MHz</span>
                <span class="px-1.5 py-0.5 bg-emerald-500/20 text-emerald-400 rounded">Normal</span>
              </div>
            </div>

            <!-- RAM Card -->
            <div class="bg-[#0f1725]/90 border border-[#1e2f47] rounded-xl p-4 shadow-xl backdrop-blur-md">
              <div class="flex justify-between items-start mb-1">
                <div class="text-xs font-bold text-slate-300 tracking-wider">SYSTEM RAM</div>
                <span class="text-sm font-extrabold text-cyan-400 ram-pct">3%</span>
              </div>
              <div class="text-3xl font-extrabold text-white tracking-tight mb-2 ram-used">0.6 GB</div>
              <div class="text-xs text-slate-300 font-medium mb-2">
                Total: <strong class="text-slate-200">16.0 GB</strong> <span class="text-slate-400">(3%)</span>
              </div>
              <div class="w-full bg-[#162338] h-1.5 rounded-full overflow-hidden mt-3">
                <div class="bg-gradient-to-r from-cyan-400 to-blue-500 h-full rounded-full transition-all duration-300 ram-bar" style="width: 3%;"></div>
              </div>
              <div class="mt-4 flex justify-between items-center text-[10px] text-slate-400 border-t border-slate-800/80 pt-2">
                <span>Memory Pool: LPDDR5</span>
                <span class="text-slate-300 font-mono">15.4 GB Free</span>
              </div>
            </div>
          </div>

          <!-- Bottom row: Fan Card -->
          <div class="grid grid-cols-1 md:grid-cols-3 gap-3.5">
            <div class="bg-[#0f1725]/90 border border-[#1e2f47] rounded-xl p-4 shadow-xl backdrop-blur-md">
              <div class="flex justify-between items-start mb-1">
                <div class="text-xs font-bold text-slate-300 tracking-wider">COOLING FAN</div>
                <span class="text-sm font-extrabold text-cyan-400 fan-pct">23%</span>
              </div>
              <div class="text-3xl font-extrabold text-white tracking-tight mb-2 fan-pct-main">23%</div>
              <div class="text-xs text-slate-300 font-medium mb-2 flex items-center gap-1.5">
                <span>Duty Cycle:</span>
                <span class="px-2 py-0.5 rounded-md bg-cyan-950 text-cyan-300 border border-cyan-800/50 font-bold text-[11px] fan-duty">Quiet</span>
              </div>
              <div class="w-full bg-[#162338] h-1.5 rounded-full overflow-hidden mt-3">
                <div class="bg-gradient-to-r from-cyan-400 to-blue-500 h-full rounded-full transition-all duration-300 fan-bar" style="width: 23%;"></div>
              </div>
            </div>
            
            <div class="hidden md:flex col-span-2 bg-[#0f1725]/40 border border-dashed border-[#1e2f47] rounded-xl p-4 items-center justify-between text-xs text-slate-400">
              <div class="flex items-center gap-3">
                <div class="w-2 h-2 rounded-full bg-emerald-400 animate-ping"></div>
                <span>Адаптивный термопрофиль активен (Zen 2 + RDNA 2 APU)</span>
              </div>
              <div class="text-[11px] font-mono text-slate-500">Overlay V1 • Default Style</div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 2: CYBER HUD 2077 (Angular, Sci-Fi, Neon Cyberpunk)
             ======================================================== -->
        <div id="variant-2" class="overlay-card hidden w-full max-w-4xl space-y-4 font-cyber">
          <div class="text-xs tracking-widest text-cyber-neonYellow uppercase font-bold flex justify-between items-center">
            <span>[SYS_TELEMETRY // RDNA2_APU_V2.077]</span>
            <span class="text-[11px] text-cyber-neonCyan">PROTOCOL: ACTIVE</span>
          </div>

          <div class="grid grid-cols-1 md:grid-cols-4 gap-3">
            <!-- CPU MODULE -->
            <div class="cyber-border bg-[#0a0d18]/95 border-l-4 border-l-cyber-neonCyan border-y border-r border-cyan-900/60 p-3.5 relative">
              <div class="flex justify-between items-center text-xs text-cyan-300 uppercase tracking-wider font-bold">
                <span>CPU // ZEN2</span>
                <span class="cpu-temp-text text-cyber-neonCyan font-bold">54°C</span>
              </div>
              <div class="text-4xl font-black text-white my-1 font-orbitron tracking-tight cpu-temp-main">54<span class="text-xl text-cyber-neonCyan">°C</span></div>
              <div class="flex justify-between text-[11px] text-slate-400 font-mono mb-1">
                <span>LOAD</span>
                <span class="text-cyber-neonCyan font-bold cpu-load-text">0%</span>
              </div>
              <div class="h-2 bg-slate-900 border border-cyan-800/60 p-0.5 mb-3">
                <div class="h-full bg-cyber-neonCyan transition-all duration-300 cpu-load-bar" style="width: 2%;"></div>
              </div>
              <!-- Cores bars -->
              <div class="grid grid-cols-4 gap-1 text-[9px] font-mono text-center">
                <div class="bg-cyan-950/60 border border-cyan-800/40 p-1"><span class="text-slate-400">0:</span><span class="text-cyber-neonCyan ml-0.5">2%</span></div>
                <div class="bg-cyan-950/60 border border-cyan-800/40 p-1"><span class="text-slate-400">1:</span><span class="text-cyber-neonCyan ml-0.5">0%</span></div>
                <div class="bg-cyan-950/60 border border-cyan-800/40 p-1"><span class="text-slate-400">2:</span><span class="text-cyber-neonCyan ml-0.5">0%</span></div>
                <div class="bg-cyan-950/60 border border-cyan-800/40 p-1"><span class="text-slate-400">3:</span><span class="text-cyber-neonCyan ml-0.5">5%</span></div>
              </div>
            </div>

            <!-- GPU MODULE -->
            <div class="cyber-border bg-[#0a0d18]/95 border-l-4 border-l-cyber-neonYellow border-y border-r border-yellow-900/60 p-3.5 relative">
              <div class="flex justify-between items-center text-xs text-yellow-300 uppercase tracking-wider font-bold">
                <span>GPU // RDNA2</span>
                <span class="gpu-temp-text text-cyber-neonYellow font-bold">47°C</span>
              </div>
              <div class="text-4xl font-black text-white my-1 font-orbitron tracking-tight gpu-temp-main">47<span class="text-xl text-cyber-neonYellow">°C</span></div>
              <div class="flex justify-between text-[11px] text-slate-400 font-mono mb-1">
                <span>VRAM USE</span>
                <span class="text-cyber-neonYellow font-bold vram-pct">2%</span>
              </div>
              <div class="h-2 bg-slate-900 border border-yellow-800/60 p-0.5 mb-2">
                <div class="h-full bg-cyber-neonYellow transition-all duration-300 vram-bar" style="width: 2%;"></div>
              </div>
              <div class="text-[11px] text-slate-400 font-mono flex justify-between">
                <span>ALLOC</span>
                <span class="text-slate-200 vram-text">0.3 / 16 GB</span>
              </div>
            </div>

            <!-- RAM MODULE -->
            <div class="cyber-border bg-[#0a0d18]/95 border-l-4 border-l-purple-500 border-y border-r border-purple-900/60 p-3.5 relative">
              <div class="flex justify-between items-center text-xs text-purple-300 uppercase tracking-wider font-bold">
                <span>SYSTEM RAM</span>
                <span class="ram-pct text-purple-400 font-bold">3%</span>
              </div>
              <div class="text-4xl font-black text-white my-1 font-orbitron tracking-tight ram-used">0.6<span class="text-xl text-purple-400">GB</span></div>
              <div class="flex justify-between text-[11px] text-slate-400 font-mono mb-1">
                <span>CAPACITY</span>
                <span class="text-slate-200">16.0 GB</span>
              </div>
              <div class="h-2 bg-slate-900 border border-purple-800/60 p-0.5 mb-2">
                <div class="h-full bg-purple-500 transition-all duration-300 ram-bar" style="width: 3%;"></div>
              </div>
              <div class="text-[10px] text-purple-400 font-mono uppercase tracking-widest text-center">QUAD-CHANNEL 5500MT</div>
            </div>

            <!-- FAN MODULE -->
            <div class="cyber-border bg-[#0a0d18]/95 border-l-4 border-l-emerald-400 border-y border-r border-emerald-900/60 p-3.5 relative">
              <div class="flex justify-between items-center text-xs text-emerald-300 uppercase tracking-wider font-bold">
                <span>FAN // TACHO</span>
                <span class="fan-pct text-emerald-400 font-bold">23%</span>
              </div>
              <div class="text-4xl font-black text-white my-1 font-orbitron tracking-tight fan-pct-main">23<span class="text-xl text-emerald-400">%</span></div>
              <div class="flex justify-between text-[11px] text-slate-400 font-mono mb-1">
                <span>MODE</span>
                <span class="text-emerald-400 font-bold fan-duty uppercase">QUIET</span>
              </div>
              <div class="h-2 bg-slate-900 border border-emerald-800/60 p-0.5 mb-2">
                <div class="h-full bg-emerald-400 transition-all duration-300 fan-bar" style="width: 23%;"></div>
              </div>
              <div class="text-[10px] text-emerald-400/80 font-mono text-center flex items-center justify-center gap-1">
                <span class="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-ping"></span> RPM: ~1850
              </div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 3: ESPORTS MINIMALIST TOP BAR (Ultra-compact MangoHud/RTSS style)
             ======================================================== -->
        <div id="variant-3" class="overlay-card hidden w-full max-w-5xl">
          <div class="text-xs uppercase tracking-wider text-emerald-400 font-bold mb-2">
            Вариант 3: Esports Top Ribbon (Минималистичная полоса)
           <button onclick="applyThemeToTv(\'esports\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>
          
          <div class="bg-black/85 backdrop-blur-md border-y border-slate-700/60 px-4 py-2.5 rounded-lg flex flex-wrap items-center justify-between gap-4 font-mono text-xs shadow-2xl">
            <!-- CPU Group -->
            <div class="flex items-center gap-3 border-r border-slate-800 pr-4">
              <span class="text-slate-400 font-bold">CPU</span>
              <span class="text-cyan-400 font-bold text-sm cpu-temp-text">54°C</span>
              <span class="text-slate-300"><span class="cpu-load-text font-semibold text-white">0%</span></span>
              <div class="w-16 h-2 bg-slate-800 rounded-full overflow-hidden">
                <div class="bg-cyan-400 h-full rounded-full cpu-load-bar" style="width: 2%;"></div>
              </div>
            </div>

            <!-- GPU Group -->
            <div class="flex items-center gap-3 border-r border-slate-800 pr-4">
              <span class="text-slate-400 font-bold">GPU</span>
              <span class="text-cyan-400 font-bold text-sm gpu-temp-text">47°C</span>
              <span class="text-slate-400">VRAM:</span>
              <span class="text-white vram-text">0.3G</span>
              <span class="text-cyan-400 vram-pct font-bold">2%</span>
            </div>

            <!-- RAM Group -->
            <div class="flex items-center gap-3 border-r border-slate-800 pr-4">
              <span class="text-slate-400 font-bold">RAM</span>
              <span class="text-white ram-used font-bold">0.6 GB</span>
              <span class="text-slate-500">/ 16G</span>
              <span class="text-cyan-400 ram-pct font-bold">3%</span>
            </div>

            <!-- FAN Group -->
            <div class="flex items-center gap-3">
              <span class="text-slate-400 font-bold">FAN</span>
              <span class="text-white fan-pct font-bold">23%</span>
              <span class="px-1.5 py-0.2 bg-slate-800 text-emerald-400 text-[10px] rounded fan-duty uppercase">Quiet</span>
            </div>
          </div>
          <p class="text-[11px] text-slate-400 mt-2">Занимает минимум экранного пространства в соревновательных играх (CS2, Apex, Dota 2).</p>
        </div>
        <!-- ========================================================
             VARIANT 4: STREAMER VERTICAL DOCK (Vertical layout for OBS)
             ======================================================== -->
        <div id="variant-4" class="overlay-card hidden w-full max-w-sm">
          <div class="text-xs uppercase tracking-wider text-purple-400 font-bold mb-2">
            Вариант 4: Streamer Side Dock (Вертикальный док для трансляций)
           <button onclick="applyThemeToTv(\'dock\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="bg-gradient-to-b from-[#0e1422]/95 to-[#090d16]/95 border border-purple-500/30 rounded-2xl p-4 shadow-2xl backdrop-blur-xl relative overflow-hidden">
            <div class="absolute top-0 right-0 w-24 h-24 bg-purple-600/10 rounded-full blur-2xl pointer-events-none"></div>

            <div class="flex items-center justify-between pb-3 mb-3 border-b border-slate-800">
              <div class="flex items-center gap-2">
                <span class="w-2.5 h-2.5 rounded-full bg-red-500 animate-pulse"></span>
                <span class="text-xs font-bold uppercase tracking-wider text-white">OBS Telemetry</span>
              </div>
              <span class="text-[10px] font-mono px-2 py-0.5 rounded bg-purple-950 text-purple-300 border border-purple-800/40">APU HW</span>
            </div>

            <!-- CPU Vertical item -->
            <div class="mb-4">
              <div class="flex justify-between items-center text-xs mb-1">
                <span class="text-slate-300 font-semibold flex items-center gap-1.5">
                  <span class="w-1.5 h-1.5 rounded-full bg-cyan-400"></span> CPU (Zen 2)
                </span>
                <div class="space-x-1.5">
                  <span class="text-cyan-400 font-bold cpu-temp-text">54°C</span>
                  <span class="text-slate-400 cpu-load-text font-mono">0%</span>
                </div>
              </div>
              <div class="w-full bg-slate-800/80 h-2 rounded-full overflow-hidden">
                <div class="bg-gradient-to-r from-cyan-500 to-blue-500 h-full rounded-full cpu-load-bar" style="width: 2%;"></div>
              </div>
              <!-- Mini core heat spots -->
              <div class="flex gap-1 mt-1.5">
                <span class="h-1 flex-1 bg-cyan-400/40 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/20 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/10 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/30 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/10 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/10 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/20 rounded"></span>
                <span class="h-1 flex-1 bg-cyan-400/10 rounded"></span>
              </div>
            </div>

            <!-- GPU Vertical item -->
            <div class="mb-4">
              <div class="flex justify-between items-center text-xs mb-1">
                <span class="text-slate-300 font-semibold flex items-center gap-1.5">
                  <span class="w-1.5 h-1.5 rounded-full bg-purple-400"></span> GPU (RDNA 2)
                </span>
                <div class="space-x-1.5">
                  <span class="text-purple-400 font-bold gpu-temp-text">47°C</span>
                  <span class="text-slate-400 vram-pct font-mono">2%</span>
                </div>
              </div>
              <div class="w-full bg-slate-800/80 h-2 rounded-full overflow-hidden">
                <div class="bg-gradient-to-r from-purple-500 to-pink-500 h-full rounded-full vram-bar" style="width: 2%;"></div>
              </div>
              <div class="flex justify-between text-[10px] text-slate-400 font-mono mt-1">
                <span>VRAM Usage</span>
                <span class="vram-text text-slate-300">0.3 / 16.0 GB</span>
              </div>
            </div>

            <!-- RAM Vertical item -->
            <div class="mb-4">
              <div class="flex justify-between items-center text-xs mb-1">
                <span class="text-slate-300 font-semibold flex items-center gap-1.5">
                  <span class="w-1.5 h-1.5 rounded-full bg-blue-400"></span> Memory
                </span>
                <div class="space-x-1.5">
                  <span class="text-white font-bold ram-used">0.6 GB</span>
                  <span class="text-cyan-400 ram-pct font-mono">3%</span>
                </div>
              </div>
              <div class="w-full bg-slate-800/80 h-2 rounded-full overflow-hidden">
                <div class="bg-gradient-to-r from-blue-500 to-indigo-500 h-full rounded-full ram-bar" style="width: 3%;"></div>
              </div>
            </div>

            <!-- Fan item -->
            <div class="pt-2 border-t border-slate-800/70 flex items-center justify-between text-xs">
              <div class="flex items-center gap-2">
                <svg class="w-4 h-4 text-emerald-400 animate-spin" style="animation-duration: 4s;" viewBox="0 0 24 24" fill="none" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 4v1m0 14v1m8-8h-1M5 12H4m15.364-6.364l-.707.707M6.343 17.657l-.707.707m12.728 0l-.707-.707M6.343 6.343l-.707-.707" />
                </svg>
                <span class="text-slate-300 font-medium">Fan Speed</span>
              </div>
              <div class="flex items-center gap-2">
                <span class="fan-duty text-[10px] uppercase font-bold text-emerald-400 bg-emerald-950/60 px-1.5 py-0.5 rounded">Quiet</span>
                <span class="fan-pct font-bold text-white font-mono">23%</span>
              </div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 5: MODERN BENTO GLASS (Apple / Cupertino Dashboard)
             ======================================================== -->
        <div id="variant-5" class="overlay-card hidden w-full max-w-4xl space-y-3">
          <div class="text-xs uppercase tracking-wider text-sky-400 font-bold mb-1">
            Вариант 5: Bento Minimalist Glass (Стильный bento-грид с мягким стеклом)
           <button onclick="applyThemeToTv(\'bento\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="grid grid-cols-1 md:grid-cols-4 gap-3">
            <!-- CPU Large Card -->
            <div class="md:col-span-2 glass-frosted rounded-3xl p-5 border border-white/10 shadow-2xl relative">
              <div class="flex justify-between items-start">
                <div>
                  <p class="text-xs font-semibold text-slate-400 tracking-wide uppercase">Processor</p>
                  <h3 class="text-lg font-bold text-white">AMD Zen 2</h3>
                </div>
                <div class="px-2.5 py-1 rounded-full bg-sky-500/10 border border-sky-500/30 text-sky-400 text-xs font-bold cpu-temp-text">
                  54 °C
                </div>
              </div>

              <div class="my-4 flex items-baseline gap-3">
                <span class="text-4xl font-extrabold text-white tracking-tight cpu-load-text">0%</span>
                <span class="text-xs text-slate-400">Current Workload</span>
              </div>

              <!-- CPU core distribution -->
              <div class="space-y-1.5">
                <div class="flex justify-between text-[11px] text-slate-400">
                  <span>8 Cores Active</span>
                  <span>Threads 0-7</span>
                </div>
                <div class="grid grid-cols-8 gap-1.5">
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 15%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 0%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 20%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 5%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 0%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 0%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 10%"></div></div>
                  <div class="h-2 rounded-full bg-slate-700/60 overflow-hidden"><div class="bg-sky-400 h-full core-fill" style="width: 0%"></div></div>
                </div>
              </div>
            </div>

            <!-- GPU Card -->
            <div class="glass-frosted rounded-3xl p-5 border border-white/10 shadow-2xl flex flex-col justify-between">
              <div>
                <div class="flex justify-between items-start">
                  <p class="text-xs font-semibold text-slate-400 uppercase">Graphics</p>
                  <span class="text-xs font-bold text-indigo-400 gpu-temp-text">47 °C</span>
                </div>
                <h3 class="text-lg font-bold text-white">RDNA 2</h3>
              </div>
              <div class="my-3">
                <div class="text-2xl font-bold text-white vram-pct">2%</div>
                <p class="text-xs text-slate-400 vram-text">0.3 / 16.0 GB VRAM</p>
              </div>
              <div class="w-full bg-white/10 h-2 rounded-full overflow-hidden">
                <div class="bg-indigo-400 h-full rounded-full vram-bar" style="width: 2%;"></div>
              </div>
            </div>

            <!-- RAM & Fan Bento Stack -->
            <div class="space-y-3 flex flex-col">
              <!-- RAM Pill -->
              <div class="glass-frosted rounded-2xl p-4 border border-white/10 flex-1 flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs">
                  <span class="text-slate-400 font-semibold">Memory</span>
                  <span class="text-sky-400 font-bold ram-pct">3%</span>
                </div>
                <div class="text-xl font-bold text-white ram-used">0.6 GB</div>
                <div class="w-full bg-white/10 h-1.5 rounded-full overflow-hidden">
                  <div class="bg-sky-400 h-full rounded-full ram-bar" style="width: 3%;"></div>
                </div>
              </div>

              <!-- Fan Pill -->
              <div class="glass-frosted rounded-2xl p-4 border border-white/10 flex-1 flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs">
                  <span class="text-slate-400 font-semibold">Acoustics</span>
                  <span class="text-emerald-400 font-bold fan-duty">Quiet</span>
                </div>
                <div class="text-xl font-bold text-white fan-pct">23%</div>
                <div class="w-full bg-white/10 h-1.5 rounded-full overflow-hidden">
                  <div class="bg-emerald-400 h-full rounded-full fan-bar" style="width: 23%;"></div>
                </div>
              </div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 6: RETRO HACKER TUI / CLI (Linux terminal / htop vibe)
             ======================================================== -->
        <div id="variant-6" class="overlay-card hidden w-full max-w-3xl font-mono">
          <div class="text-xs uppercase tracking-wider text-emerald-400 font-bold mb-2">
            Вариант 6: Retro Terminal / TUI (В стиле htop и Linux CLI)
           <button onclick="applyThemeToTv(\'matrix\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="bg-[#050b07]/95 border-2 border-emerald-500/60 rounded-lg p-4 text-emerald-400 shadow-[0_0_25px_rgba(16,185,129,0.2)]">
            <div class="flex justify-between border-b border-emerald-800/80 pb-2 mb-3 text-xs">
              <span class="font-bold flex items-center gap-2">
                <span class="w-2 h-2 rounded-full bg-emerald-400 animate-ping"></span>
                DEV://AMD_CUSTOM_APU_0 (ZEN2 + RDNA2)
              </span>
              <span class="text-emerald-500">[STATUS: NORMAL]</span>
            </div>

            <!-- CPU CLI Row -->
            <div class="space-y-1 text-xs mb-3">
              <div class="flex justify-between">
                <span>CPU_TEMP: <strong class="text-emerald-200 cpu-temp-text">54°C</strong></span>
                <span>CPU_LOAD: <strong class="text-emerald-200 cpu-load-text">0%</strong></span>
              </div>
              <div class="flex items-center gap-2 text-[11px]">
                <span class="text-emerald-600">[0]</span>
                <span class="text-emerald-300 font-mono tracking-tighter">|░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|</span>
                <span class="text-emerald-500">2%</span>
                <span class="text-emerald-600">[1]</span>
                <span class="text-emerald-300 font-mono tracking-tighter">|░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|</span>
                <span class="text-emerald-500">0%</span>
              </div>
              <div class="flex items-center gap-2 text-[11px]">
                <span class="text-emerald-600">[2]</span>
                <span class="text-emerald-300 font-mono tracking-tighter">|░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|</span>
                <span class="text-emerald-500">0%</span>
                <span class="text-emerald-600">[3]</span>
                <span class="text-emerald-300 font-mono tracking-tighter">|░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|</span>
                <span class="text-emerald-500">5%</span>
              </div>
            </div>

            <!-- GPU & VRAM CLI Row -->
            <div class="space-y-1 text-xs mb-3 border-t border-emerald-900/60 pt-2">
              <div class="flex justify-between">
                <span>GPU_TEMP: <strong class="text-emerald-200 gpu-temp-text">47°C</strong></span>
                <span>VRAM: <span class="vram-text">0.3/16.0GB</span> (<span class="vram-pct text-emerald-200">2%</span>)</span>
              </div>
              <div class="text-[11px] flex items-center gap-2">
                <span>GPU_BAR:</span>
                <span class="text-emerald-300 font-mono flex-1">|█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|</span>
              </div>
            </div>

            <!-- RAM & FAN CLI Row -->
            <div class="grid grid-cols-2 gap-4 text-xs border-t border-emerald-900/60 pt-2">
              <div>
                <div class="flex justify-between mb-1">
                  <span>RAM_MEM:</span>
                  <span class="ram-used font-bold">0.6 GB (3%)</span>
                </div>
                <div class="h-1.5 bg-emerald-950 border border-emerald-800">
                  <div class="bg-emerald-400 h-full ram-bar" style="width: 3%;"></div>
                </div>
              </div>
              <div>
                <div class="flex justify-between mb-1">
                  <span>FAN_TACH:</span>
                  <span><strong class="fan-pct">23%</strong> (<span class="fan-duty">Quiet</span>)</span>
                </div>
                <div class="h-1.5 bg-emerald-950 border border-emerald-800">
                  <div class="bg-emerald-400 h-full fan-bar" style="width: 23%;"></div>
                </div>
              </div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 7: RADIAL GAUGES (Sim-Racing / Speedometer Cluster)
             ======================================================== -->
        <div id="variant-7" class="overlay-card hidden w-full max-w-4xl">
          <div class="text-xs uppercase tracking-wider text-amber-400 font-bold mb-2">
            Вариант 7: Radial Cockpit Clusters (Спидометры / Автосимуляторы)
           <button onclick="applyThemeToTv(\'radial\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="bg-[#0e121a]/95 border border-slate-800 rounded-3xl p-6 shadow-2xl backdrop-blur-xl">
            <div class="grid grid-cols-2 md:grid-cols-4 gap-6 text-center">
              
              <!-- Radial CPU -->
              <div class="flex flex-col items-center">
                <div class="relative w-28 h-28 flex items-center justify-center">
                  <svg class="w-full h-full -rotate-90 transform" viewBox="0 0 100 100">
                    <circle cx="50" cy="50" r="40" stroke="#1e293b" stroke-width="8" fill="none" />
                    <circle id="radialCpuCircle" cx="50" cy="50" r="40" stroke="#00a3ff" stroke-width="8" stroke-dasharray="251.2" stroke-dashoffset="240" stroke-linecap="round" fill="none" class="transition-all duration-500" />
                  </svg>
                  <div class="absolute flex flex-col items-center">
                    <span class="text-xl font-black text-white cpu-temp-main">54°</span>
                    <span class="text-[10px] text-cyan-400 font-mono cpu-load-text">0% Load</span>
                  </div>
                </div>
                <span class="text-xs font-bold text-slate-300 mt-2">CPU (ZEN 2)</span>
                <span class="text-[10px] text-slate-500">8 Cores Active</span>
              </div>

              <!-- Radial GPU -->
              <div class="flex flex-col items-center">
                <div class="relative w-28 h-28 flex items-center justify-center">
                  <svg class="w-full h-full -rotate-90 transform" viewBox="0 0 100 100">
                    <circle cx="50" cy="50" r="40" stroke="#1e293b" stroke-width="8" fill="none" />
                    <circle id="radialGpuCircle" cx="50" cy="50" r="40" stroke="#f59e0b" stroke-width="8" stroke-dasharray="251.2" stroke-dashoffset="240" stroke-linecap="round" fill="none" class="transition-all duration-500" />
                  </svg>
                  <div class="absolute flex flex-col items-center">
                    <span class="text-xl font-black text-white gpu-temp-main">47°</span>
                    <span class="text-[10px] text-amber-400 font-mono vram-pct">2% VRAM</span>
                  </div>
                </div>
                <span class="text-xs font-bold text-slate-300 mt-2">GPU (RDNA 2)</span>
                <span class="text-[10px] text-slate-500 vram-text">0.3 / 16.0 GB</span>
              </div>

              <!-- Radial RAM -->
              <div class="flex flex-col items-center">
                <div class="relative w-28 h-28 flex items-center justify-center">
                  <svg class="w-full h-full -rotate-90 transform" viewBox="0 0 100 100">
                    <circle cx="50" cy="50" r="40" stroke="#1e293b" stroke-width="8" fill="none" />
                    <circle id="radialRamCircle" cx="50" cy="50" r="40" stroke="#a855f7" stroke-width="8" stroke-dasharray="251.2" stroke-dashoffset="243" stroke-linecap="round" fill="none" class="transition-all duration-500" />
                  </svg>
                  <div class="absolute flex flex-col items-center">
                    <span class="text-xl font-black text-white ram-used">0.6G</span>
                    <span class="text-[10px] text-purple-400 font-mono ram-pct">3%</span>
                  </div>
                </div>
                <span class="text-xs font-bold text-slate-300 mt-2">SYSTEM RAM</span>
                <span class="text-[10px] text-slate-500">16.0 GB Total</span>
              </div>

              <!-- Radial Fan -->
              <div class="flex flex-col items-center">
                <div class="relative w-28 h-28 flex items-center justify-center">
                  <svg class="w-full h-full -rotate-90 transform" viewBox="0 0 100 100">
                    <circle cx="50" cy="50" r="40" stroke="#1e293b" stroke-width="8" fill="none" />
                    <circle id="radialFanCircle" cx="50" cy="50" r="40" stroke="#10b981" stroke-width="8" stroke-dasharray="251.2" stroke-dashoffset="193" stroke-linecap="round" fill="none" class="transition-all duration-500" />
                  </svg>
                  <div class="absolute flex flex-col items-center">
                    <span class="text-xl font-black text-white fan-pct">23%</span>
                    <span class="text-[10px] text-emerald-400 font-mono fan-duty">Quiet</span>
                  </div>
                </div>
                <span class="text-xs font-bold text-slate-300 mt-2">COOLING FAN</span>
                <span class="text-[10px] text-slate-500">Duty Cycle</span>
              </div>

            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 8: FLOATING MICRO-PILLS (Dispersed minimalist capsules)
             ======================================================== -->
        <div id="variant-8" class="overlay-card hidden w-full max-w-3xl">
          <div class="text-xs uppercase tracking-wider text-rose-400 font-bold mb-2">
            Вариант 8: Floating Micro-Pills (Модульные плавающие капсулы)
           <button onclick="applyThemeToTv(\'pills\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="flex flex-wrap items-center justify-center gap-3">
            <!-- CPU Pill -->
            <div class="bg-[#0b101c]/90 border border-slate-700/70 rounded-full px-4 py-2 flex items-center gap-3 shadow-xl backdrop-blur-md hover:border-cyan-500 transition">
              <span class="w-2.5 h-2.5 rounded-full bg-cyan-400"></span>
              <div class="text-xs">
                <span class="text-slate-400 font-medium">CPU</span>
                <span class="text-white font-bold ml-1.5 cpu-temp-text">54°C</span>
              </div>
              <span class="text-xs font-mono text-cyan-400 cpu-load-text font-bold bg-cyan-950/60 px-1.5 py-0.5 rounded">0%</span>
            </div>

            <!-- GPU Pill -->
            <div class="bg-[#0b101c]/90 border border-slate-700/70 rounded-full px-4 py-2 flex items-center gap-3 shadow-xl backdrop-blur-md hover:border-blue-500 transition">
              <span class="w-2.5 h-2.5 rounded-full bg-blue-400"></span>
              <div class="text-xs">
                <span class="text-slate-400 font-medium">GPU</span>
                <span class="text-white font-bold ml-1.5 gpu-temp-text">47°C</span>
              </div>
              <span class="text-xs font-mono text-blue-300 font-semibold vram-text">0.3G</span>
            </div>

            <!-- RAM Pill -->
            <div class="bg-[#0b101c]/90 border border-slate-700/70 rounded-full px-4 py-2 flex items-center gap-3 shadow-xl backdrop-blur-md hover:border-purple-500 transition">
              <span class="w-2.5 h-2.5 rounded-full bg-purple-400"></span>
              <div class="text-xs">
                <span class="text-slate-400 font-medium">RAM</span>
                <span class="text-white font-bold ml-1.5 ram-used">0.6 GB</span>
              </div>
              <span class="text-xs font-mono text-purple-300 font-semibold ram-pct">3%</span>
            </div>

            <!-- FAN Pill -->
            <div class="bg-[#0b101c]/90 border border-slate-700/70 rounded-full px-4 py-2 flex items-center gap-3 shadow-xl backdrop-blur-md hover:border-emerald-500 transition">
              <span class="w-2.5 h-2.5 rounded-full bg-emerald-400"></span>
              <div class="text-xs">
                <span class="text-slate-400 font-medium">FAN</span>
                <span class="text-white font-bold ml-1.5 fan-pct">23%</span>
              </div>
              <span class="text-[10px] font-mono text-emerald-300 uppercase bg-emerald-950/60 px-1.5 py-0.5 rounded fan-duty">Quiet</span>
            </div>
          </div>
          <p class="text-center text-[11px] text-slate-400 mt-4">Капсулы можно свободно расставлять по углам монитора в любых конфигурациях.</p>
        </div>
        <!-- ========================================================
             VARIANT 9: TACTICAL STEALTH (Asus ROG / Gunmetal / Crimson)
             ======================================================== -->
        <div id="variant-9" class="overlay-card hidden w-full max-w-4xl space-y-4">
          <div class="text-xs uppercase tracking-widest text-red-500 font-bold mb-1 flex items-center gap-2">
            <span class="w-2 h-2 bg-red-500 rotate-45"></span>
            Вариант 9: ROG Tactical Stealth (Агрессивный геймерский дизайн)
           <button onclick="applyThemeToTv(\'rog\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="grid grid-cols-1 md:grid-cols-4 gap-3">
            <!-- CPU Tactical Block -->
            <div class="bg-[#121318] border-t-2 border-t-red-600 border-x border-b border-zinc-800 p-4 relative shadow-2xl">
              <div class="flex justify-between items-center text-[11px] font-bold text-zinc-400 uppercase">
                <span>CPU [ZEN 2]</span>
                <span class="text-red-500 font-mono cpu-temp-text">54 °C</span>
              </div>
              <div class="text-3xl font-black text-white my-2 font-mono cpu-temp-main">54°C</div>
              <div class="flex justify-between text-xs text-zinc-400 mb-1">
                <span>Core Load</span>
                <span class="text-red-400 font-bold cpu-load-text">0%</span>
              </div>
              <div class="h-1 bg-zinc-800">
                <div class="h-full bg-red-600 cpu-load-bar" style="width: 2%;"></div>
              </div>
              <div class="mt-3 grid grid-cols-4 gap-1 text-[9px] font-mono text-zinc-500">
                <div>C0: 2%</div><div>C1: 0%</div><div>C2: 0%</div><div>C3: 5%</div>
              </div>
            </div>

            <!-- GPU Tactical Block -->
            <div class="bg-[#121318] border-t-2 border-t-red-600 border-x border-b border-zinc-800 p-4 relative shadow-2xl">
              <div class="flex justify-between items-center text-[11px] font-bold text-zinc-400 uppercase">
                <span>GPU [RDNA 2]</span>
                <span class="text-red-500 font-mono gpu-temp-text">47 °C</span>
              </div>
              <div class="text-3xl font-black text-white my-2 font-mono gpu-temp-main">47°C</div>
              <div class="flex justify-between text-xs text-zinc-400 mb-1">
                <span>VRAM Load</span>
                <span class="text-red-400 font-bold vram-pct">2%</span>
              </div>
              <div class="h-1 bg-zinc-800">
                <div class="h-full bg-red-600 vram-bar" style="width: 2%;"></div>
              </div>
              <div class="mt-3 text-[10px] text-zinc-500 font-mono vram-text">0.3 / 16.0 GB</div>
            </div>

            <!-- RAM Tactical Block -->
            <div class="bg-[#121318] border-t-2 border-t-zinc-500 border-x border-b border-zinc-800 p-4 relative shadow-2xl">
              <div class="flex justify-between items-center text-[11px] font-bold text-zinc-400 uppercase">
                <span>SYS MEMORY</span>
                <span class="text-zinc-300 font-mono ram-pct">3%</span>
              </div>
              <div class="text-3xl font-black text-white my-2 font-mono ram-used">0.6GB</div>
              <div class="flex justify-between text-xs text-zinc-400 mb-1">
                <span>Allocation</span>
                <span class="text-zinc-300 font-mono">16.0 GB</span>
              </div>
              <div class="h-1 bg-zinc-800">
                <div class="h-full bg-zinc-400 ram-bar" style="width: 3%;"></div>
              </div>
              <div class="mt-3 text-[10px] text-zinc-500 font-mono">LPDDR5 5500</div>
            </div>

            <!-- FAN Tactical Block -->
            <div class="bg-[#121318] border-t-2 border-t-emerald-500 border-x border-b border-zinc-800 p-4 relative shadow-2xl">
              <div class="flex justify-between items-center text-[11px] font-bold text-zinc-400 uppercase">
                <span>FAN PROFILE</span>
                <span class="text-emerald-400 font-mono fan-duty">QUIET</span>
              </div>
              <div class="text-3xl font-black text-white my-2 font-mono fan-pct-main">23%</div>
              <div class="flex justify-between text-xs text-zinc-400 mb-1">
                <span>Pulse Width</span>
                <span class="text-emerald-400 font-bold fan-pct">23%</span>
              </div>
              <div class="h-1 bg-zinc-800">
                <div class="h-full bg-emerald-500 fan-bar" style="width: 23%;"></div>
              </div>
              <div class="mt-3 text-[10px] text-emerald-500/80 font-mono">Silent Bearing</div>
            </div>
          </div>
        </div>
        <!-- ========================================================
             VARIANT 10: NEON PRISM GRADIENT (Vibrant border glow / Gen-Z Aesthetic)
             ======================================================== -->
        <div id="variant-10" class="overlay-card hidden w-full max-w-4xl space-y-4">
          <div class="text-xs uppercase tracking-widest text-transparent bg-clip-text bg-gradient-to-r from-pink-500 via-purple-500 to-cyan-500 font-bold mb-1">
            Вариант 10: Neon Prism (Градиентные неоновые рамки и стеклянная глубина)
           <button onclick="applyThemeToTv(\'prism\')" class="ml-3 px-2.5 py-0.5 rounded-md text-[11px] font-bold bg-cyan-500/20 hover:bg-cyan-500/40 text-cyan-300 border border-cyan-500/50 transition">📺 Включить этот стиль на ТВ</button></div>

          <div class="grid grid-cols-1 md:grid-cols-4 gap-3.5">
            <!-- CPU Card -->
            <div class="relative p-0.5 rounded-2xl bg-gradient-to-b from-cyan-500 to-blue-600 shadow-lg shadow-cyan-500/10">
              <div class="bg-[#0d1322] p-4 rounded-[14px] h-full flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs font-bold text-cyan-300">
                  <span>CPU (ZEN 2)</span>
                  <span class="cpu-temp-text bg-cyan-500/20 px-2 py-0.5 rounded-full text-cyan-300">54°C</span>
                </div>
                <div class="my-3">
                  <div class="text-3xl font-extrabold text-white cpu-temp-main">54 °C</div>
                  <div class="text-xs text-slate-400 mt-0.5">Load: <span class="text-cyan-400 font-bold cpu-load-text">0%</span></div>
                </div>
                <div class="h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
                  <div class="h-full bg-gradient-to-r from-cyan-400 to-blue-500 rounded-full cpu-load-bar" style="width: 2%;"></div>
                </div>
              </div>
            </div>

            <!-- GPU Card -->
            <div class="relative p-0.5 rounded-2xl bg-gradient-to-b from-purple-500 to-pink-600 shadow-lg shadow-purple-500/10">
              <div class="bg-[#0d1322] p-4 rounded-[14px] h-full flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs font-bold text-purple-300">
                  <span>GPU (RDNA 2)</span>
                  <span class="gpu-temp-text bg-purple-500/20 px-2 py-0.5 rounded-full text-purple-300">47°C</span>
                </div>
                <div class="my-3">
                  <div class="text-3xl font-extrabold text-white gpu-temp-main">47 °C</div>
                  <div class="text-xs text-slate-400 mt-0.5">VRAM: <span class="text-purple-400 font-bold vram-pct">2%</span></div>
                </div>
                <div class="h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
                  <div class="h-full bg-gradient-to-r from-purple-400 to-pink-500 rounded-full vram-bar" style="width: 2%;"></div>
                </div>
              </div>
            </div>

            <!-- RAM Card -->
            <div class="relative p-0.5 rounded-2xl bg-gradient-to-b from-blue-500 to-emerald-500 shadow-lg shadow-blue-500/10">
              <div class="bg-[#0d1322] p-4 rounded-[14px] h-full flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs font-bold text-blue-300">
                  <span>SYSTEM RAM</span>
                  <span class="ram-pct bg-blue-500/20 px-2 py-0.5 rounded-full text-blue-300">3%</span>
                </div>
                <div class="my-3">
                  <div class="text-3xl font-extrabold text-white ram-used">0.6 GB</div>
                  <div class="text-xs text-slate-400 mt-0.5">Total: <span class="text-slate-200">16.0 GB</span></div>
                </div>
                <div class="h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
                  <div class="h-full bg-gradient-to-r from-blue-400 to-emerald-400 rounded-full ram-bar" style="width: 3%;"></div>
                </div>
              </div>
            </div>

            <!-- Fan Card -->
            <div class="relative p-0.5 rounded-2xl bg-gradient-to-b from-emerald-400 to-teal-600 shadow-lg shadow-emerald-500/10">
              <div class="bg-[#0d1322] p-4 rounded-[14px] h-full flex flex-col justify-between">
                <div class="flex justify-between items-center text-xs font-bold text-emerald-300">
                  <span>COOLING FAN</span>
                  <span class="fan-pct bg-emerald-500/20 px-2 py-0.5 rounded-full text-emerald-300">23%</span>
                </div>
                <div class="my-3">
                  <div class="text-3xl font-extrabold text-white fan-pct-main">23%</div>
                  <div class="text-xs text-slate-400 mt-0.5">Duty: <span class="text-emerald-400 font-bold fan-duty">Quiet</span></div>
                </div>
                <div class="h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
                  <div class="h-full bg-gradient-to-r from-emerald-400 to-teal-500 rounded-full fan-bar" style="width: 23%;"></div>
                </div>
              </div>
            </div>
          </div>
        </div>

      </div><!-- End singleViewWrapper -->

      <!-- CONTAINER FOR ALL 10 IN GRID (Hidden by default) -->
      <div id="allViewWrapper" class="hidden w-full space-y-10 py-6">
        <!-- Javascript dynamically duplicates content or exposes all 10 designs in grid -->
      </div>
    </div>
    <!-- Design Comparison & Advice Section -->
    <div class="mt-8 grid grid-cols-1 md:grid-cols-3 gap-4">
      <div class="bg-slate-900/90 border border-slate-800 p-4 rounded-xl">
        <div class="flex items-center gap-2 text-cyan-400 font-bold text-sm mb-2">
          <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
          Для Steam Deck & Портативок
        </div>
        <p class="text-xs text-slate-300 leading-relaxed">
          Идеально подходят <strong>Вариант 1</strong> (Deck OS Refined) и <strong>Вариант 5</strong> (Bento Glass). Крупные цифры температуры и компактные прогресс-бары отлично считываются на 7–8 дюймовых экранах при разрешении 800p/1200p.
        </p>
      </div>

      <div class="bg-slate-900/90 border border-slate-800 p-4 rounded-xl">
        <div class="flex items-center gap-2 text-emerald-400 font-bold text-sm mb-2">
          <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9.75 17L9 20l-1 1h8l-1-1-.75-3M3 13h18M5 17h14a2 2 0 002-2V5a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z" /></svg>
          Для Стримеров (OBS / Twitch)
        </div>
        <p class="text-xs text-slate-300 leading-relaxed">
          Используйте <strong>Вариант 4</strong> (Streamer Vertical Dock) или <strong>Вариант 8</strong> (Floating Pills) с режимом фона «OBS». Они не перекрывают мини-карту и инвентарь в игре, отлично вписываясь в углы трансляции.
        </p>
      </div>

      <div class="bg-slate-900/90 border border-slate-800 p-4 rounded-xl">
        <div class="flex items-center gap-2 text-yellow-400 font-bold text-sm mb-2">
          <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
          Для Киберспорта и Тяжелых игр
        </div>
        <p class="text-xs text-slate-300 leading-relaxed">
          <strong>Вариант 3</strong> (Esports Top Bar) практически незаметен периферийным зрением и не отвлекает при прицеливании, при этом давая мгновенную информацию о просадках частот APU.
        </p>
      </div>
    </div>

    <!-- Quick Code / Export modal trigger notification -->
    <div class="mt-6 text-center text-xs text-slate-500">
      Все 10 стилей полностью адаптивны, поддерживают живое обновление данных через JS и используют современный стек HTML5 + Tailwind CSS.
    </div>
  </main>
  <script>
    // State
    let currentVariant = 1;
    let isGamingLoad = false;
    let isViewAll = false;
    let simInterval = null;

    // Base telemetry values from user screenshot
    const idleData = {
      cpuTemp: 54,
      cpuLoad: 0,
      cores: [2, 0, 0, 5, 0, 0, 2, 0],
      gpuTemp: 47,
      vramGB: 0.3,
      vramPct: 2,
      ramGB: 0.6,
      ramPct: 3,
      fanPct: 23,
      fanDuty: 'Quiet'
    };

    const gamingData = {
      cpuTemp: 79,
      cpuLoad: 68,
      cores: [72, 85, 60, 91, 55, 48, 77, 64],
      gpuTemp: 74,
      vramGB: 7.8,
      vramPct: 49,
      ramGB: 11.4,
      ramPct: 71,
      fanPct: 78,
      fanDuty: 'Performance'
    };

    let activeData = { ...idleData };

    // Select Variant
    function selectVariant(num) {
      if (isViewAll) {
        toggleViewAll();
      }
      currentVariant = num;

      // Update tabs
      document.querySelectorAll('.var-tab-btn').forEach(btn => {
        if (parseInt(btn.getAttribute('data-var')) === num) {
          btn.className = 'var-tab-btn active px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap bg-cyan-500 text-white';
        } else {
          btn.className = 'var-tab-btn px-3 py-1 rounded-full text-xs font-semibold whitespace-nowrap text-slate-400 hover:text-slate-200 hover:bg-slate-800';
        }
      });

      // Show specific card
      for (let i = 1; i <= 10; i++) {
        const el = document.getElementById(`variant-${i}`);
        if (el) {
          if (i === num) {
            el.classList.remove('hidden');
          } else {
            el.classList.add('hidden');
          }
        }
      }
      updateUI();
    }
    // Update all visual values on screen
    function updateUI() {
      // CPU
      document.querySelectorAll('.cpu-temp-text').forEach(el => el.textContent = `${activeData.cpuTemp} °C`);
      document.querySelectorAll('.cpu-temp-main').forEach(el => el.textContent = `${activeData.cpuTemp} °C`);
      document.querySelectorAll('.cpu-load-text').forEach(el => el.textContent = `${activeData.cpuLoad}%`);
      document.querySelectorAll('.cpu-load-bar').forEach(el => el.style.width = `${Math.max(activeData.cpuLoad, 2)}%`);

      // GPU
      document.querySelectorAll('.gpu-temp-text').forEach(el => el.textContent = `${activeData.gpuTemp} °C`);
      document.querySelectorAll('.gpu-temp-main').forEach(el => el.textContent = `${activeData.gpuTemp} °C`);
      document.querySelectorAll('.vram-text').forEach(el => el.textContent = `${activeData.vramGB.toFixed(1)} / 16.0 GB`);
      document.querySelectorAll('.vram-pct').forEach(el => el.textContent = `${activeData.vramPct}%`);
      document.querySelectorAll('.vram-bar').forEach(el => el.style.width = `${Math.max(activeData.vramPct, 2)}%`);

      // RAM
      document.querySelectorAll('.ram-used').forEach(el => el.textContent = `${activeData.ramGB.toFixed(1)} GB`);
      document.querySelectorAll('.ram-pct').forEach(el => el.textContent = `${activeData.ramPct}%`);
      document.querySelectorAll('.ram-bar').forEach(el => el.style.width = `${Math.max(activeData.ramPct, 2)}%`);

      // FAN
      document.querySelectorAll('.fan-pct').forEach(el => el.textContent = `${activeData.fanPct}%`);
      document.querySelectorAll('.fan-pct-main').forEach(el => el.textContent = `${activeData.fanPct}%`);
      document.querySelectorAll('.fan-duty').forEach(el => el.textContent = activeData.fanDuty);
      document.querySelectorAll('.fan-bar').forEach(el => el.style.width = `${activeData.fanPct}%`);

      // Radial circles (Variant 7)
      const circumference = 251.2;
      const setRadial = (id, pct) => {
        const circle = document.getElementById(id);
        if (circle) {
          const offset = circumference - (pct / 100) * circumference;
          circle.style.strokeDashoffset = offset;
        }
      };
      setRadial('radialCpuCircle', (activeData.cpuTemp / 100) * 100);
      setRadial('radialGpuCircle', (activeData.gpuTemp / 100) * 100);
      setRadial('radialRamCircle', activeData.ramPct);
      setRadial('radialFanCircle', activeData.fanPct);

      // Core fills
      document.querySelectorAll('.core-fill').forEach((fill, index) => {
        const coreVal = activeData.cores[index % 8];
        fill.style.height = `${coreVal}%`;
        fill.style.width = `${coreVal}%`;
      });
    }

    // Toggle simulation load
    function toggleSim() {
      isGamingLoad = !isGamingLoad;
      const simBtn = document.getElementById('simBtn');
      const simDot = document.getElementById('simDot');
      const simText = document.getElementById('simText');

      if (isGamingLoad) {
        simDot.className = 'w-2 h-2 rounded-full bg-red-500 animate-ping';
        simBtn.className = 'flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-red-500/50 bg-red-950/40 text-red-200 transition';
        simText.textContent = 'Режим: Cyberpunk 2077 (Load)';
        activeData = { ...gamingData };
      } else {
        simDot.className = 'w-2 h-2 rounded-full bg-emerald-400 animate-pulse';
        simBtn.className = 'flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-200 transition';
        simText.textContent = 'Режим: Простой (Idle)';
        activeData = { ...idleData };
      }
      updateUI();
    }
    // Background Switcher
    function setBg(type) {
      const canvas = document.getElementById('previewCanvas');
      const btnGame = document.getElementById('bgBtnGame');
      const btnDark = document.getElementById('bgBtnDark');
      const btnObs = document.getElementById('bgBtnObs');

      const resetBtns = () => {
        btnGame.className = 'px-2.5 py-1 rounded-md transition font-medium text-slate-400 hover:text-white';
        btnDark.className = 'px-2.5 py-1 rounded-md transition font-medium text-slate-400 hover:text-white';
        btnObs.className = 'px-2.5 py-1 rounded-md transition font-medium text-slate-400 hover:text-white';
      };

      resetBtns();

      if (type === 'game') {
        btnGame.className = 'px-2.5 py-1 rounded-md transition font-medium bg-cyan-500 text-white';
        canvas.style.background = "linear-gradient(rgba(10, 14, 23, 0.78), rgba(6, 9, 15, 0.88)), url('https://images.unsplash.com/photo-1542751371-adc38448a05e?q=80&w=1600&auto=format&fit=crop') center/cover no-repeat";
      } else if (type === 'dark') {
        btnDark.className = 'px-2.5 py-1 rounded-md transition font-medium bg-cyan-500 text-white';
        canvas.style.background = "#090d14";
      } else if (type === 'obs') {
        btnObs.className = 'px-2.5 py-1 rounded-md transition font-medium bg-cyan-500 text-white';
        canvas.style.background = "repeating-conic-gradient(#151923 0% 25%, #0e121a 0% 50%) 50% / 20px 20px";
      }
    }

    // Toggle View All 10 designs in single scrollable view
    function toggleViewAll() {
      isViewAll = !isViewAll;
      const singleWrapper = document.getElementById('singleViewWrapper');
      const allWrapper = document.getElementById('allViewWrapper');
      const viewAllBtn = document.getElementById('viewAllBtn');

      if (isViewAll) {
        viewAllBtn.textContent = 'Один вариант';
        viewAllBtn.className = 'px-3 py-1.5 rounded-lg border border-emerald-500/40 bg-emerald-950/40 text-emerald-300 transition font-medium';
        singleWrapper.classList.add('hidden');
        allWrapper.classList.remove('hidden');

        // Show all cards
        allWrapper.innerHTML = '';
        for (let i = 1; i <= 10; i++) {
          const card = document.getElementById(`variant-${i}`);
          if (card) {
            const clone = card.cloneNode(true);
            clone.classList.remove('hidden');
            clone.classList.add('mx-auto');
            allWrapper.appendChild(clone);
          }
        }
      } else {
        viewAllBtn.textContent = 'Показать все 10 разом';
        viewAllBtn.className = 'px-3 py-1.5 rounded-lg border border-cyan-500/40 bg-cyan-950/30 text-cyan-300 hover:bg-cyan-900/40 transition font-medium';
        allWrapper.classList.add('hidden');
        singleWrapper.classList.remove('hidden');
        selectVariant(currentVariant);
      }
      updateUI();
    }

    
    const varMap = {
      1: "deck",
      2: "cyber",
      3: "esports",
      4: "dock",
      5: "bento",
      6: "matrix",
      7: "radial",
      8: "pills",
      9: "rog",
      10: "prism"
    };

    function applyCurrentVariantToTv() {
      const theme = varMap[currentVariant] || "esports";
      applyThemeToTv(theme);
    }

    async function applyThemeToTv(themeName) {
      const pos = document.getElementById('tvPosSelect')?.value || 'top';
      const tag = document.getElementById('tvStatusTag');
      if (tag) tag.textContent = 'Применяем ' + themeName + '...';
      try {
        const res = await fetch('/api/theme/set?name=' + encodeURIComponent(themeName) + '&pos=' + encodeURIComponent(pos));
        if (res.ok) {
          const d = await res.json();
          if (tag) tag.textContent = '✓ Активно на ТВ: ' + d.theme.toUpperCase() + ' (' + d.pos + ')';
        } else {
          if (tag) tag.textContent = 'Ошибка сервера';
        }
      } catch (err) {
        if (tag) tag.textContent = 'Нет связи с PS5 (' + err + ')';
      }
    }

    async function turnOffTvOverlay() {
      const tag = document.getElementById('tvStatusTag');
      if (tag) tag.textContent = 'Выключаем...';
      try {
        const res = await fetch('/api/theme/set?name=none&pos=top');
        if (res.ok) {
          if (tag) tag.textContent = 'Оверлей на ТВ выключен';
        }
      } catch (err) {
        if (tag) tag.textContent = 'Ошибка: ' + err;
      }
    }

    function onPosChanged() {
      applyCurrentVariantToTv();
    }

    async function checkTvStatus() {
      try {
        const res = await fetch('/api/theme');
        if (res.ok) {
          const d = await res.json();
          const tag = document.getElementById('tvStatusTag');
          if (tag) {
            if (d.theme === 'none' || d.theme === 'off' || d.theme === 'hide') {
              tag.textContent = 'Оверлей на ТВ выключен';
            } else {
              tag.textContent = 'Активно на ТВ: ' + d.theme.toUpperCase() + ' (' + d.pos + ')';
            }
          }
          const sel = document.getElementById('tvPosSelect');
          if (sel && d.pos) {
            sel.value = d.pos;
          }
        }
      } catch (e) {}
    }

    // Real PS5 Telemetry Polling
    let realDataActive = false;
    async function pollRealTelemetry() {
      try {
        const res = await fetch('/api/metrics');
        if (res.ok) {
          const m = await res.json();
          realDataActive = true;
          activeData.cpuTemp = m.cpu_temp || 0;
          activeData.gpuTemp = m.soc_temp || 0;
          activeData.cpuLoad = Math.round(m.cpu_usage || 0);
          if (m.cpu_cores && m.cpu_cores.length >= 8) {
            activeData.cores = m.cpu_cores.map(c => Math.round(c));
          }
          activeData.ramGB = (m.ram_used_mb || 0) / 1024.0;
          activeData.ramPct = Math.round(m.ram_pct || 0);
          activeData.vramGB = (m.vram_used_mb || 0) / 1024.0;
          activeData.vramPct = Math.round(m.vram_pct || 0);
          activeData.fanPct = m.fan_duty_pct || 0;
          activeData.fanDuty = (m.fan_duty_pct < 35) ? 'Quiet' : ((m.fan_duty_pct < 65) ? 'Balanced' : 'Performance');
          updateUI();

          const simText = document.getElementById('simText');
          if (simText) simText.textContent = 'PS5 Реальная телеметрия';
          const simDot = document.getElementById('simDot');
          if (simDot) simDot.className = 'w-2 h-2 rounded-full bg-emerald-400 animate-pulse';
        }
      } catch (e) {
        realDataActive = false;
      }
    }

    setInterval(pollRealTelemetry, 1000);
    setInterval(checkTvStatus, 4000);
    setTimeout(() => {
      checkTvStatus();
      pollRealTelemetry();
    }, 300);

    // Live micro fluctuations
    setInterval(() => {
      if (isGamingLoad) {
        activeData.cpuTemp = 78 + Math.floor(Math.random() * 5);
        activeData.gpuTemp = 73 + Math.floor(Math.random() * 4);
        activeData.cpuLoad = 65 + Math.floor(Math.random() * 15);
      } else {
        activeData.cpuTemp = 53 + Math.floor(Math.random() * 3);
        activeData.gpuTemp = 46 + Math.floor(Math.random() * 3);
      }
      updateUI();
    }, 2000);

    // Initial render
    window.addEventListener('DOMContentLoaded', () => {
      selectVariant(1);
      updateUI();
    });
  </script>
</body>
</html>)rawliteral";
