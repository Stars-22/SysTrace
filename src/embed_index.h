// embed_index.h - Auto-generated from web/index.html by scripts/embed_index.py
// DO NOT EDIT MANUALLY - modify web/index.html instead and re-run the script

#pragma once

constexpr const char* INDEX_HTML = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SysTrace</title>
<style>
:root {
    --bg-primary: #0f0e17;
    --bg-secondary: #1a1a2e;
    --bg-card: #16213e;
    --bg-card-hover: #1c2847;
    --bg-surface: #0f3460;
    --border: #2a2a4a;
    --border-light: #3a3a5c;
    --text-primary: #e8e8e8;
    --text-secondary: #a0a0b8;
    --text-muted: #6c6c8a;
    --accent: #4ecdc4;
    --accent-glow: rgba(78,205,196,0.25);
    --brand: #e94560;
    --brand-glow: rgba(233,69,96,0.3);
    --cpu-blue: #4ecdc4;
    --mem-green: #ffe66d;
    --disk-orange: #ff9f43;
    --spike-red: #e94560;
    --spike-orange: #ff9f43;
}
*{margin:0;padding:0;box-sizing:border-box}
body{background:var(--bg-primary);color:var(--text-primary);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Helvetica Neue',Arial,sans-serif;font-size:14px;min-height:100vh;overflow-x:hidden}
#app{max-width:1440px;margin:0 auto;padding:24px 28px 40px}

/* ====== Header ====== */
header{display:flex;justify-content:space-between;align-items:center;padding:16px 24px;background:var(--bg-secondary);border:1px solid var(--border);border-radius:12px;margin-bottom:24px;position:relative;overflow:hidden}
header::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;background:linear-gradient(90deg,var(--accent),var(--brand),#a855f7,var(--accent));background-size:200% 100%;animation:gradientShift 4s ease infinite}
@keyframes gradientShift{0%{background-position:0% 50%}50%{background-position:100% 50%}100%{background-position:0% 50%}}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:36px;height:36px;border-radius:10px;background:linear-gradient(135deg,var(--bg-surface),#1a3a70);display:flex;align-items:center;justify-content:center;position:relative;box-shadow:0 2px 12px var(--accent-glow)}
.logo-icon .pulse{width:10px;height:10px;border-radius:50%;background:var(--accent);animation:pulse 2s ease-in-out infinite;position:relative}
.logo-icon .pulse::after{content:'';position:absolute;inset:-4px;border-radius:50%;border:2px solid var(--accent);opacity:0;animation:pulseRing 2s ease-in-out infinite}
@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:.7;transform:scale(.85)}}
@keyframes pulseRing{0%{transform:scale(.8);opacity:.6}100%{transform:scale(1.5);opacity:0}}
header h1{font-size:22px;font-weight:700;color:var(--text-primary);letter-spacing:-0.5px}
header h1 span{color:var(--brand);font-weight:800}
.realtime-cards{display:flex;gap:12px}
.rt-card{background:var(--bg-card);border:1px solid var(--border);border-radius:10px;padding:10px 18px;display:flex;align-items:center;gap:14px;min-width:140px;transition:border-color .3s,box-shadow .3s}
.rt-card:hover{border-color:var(--border-light);box-shadow:0 2px 16px rgba(0,0,0,.3)}
.rt-card .rt-icon{width:38px;height:38px;border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:16px;font-weight:700;flex-shrink:0}
.rt-card.cpu-card .rt-icon{background:rgba(78,205,196,.15);color:var(--cpu-blue)}
.rt-card.mem-card .rt-icon{background:rgba(255,230,109,.15);color:var(--mem-green)}
.rt-card .rt-body{display:flex;flex-direction:column}
.rt-card .rt-label{font-size:10px;text-transform:uppercase;letter-spacing:1px;color:var(--text-muted);line-height:1}
.rt-card .rt-value{font-size:22px;font-weight:700;line-height:1.2;font-variant-numeric:tabular-nums;color:var(--text-primary);transition:color .3s}

/* ====== Metric Tabs ====== */
.section{margin-bottom:20px}
.section-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
.section-title{font-size:15px;font-weight:600;color:var(--text-secondary);display:flex;align-items:center;gap:8px}
.section-title svg{width:16px;height:16px;opacity:.7}
.metric-tabs{display:flex;gap:4px;background:var(--bg-secondary);border:1px solid var(--border);border-radius:8px;padding:3px}
.metric-tab{padding:6px 16px;border:none;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500;background:transparent;color:var(--text-muted);transition:all .25s ease;font-family:inherit}
.metric-tab:hover{color:var(--text-primary);background:rgba(255,255,255,.05)}
.metric-tab.active{background:var(--bg-surface);color:var(--accent);box-shadow:0 1px 6px rgba(78,205,196,.2)}

/* ====== Heatmap Container ====== */
.heatmap-container{position:relative;background:var(--bg-card);border:1px solid var(--border);border-radius:12px;padding:20px 20px 14px;overflow:hidden;box-shadow:0 4px 24px rgba(0,0,0,.2)}
.heatmap-container::after{content:'';position:absolute;inset:0;pointer-events:none;box-shadow:inset 0 2px 8px rgba(0,0,0,.4);border-radius:12px}
canvas#heatmap{width:100%;height:180px;display:block;cursor:crosshair;border-radius:6px}

/* ====== Skeleton Loading ====== */
.skeleton-overlay{position:absolute;inset:20px 20px 14px;border-radius:6px;overflow:hidden;pointer-events:none;z-index:2;display:flex;flex-direction:column;gap:8px;padding:12px}
.skeleton-overlay.hidden{display:none}
.skel-bar{height:8px;border-radius:4px;background:linear-gradient(90deg,#1e2140 25%,#2a2d50 50%,#1e2140 75%);background-size:200% 100%;animation:shimmer 1.5s ease-in-out infinite;opacity:.6}
.skel-bar:nth-child(1){width:70%;animation-delay:0s}
.skel-bar:nth-child(2){width:100%;animation-delay:.15s}
.skel-bar:nth-child(3){width:85%;animation-delay:.3s}
.skel-bar:nth-child(4){width:60%;animation-delay:.45s}
@keyframes shimmer{0%{background-position:200% 0}100%{background-position:-200% 0}}

/* ====== Tooltip ====== */
.tooltip{position:fixed;background:var(--bg-primary);border:1px solid var(--border-light);border-radius:10px;padding:14px 16px;font-size:12px;pointer-events:none;z-index:1000;display:none;box-shadow:0 8px 32px rgba(0,0,0,.5);min-width:180px;backdrop-filter:blur(8px)}
.tooltip .tt-time{color:var(--accent);font-size:13px;font-weight:700;margin-bottom:8px;padding-bottom:6px;border-bottom:1px solid var(--border)}
.tt-row{display:flex;justify-content:space-between;align-items:center;padding:3px 0}
.tt-row .tt-label{color:var(--text-muted);font-size:11px}
.tt-row .tt-val{font-weight:600;font-variant-numeric:tabular-nums}
.tt-bar-wrap{height:3px;background:var(--border);border-radius:2px;margin-top:2px;flex:0 0 60px;margin-left:8px;overflow:hidden}
.tt-bar{height:100%;border-radius:2px;transition:width .2s ease}

/* ====== Legend ====== */
.legend{display:flex;gap:16px;margin-top:12px;font-size:11px;color:var(--text-muted);justify-content:center}
.legend-item{display:flex;align-items:center;gap:5px}
.legend-item .swatch{width:14px;height:10px;border-radius:2px}

/* ====== Detail Container ====== */
.detail-container{background:var(--bg-card);border:1px solid var(--border);border-radius:12px;padding:24px;box-shadow:0 4px 24px rgba(0,0,0,.2)}
.no-snapshot{text-align:center;color:var(--text-muted);padding:48px 20px;font-size:14px}
.no-snapshot .ns-icon{font-size:40px;margin-bottom:12px;opacity:.4;display:block}

/* ====== Snapshot Info ====== */
.snapshot-info{margin-bottom:20px}
.snap-header{display:flex;align-items:center;gap:10px;margin-bottom:14px}
.snap-header .clock-icon{width:32px;height:32px;border-radius:8px;background:var(--bg-surface);display:flex;align-items:center;justify-content:center;flex-shrink:0}
.snap-header .clock-icon svg{width:18px;height:18px;stroke:var(--accent);fill:none;stroke-width:2}
.snap-time{font-size:18px;font-weight:700}
.snap-time-sub{font-size:11px;color:var(--text-muted);margin-left:8px}
.sys-stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px}
.stat-card{background:var(--bg-secondary);border:1px solid var(--border);border-radius:8px;padding:10px 14px;display:flex;align-items:center;gap:10px}
.stat-card .sc-icon{width:28px;height:28px;border-radius:6px;display:flex;align-items:center;justify-content:center;font-size:12px;font-weight:700;flex-shrink:0}
.stat-card.cpu-card .sc-icon{background:rgba(78,205,196,.15);color:var(--cpu-blue)}
.stat-card.mem-card .sc-icon{background:rgba(255,230,109,.15);color:var(--mem-green)}
.stat-card.dr-card .sc-icon{background:rgba(255,159,67,.15);color:var(--disk-orange)}
.stat-card.dw-card .sc-icon{background:rgba(168,85,247,.15);color:#a855f7}
.stat-card .sc-body{display:flex;flex-direction:column}
.stat-card .sc-label{font-size:10px;text-transform:uppercase;letter-spacing:.5px;color:var(--text-muted)}
.stat-card .sc-value{font-size:15px;font-weight:700;font-variant-numeric:tabular-nums}

/* ====== Process Table ====== */
.table-wrap{border:1px solid var(--border);border-radius:8px;overflow:hidden;margin-top:4px}
.table-scroll{max-height:420px;overflow-y:auto;overflow-x:auto}
.table-scroll::-webkit-scrollbar{width:6px}
.table-scroll::-webkit-scrollbar-track{background:var(--bg-secondary)}
.table-scroll::-webkit-scrollbar-thumb{background:var(--border-light);border-radius:3px}
table.process-table{width:100%;border-collapse:collapse}
table.process-table th{background:var(--bg-surface);padding:10px 14px;text-align:left;cursor:pointer;user-select:none;font-size:11px;font-weight:600;color:var(--text-secondary);position:sticky;top:0;z-index:1;text-transform:uppercase;letter-spacing:.5px;border-bottom:1px solid var(--border)}
table.process-table th:hover{background:#1a3a70}
table.process-table th .sort-arrow{margin-left:4px;font-size:9px;opacity:.4;transition:opacity .2s}
table.process-table th.sorted .sort-arrow{opacity:1;color:var(--accent)}
table.process-table td{padding:9px 14px;border-bottom:1px solid rgba(42,42,74,.5);font-size:13px;transition:background .15s}
table.process-table tbody tr{transition:background .15s,border-left .15s}
table.process-table tbody tr:hover{background:var(--bg-card-hover)}
table.process-table tbody tr.spike-cpu{background:rgba(233,69,96,.08);border-left:3px solid var(--spike-red)}
table.process-table tbody tr.spike-cpu:hover{background:rgba(233,69,96,.15)}
table.process-table tbody tr.spike-mem{background:rgba(255,159,67,.08);border-left:3px solid var(--spike-orange)}
table.process-table tbody tr.spike-mem:hover{background:rgba(255,159,67,.15)}
table.process-table td.pid-cell{color:var(--text-muted);font-size:11px;font-family:'Consolas','Menlo',monospace}
table.process-table td.name-cell{font-weight:500;max-width:280px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.cpu-bar-wrap{display:inline-flex;align-items:center;gap:8px;min-width:120px}
.cpu-bar{display:inline-block;height:6px;border-radius:3px;min-width:2px;transition:width .3s ease}
.cpu-bar.low{background:var(--cpu-blue)}
.cpu-bar.mid{background:var(--mem-green)}
.cpu-bar.high{background:var(--spike-orange)}
.cpu-bar.extreme{background:var(--spike-red)}
.cpu-val{font-variant-numeric:tabular-nums;font-weight:600;min-width:42px}
.badge{font-size:10px;font-weight:600;padding:2px 7px;border-radius:4px;margin-left:4px;letter-spacing:.3px}
.badge.cpu-spike{background:rgba(233,69,96,.2);color:#f87171;border:1px solid rgba(233,69,96,.3)}
.badge.mem-spike{background:rgba(255,159,67,.2);color:#fbbf24;border:1px solid rgba(255,159,67,.3)}
.loading{text-align:center;padding:40px;color:var(--text-muted)}
.loading .spinner{width:28px;height:28px;border:3px solid var(--border);border-top-color:var(--accent);border-radius:50%;animation:spin .8s linear infinite;margin:0 auto 12px}
@keyframes spin{to{transform:rotate(360deg)}}
.error-box{background:rgba(233,69,96,.1);border:1px solid rgba(233,69,96,.3);border-radius:8px;padding:12px 16px;color:#f87171;font-size:13px;display:flex;align-items:center;gap:8px}
.error-box svg{flex-shrink:0}

/* ====== Fade-in Animation ====== */
.fade-in{animation:fadeIn .35s ease}
@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}

/* ====== Responsive ====== */
@media(max-width:768px){
    #app{padding:12px}
    header{flex-direction:column;gap:12px;padding:14px}
    .realtime-cards{width:100%}
    .rt-card{min-width:0;flex:1;padding:8px 12px}
    .rt-card .rt-value{font-size:18px}
    .sys-stats{grid-template-columns:1fr 1fr}
}
</style>
</head>
<body>
<div id="app">
<header>
  <div class="logo">
    <div class="logo-icon"><div class="pulse"></div></div>
    <h1>Sys<span>Trace</span></h1>
  </div>
  <div class="realtime-cards" id="realtime">
    <div class="rt-card cpu-card">
      <div class="rt-icon">C</div>
      <div class="rt-body">
        <div class="rt-label">CPU</div>
        <div class="rt-value" id="rt-cpu">--</div>
      </div>
    </div>
    <div class="rt-card mem-card">
      <div class="rt-icon">M</div>
      <div class="rt-body">
        <div class="rt-label">MEM</div>
        <div class="rt-value" id="rt-mem">--</div>
      </div>
    </div>
  </div>
</header>

<div class="section">
  <div class="section-header">
    <div class="section-title">
      <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="1" y="1" width="14" height="14" rx="2"/><rect x="3" y="4" width="2" height="8" rx="1"/><rect x="6.5" y="3" width="2" height="9" rx="1"/><rect x="10" y="5" width="2" height="7" rx="1"/></svg>
      Resource Heatmap
    </div>
    <div class="metric-tabs" id="metric-tabs">
      <button class="metric-tab active" data-metric="cpu">CPU</button>
      <button class="metric-tab" data-metric="mem">Memory</button>
      <button class="metric-tab" data-metric="dr">Disk Read</button>
      <button class="metric-tab" data-metric="dw">Disk Write</button>
    </div>
  </div>
  <div class="heatmap-container">
    <div class="skeleton-overlay" id="skeleton">
      <div class="skel-bar"></div>
      <div class="skel-bar"></div>
      <div class="skel-bar"></div>
      <div class="skel-bar"></div>
    </div>
    <canvas id="heatmap"></canvas>
    <div class="tooltip" id="tooltip">
      <div class="tt-time" id="tt-time"></div>
      <div id="tt-body"></div>
    </div>
    <div class="legend">
      <div class="legend-item"><div class="swatch" style="background:hsl(120,80%,50%)"></div>Low</div>
      <div class="legend-item"><div class="swatch" style="background:hsl(60,80%,50%)"></div>Medium</div>
      <div class="legend-item"><div class="swatch" style="background:hsl(30,80%,50%)"></div>High</div>
      <div class="legend-item"><div class="swatch" style="background:hsl(0,80%,50%)"></div>Extreme</div>
      <div class="legend-item"><div class="swatch" style="background:hsl(0,0%,25%)"></div>N/A</div>
    </div>
  </div>
</div>

<div class="section">
  <div class="detail-container" id="detail-container">
    <div class="no-snapshot" id="no-snapshot"><span class="ns-icon">&#128433;</span>Click on the heatmap above to view process details</div>
    <div id="snapshot-content" style="display:none" class="fade-in">
      <div class="snapshot-info">
        <div class="snap-header">
          <div class="clock-icon">
            <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
          </div>
          <div>
            <div class="snap-time" id="snap-time"></div>
          </div>
        </div>
        <div class="sys-stats">
          <div class="stat-card cpu-card"><div class="sc-icon">C</div><div class="sc-body"><div class="sc-label">CPU</div><div class="sc-value" id="snap-cpu"></div></div></div>
          <div class="stat-card mem-card"><div class="sc-icon">M</div><div class="sc-body"><div class="sc-label">Memory</div><div class="sc-value" id="snap-mem"></div></div></div>
          <div class="stat-card dr-card"><div class="sc-icon">R</div><div class="sc-body"><div class="sc-label">Disk Read</div><div class="sc-value" id="snap-dr"></div></div></div>
          <div class="stat-card dw-card"><div class="sc-icon">W</div><div class="sc-body"><div class="sc-label">Disk Write</div><div class="sc-value" id="snap-dw"></div></div></div>
        </div>
      </div>
      <div class="table-wrap">
        <div class="table-scroll">
          <table class="process-table">
            <thead>
              <tr>
                <th data-sort="cpu">CPU% <span class="sort-arrow">&#9660;</span></th>
                <th data-sort="mem">Memory <span class="sort-arrow">&#9650;</span></th>
                <th data-sort="pid">PID <span class="sort-arrow">&#9650;</span></th>
                <th>Name</th>
                <th>Status</th>
              </tr>
            </thead>
            <tbody id="process-tbody"></tbody>
          </table>
        </div>
      </div>
    </div>
    <div class="loading" id="snap-loading" style="display:none"><div class="spinner"></div>Loading...</div>
    <div class="error-box" id="snap-error" style="display:none"></div>
  </div>
</div>
</div>

<script>
(function(){
    // --- Configuration ---
    const API_BASE = '';

    // --- State ---
    let heatmapData = [];
    let currentMetric = 'cpu';
    let lastTimestamp = 0;
    let prevSnapshot = null;
    let sortKey = 'cpu';
    let sortDesc = true;
    let currentSnapshotData = null;
    let dataLoaded = false;
    const canvas = document.getElementById('heatmap');
    const ctx = canvas.getContext('2d');
    const tooltip = document.getElementById('tooltip');

    // --- Metric helpers ---
    function metricValue(d) {
        if (currentMetric === 'cpu') return d.cpu;
        if (currentMetric === 'mem') return d.mem;
        if (currentMetric === 'dr') return d.dr;
        if (currentMetric === 'dw') return d.dw;
        return d.cpu;
    }

    function metricLabel(v) {
        if (v < 0) return 'N/A';
        if (currentMetric === 'dr' || currentMetric === 'dw') {
            if (v < 1024) return v.toFixed(0) + ' B/s';
            if (v < 1048576) return (v/1024).toFixed(1) + ' KB/s';
            return (v/1048576).toFixed(1) + ' MB/s';
        }
        return v.toFixed(1) + '%';
    }

    // --- Color mapping: value -> HSL color (green=low to red=high) ---
    function metricColor(v) {
        if (v < 0) return 'hsl(0,0%,25%)';
        const maxVal = (currentMetric === 'cpu' || currentMetric === 'mem') ? 100 : 104857600;
        const pct = Math.min(v / maxVal, 1.0) * 100;
        const h = Math.round(120 * (1 - pct / 100));
        return 'hsl(' + h + ',75%,48%)';
    }

    function cpuBarClass(v) {
        if (v < 25) return 'low';
        if (v < 50) return 'mid';
        if (v < 75) return 'high';
        return 'extreme';
    }

    // --- Calculate heatmap downsample step based on canvas width ---
    function calculateStep() {
        const w = Math.max(canvas.clientWidth - 60, 100);
        let step = Math.ceil(86400 / w);
        step = Math.ceil(step / 5) * 5;
        return Math.max(step, 1);
    }

    // --- Draw heatmap on canvas ---
    function drawHeatmap() {
        const dpr = window.devicePixelRatio || 1;
        const cw = canvas.clientWidth;
        const ch = canvas.clientHeight;
        canvas.width = cw * dpr;
        canvas.height = ch * dpr;
        ctx.scale(dpr, dpr);

        const ml = 8, mr = 8, mt = 8, mb = 28;
        const dw = cw - ml - mr;
        const dh = ch - mt - mb;

        // Background with subtle gradient
        const bgGrad = ctx.createLinearGradient(0, 0, 0, ch);
        bgGrad.addColorStop(0, '#12122a');
        bgGrad.addColorStop(1, '#0a0a18');
        ctx.fillStyle = bgGrad;
        ctx.fillRect(0, 0, cw, ch);

        // Rounded clip for heatmap area
        ctx.save();
        ctx.beginPath();
        ctx.roundRect(ml, mt, dw, dh, 4);
        ctx.clip();

        ctx.fillStyle = '#0d0d1a';
        ctx.fillRect(ml, mt, dw, dh);

        if (heatmapData.length === 0) {
            ctx.restore();
            ctx.fillStyle = '#5a5a7a';
            ctx.font = '13px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('Waiting for data...', cw / 2, ch / 2);
            return;
        }

        const bw = dw / heatmapData.length;
        for (let i = 0; i < heatmapData.length; i++) {
            const x = ml + i * bw;
            const v = metricValue(heatmapData[i]);
            ctx.fillStyle = metricColor(v);
            ctx.fillRect(x, mt, Math.max(bw + 0.5, 1), dh);
        }
        ctx.restore();

        // Time axis labels
        ctx.fillStyle = '#5a5a7a';
        ctx.font = '10px -apple-system, BlinkMacSystemFont, sans-serif';
        ctx.textAlign = 'center';
        const labelCount = Math.min(24, Math.floor(dw / 70));
        const labelStep = Math.max(1, Math.floor(heatmapData.length / labelCount));
        for (let i = 0; i < heatmapData.length; i += labelStep) {
            const x = ml + i * bw + bw / 2;
            const d = new Date(heatmapData[i].t * 1000);
            const label = String(d.getHours()).padStart(2,'0') + ':' + String(d.getMinutes()).padStart(2,'0');
            ctx.fillText(label, x, ch - 5);
        }
        if (heatmapData.length > 0) {
            const x = ml + (heatmapData.length - 1) * bw + bw / 2;
            ctx.fillStyle = 'var(--accent)';
            ctx.font = 'bold 10px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.fillStyle = '#4ecdc4';
            ctx.fillText('Now', x, ch - 5);
        }
    }

    // --- Tooltip on hover ---
    function showTooltip(e) {
        if (heatmapData.length === 0) { tooltip.style.display = 'none'; return; }
        const rect = canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const cw = canvas.clientWidth;
        const ml = 8, mr = 8;
        const dw = cw - ml - mr;
        const bw = dw / heatmapData.length;
        const idx = Math.floor((mx - ml) / bw);
        if (idx < 0 || idx >= heatmapData.length) { tooltip.style.display = 'none'; return; }
        const d = heatmapData[idx];
        const dt = new Date(d.t * 1000);

        // Build tooltip content with mini bars
        const cpuV = d.cpu >= 0 ? d.cpu : -1;
        const memV = d.mem >= 0 ? d.mem : -1;
        const cpuPct = cpuV >= 0 ? Math.min(cpuV, 100) : 0;
        const memPct = memV >= 0 ? Math.min(memV, 100) : 0;

        let body = '';
        body += '<div class="tt-time">' + dt.toLocaleString() + '</div>';
        body += '<div class="tt-row"><span class="tt-label">CPU</span><span class="tt-val">' + (cpuV >= 0 ? cpuV.toFixed(1) + '%' : 'N/A') + '</span>';
        body += '<div class="tt-bar-wrap"><div class="tt-bar" style="width:' + cpuPct + '%;background:#4ecdc4"></div></div></div>';
        body += '<div class="tt-row"><span class="tt-label">MEM</span><span class="tt-val">' + (memV >= 0 ? memV.toFixed(1) + '%' : 'N/A') + '</span>';
        body += '<div class="tt-bar-wrap"><div class="tt-bar" style="width:' + memPct + '%;background:#ffe66d"></div></div></div>';
        body += '<div class="tt-row"><span class="tt-label">Disk R</span><span class="tt-val">' + metricLabel(d.dr) + '</span></div>';
        body += '<div class="tt-row"><span class="tt-label">Disk W</span><span class="tt-val">' + metricLabel(d.dw) + '</span></div>';

        document.getElementById('tt-time').style.display = 'none';
        document.getElementById('tt-body').innerHTML = body;

        tooltip.style.display = 'block';
        // Position tooltip, keep within viewport
        let tx = e.clientX + 16;
        let ty = e.clientY + 16;
        const tw = tooltip.offsetWidth;
        const th = tooltip.offsetHeight;
        if (tx + tw > window.innerWidth - 10) tx = e.clientX - tw - 16;
        if (ty + th > window.innerHeight - 10) ty = e.clientY - th - 16;
        tooltip.style.left = tx + 'px';
        tooltip.style.top = ty + 'px';
    }

    canvas.addEventListener('mousemove', showTooltip);
    canvas.addEventListener('mouseleave', function() { tooltip.style.display = 'none'; });

    // --- Click on heatmap to load process snapshot ---
    canvas.addEventListener('click', function(e) {
        if (heatmapData.length === 0) return;
        const rect = canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const cw = canvas.clientWidth;
        const ml = 8, mr = 8;
        const dw = cw - ml - mr;
        const bw = dw / heatmapData.length;
        const idx = Math.floor((mx - ml) / bw);
        if (idx < 0 || idx >= heatmapData.length) return;
        const t = heatmapData[idx].t;
        loadSnapshot(t);
    });

    // --- Data fetching functions ---
    async function loadHeatmap() {
        try {
            const step = calculateStep();
            const resp = await fetch(API_BASE + '/api/heatmap?step=' + step);
            if (!resp.ok) return;
            const json = await resp.json();
            heatmapData = json.data || [];
            if (heatmapData.length > 0) {
                lastTimestamp = heatmapData[heatmapData.length - 1].t;
            }
            drawHeatmap();
            if (heatmapData.length > 0) {
                document.getElementById('skeleton').classList.add('hidden');
            }
        } catch(e) {}
    }

    // --- Incremental update: fetch new data since last timestamp ---
    async function incrementalUpdate() {
        if (lastTimestamp === 0) return;
        try {
            const resp = await fetch(API_BASE + '/api/heatmap?from=' + (lastTimestamp + 1) + '&step=1');
            if (!resp.ok) return;
            const json = await resp.json();
            const newData = json.data || [];
            if (newData.length > 0) {
                heatmapData.push(...newData);
                const cutoff = Math.floor(Date.now() / 1000) - 86400;
                heatmapData = heatmapData.filter(d => d.t >= cutoff);
                lastTimestamp = heatmapData[heatmapData.length - 1].t;
                drawHeatmap();
            }
        } catch(e) {}
    }

    // --- Load process snapshot for a given timestamp ---
    async function loadSnapshot(t) {
        const loading = document.getElementById('snap-loading');
        const error = document.getElementById('snap-error');
        const content = document.getElementById('snapshot-content');
        const noSnap = document.getElementById('no-snapshot');
        loading.style.display = 'block';
        error.style.display = 'none';
        content.style.display = 'none';
        noSnap.style.display = 'none';

        try {
            const resp = await fetch(API_BASE + '/api/snapshot?time=' + t);
            if (!resp.ok) {
                const err = await resp.json();
                error.textContent = err.message || 'Failed to load snapshot';
                error.style.display = 'flex';
                loading.style.display = 'none';
                return;
            }
            const json = await resp.json();
            renderSnapshot(json, t);
            loading.style.display = 'none';
        } catch(e) {
            error.textContent = 'Network error';
            error.style.display = 'flex';
            loading.style.display = 'none';
        }
    }

    // --- Render snapshot data into the detail panel ---
    function renderSnapshot(data, reqTime) {
        const content = document.getElementById('snapshot-content');
        content.style.display = 'block';
        content.className = 'fade-in';

        const dt = new Date(data.timestamp * 1000);
        let timeStr = dt.toLocaleString();
        if (data.meta && data.meta.delta_seconds > 0) {
            timeStr += ' <span class="snap-time-sub">(' + data.meta.delta_seconds + 's earlier)</span>';
        }
        document.getElementById('snap-time').innerHTML = timeStr;

        const cpuEl = document.getElementById('snap-cpu');
        const memEl = document.getElementById('snap-mem');
        cpuEl.textContent = data.system.cpu >= 0 ? data.system.cpu.toFixed(1) + '%' : 'N/A';
        cpuEl.style.color = data.system.cpu >= 0 ? (data.system.cpu > 75 ? '#f87171' : data.system.cpu > 50 ? '#ff9f43' : '#4ecdc4') : '';
        memEl.textContent = data.system.mem >= 0 ? data.system.mem.toFixed(1) + '%' : 'N/A';
        memEl.style.color = data.system.mem >= 0 ? (data.system.mem > 80 ? '#f87171' : '#ffe66d') : '';
        document.getElementById('snap-dr').textContent = data.system.disk_read_bps >= 0 ? formatBytes(data.system.disk_read_bps) + '/s' : 'N/A';
        document.getElementById('snap-dw').textContent = data.system.disk_write_bps >= 0 ? formatBytes(data.system.disk_write_bps) + '/s' : 'N/A';

        currentSnapshotData = data.processes;
        renderTable();
    }

    function formatBytes(b) {
        if (b < 1024) return b.toFixed(0) + ' B';
        if (b < 1048576) return (b/1024).toFixed(1) + ' KB';
        return (b/1048576).toFixed(1) + ' MB';
    }

    // --- Render process table with spike highlighting ---
    function renderTable() {
        if (!currentSnapshotData) return;
        const tbody = document.getElementById('process-tbody');
        let procs = [...currentSnapshotData];

        procs.sort((a, b) => {
            if (sortKey === 'cpu') return sortDesc ? b.cpu - a.cpu : a.cpu - b.cpu;
            if (sortKey === 'mem') return sortDesc ? b.mem - a.mem : a.mem - b.mem;
            return sortDesc ? b.pid - a.pid : a.pid - b.pid;
        });

        let html = '';
        for (const p of procs) {
            let spikeClass = '';
            let badge = '';
            if (prevSnapshot) {
                const prev = prevSnapshot.find(pp => pp.pid === p.pid);
                if (prev) {
                    const cpuDelta = p.cpu - prev.cpu;
                    const memDelta = p.mem - prev.mem;
                    if (cpuDelta > 10) { spikeClass = 'spike-cpu'; badge += '<span class="badge cpu-spike">&#9650; +' + cpuDelta.toFixed(1) + '%</span>'; }
                    if (memDelta > 50) { spikeClass = 'spike-mem'; badge += '<span class="badge mem-spike">&#9650; +' + memDelta.toFixed(0) + 'MB</span>'; }
                }
            }

            const barW = Math.min(p.cpu, 100);
            const barClass = cpuBarClass(p.cpu);

            html += '<tr class="' + spikeClass + '">' +
                '<td><div class="cpu-bar-wrap"><span class="cpu-val">' + p.cpu.toFixed(1) + '%</span><div style="flex:1;background:#1e2140;height:6px;border-radius:3px;overflow:hidden"><div class="cpu-bar ' + barClass + '" style="width:' + barW + '%"></div></div></div></td>' +
                '<td>' + p.mem.toFixed(1) + ' MB</td>' +
                '<td class="pid-cell">' + p.pid + '</td>' +
                '<td class="name-cell" title="' + escapeHtml(p.name) + '">' + escapeHtml(p.name) + '</td>' +
                '<td>' + badge + '</td></tr>';
        }
        tbody.innerHTML = html;
    }

    function escapeHtml(s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
    }

    // --- Metric tab switching ---
    document.getElementById('metric-tabs').addEventListener('click', function(e) {
        if (!e.target.classList.contains('metric-tab')) return;
        document.querySelectorAll('.metric-tab').forEach(t => t.classList.remove('active'));
        e.target.classList.add('active');
        currentMetric = e.target.dataset.metric;
        drawHeatmap();
    });

    // --- Table column sorting ---
    document.querySelectorAll('.process-table th[data-sort]').forEach(th => {
        th.addEventListener('click', function() {
            const key = this.dataset.sort;
            if (sortKey === key) { sortDesc = !sortDesc; }
            else { sortKey = key; sortDesc = (key === 'cpu' || key === 'mem'); }
            document.querySelectorAll('.process-table th').forEach(t => t.classList.remove('sorted'));
            this.classList.add('sorted');
            this.querySelector('.sort-arrow').innerHTML = sortDesc ? '&#9660;' : '&#9650;';
            renderTable();
        });
    });

    // --- Poll /api/heatmap for realtime CPU/mem display in header ---
    async function updateRealtime() {
        try {
            if (heatmapData.length > 0) {
                const last = heatmapData[heatmapData.length - 1];
                const cpuEl = document.getElementById('rt-cpu');
                const memEl = document.getElementById('rt-mem');
                if (last.cpu >= 0) {
                    cpuEl.textContent = last.cpu.toFixed(1) + '%';
                    cpuEl.style.color = last.cpu > 75 ? '#f87171' : last.cpu > 50 ? '#ff9f43' : '#4ecdc4';
                } else { cpuEl.textContent = '--'; }
                if (last.mem >= 0) {
                    memEl.textContent = last.mem.toFixed(1) + '%';
                    memEl.style.color = last.mem > 80 ? '#f87171' : '#ffe66d';
                } else { memEl.textContent = '--'; }
            }
        } catch(e) {}
    }

    // --- Initialize: load heatmap, start periodic updates ---
    async function init() {
        await loadHeatmap();
        setInterval(incrementalUpdate, 5000);
        setInterval(updateRealtime, 2000);
        updateRealtime();
    }

    window.addEventListener('resize', function() { drawHeatmap(); });
    init();
})();
</script>
</body>
</html>)rawliteral";
