(function(){
    const API_BASE = '';
    let heatmapData = [];
    let currentMetric = 'cpu';
    let lastTimestamp = 0;
    let prevSnapshot = null;
    let sortKey = 'cpu';
    let sortDesc = true;
    let currentSnapshotData = null;
    let hoveredIdx = -1;
    let selectedIdx = -1;
    let zoomFrom = null;
    let zoomTo = null;
    let isDragging = false;
    let dragStartX = 0;
    let dragCurrentX = 0;
    let hasDragged = false;
    const DRAG_THRESHOLD = 5;
    const CLICK_TOLERANCE = 10;
    const MIN_ZOOM_SECONDS = 30;

    let currentViewMode = 'now';
    let currentOffset = 0;
    let refreshInterval = 0;
    let refreshTimer = null;

    const canvas = document.getElementById('heatmap');
    const ctx = canvas.getContext('2d');
    const tooltip = document.getElementById('tooltip');
    const crosshairV = document.getElementById('crosshair-v');
    const crosshairH = document.getElementById('crosshair-h');
    const zoomBar = document.getElementById('zoom-bar');
    const zoomRangeEl = document.getElementById('zoom-range');
    const timeInput = document.getElementById('time-input');

    function getVisibleData() {
        if (zoomFrom === null || zoomTo === null) return heatmapData;
        return heatmapData.filter(d => d.t >= zoomFrom && d.t <= zoomTo);
    }

    function metricValue(d) {
        if (currentMetric === 'cpu') return d.cpu;
        if (currentMetric === 'mem') return d.mem;
        if (currentMetric === 'dr') return d.dr;
        if (currentMetric === 'dw') return d.dw;
        if (currentMetric === 'nu') return d.nu;
        if (currentMetric === 'nd') return d.nd;
        return d.cpu;
    }

    function metricLabel(v) {
        if (v < 0) return 'N/A';
        if (currentMetric === 'dr' || currentMetric === 'dw' || currentMetric === 'nu' || currentMetric === 'nd') return diskLabel(v);
        return v.toFixed(1) + '%';
    }

    function diskLabel(v) {
        if (v < 0) return 'N/A';
        if (v < 1024) return v.toFixed(0) + ' B/s';
        if (v < 1048576) return (v/1024).toFixed(1) + ' KB/s';
        return (v/1048576).toFixed(2) + ' MB/s';
    }

    function metricColor(v) {
        if (v < 0) return 'hsl(0,0%,22%)';
        const maxVal = (currentMetric === 'cpu' || currentMetric === 'mem') ? 100 : 104857600;
        const pct = Math.min(v / maxVal, 1.0) * 100;
        const h = Math.round(120 * (1 - pct / 100));
        const s = 70 + pct * 0.3;
        const l = 35 + pct * 0.2;
        return 'hsl(' + h + ',' + s + '%,' + l + '%)';
    }

    function cpuBarClass(v) {
        if (v < 25) return 'low';
        if (v < 50) return 'mid';
        if (v < 75) return 'high';
        return 'extreme';
    }

    function calculateStep() {
        const w = Math.max(canvas.clientWidth - 60, 100);
        let step = Math.ceil(86400 / w);
        step = Math.ceil(step / 5) * 5;
        return Math.max(step, 1);
    }

    function findNearestBar(mx) {
        const vd = getVisibleData();
        if (vd.length === 0) return -1;
        const cw = canvas.clientWidth;
        const ml = 10, mr = 10;
        const dw = cw - ml - mr;
        const bw = dw / vd.length;
        const directIdx = Math.floor((mx - ml) / bw);
        if (directIdx >= 0 && directIdx < vd.length) {
            const barCenter = ml + directIdx * bw + bw / 2;
            if (Math.abs(mx - barCenter) <= Math.max(bw / 2, CLICK_TOLERANCE)) return directIdx;
        }
        let bestIdx = 0, bestDist = Infinity;
        for (let i = 0; i < vd.length; i++) {
            const cx = ml + i * bw + bw / 2;
            const dist = Math.abs(mx - cx);
            if (dist < bestDist) { bestDist = dist; bestIdx = i; }
        }
        return bestIdx;
    }

    function formatTimeShort(t) {
        const d = new Date(t * 1000);
        return String(d.getHours()).padStart(2,'0') + ':' + String(d.getMinutes()).padStart(2,'0') + ':' + String(d.getSeconds()).padStart(2,'0');
    }

    function formatDateTime(t) {
        const d = new Date(t * 1000);
        const mo = String(d.getMonth() + 1).padStart(2,'0');
        const da = String(d.getDate()).padStart(2,'0');
        const h = String(d.getHours()).padStart(2,'0');
        const mi = String(d.getMinutes()).padStart(2,'0');
        const s = String(d.getSeconds()).padStart(2,'0');
        return mo + '/' + da + ' ' + h + ':' + mi + ':' + s;
    }

    function timestampToInputVal(t) {
        const d = new Date(t * 1000);
        return d.getFullYear() + '-' + String(d.getMonth()+1).padStart(2,'0') + '-' + String(d.getDate()).padStart(2,'0') +
            'T' + String(d.getHours()).padStart(2,'0') + ':' + String(d.getMinutes()).padStart(2,'0') + ':' + String(d.getSeconds()).padStart(2,'0');
    }

    function inputValToTimestamp(val) {
        return Math.floor(new Date(val).getTime() / 1000);
    }

    function findNearestIdxInData(t) {
        const vd = getVisibleData();
        if (vd.length === 0) return -1;
        let bestIdx = 0, bestDist = Infinity;
        for (let i = 0; i < vd.length; i++) {
            const dist = Math.abs(vd[i].t - t);
            if (dist < bestDist) { bestDist = dist; bestIdx = i; }
        }
        return bestIdx;
    }

    function getCurrentTimestamp() {
        return Math.floor(Date.now() / 1000) - currentOffset;
    }

    let activePresetOffset = 0;

    function setViewMode(mode, offset) {
        currentViewMode = mode;
        currentOffset = offset || 0;
        updatePresetHighlight();
        updateRefreshBtnsState();
        if (currentViewMode === 'now') {
            timeInput.value = '';
        }
    }

    function updatePresetHighlight() {
        document.querySelectorAll('.preset-btn').forEach(function(btn) {
            if (currentViewMode === 'now' && parseInt(btn.dataset.offset) === currentOffset) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });
    }

    function updateRefreshBtnsState() {
        const isNow = (currentViewMode === 'now');
        document.querySelectorAll('.refresh-btn').forEach(function(btn) {
            if (isNow) {
                btn.classList.remove('disabled');
            } else {
                btn.classList.add('disabled');
            }
        });
        if (!isNow) {
            clearRefresh();
        }
    }

    function setRefresh(seconds) {
        clearRefresh();
        if (seconds <= 0 || currentViewMode !== 'now') return;
        refreshInterval = seconds;
        document.querySelectorAll('.refresh-btn').forEach(function(btn) {
            btn.classList.toggle('active', parseInt(btn.dataset.interval) === seconds);
        });
        refreshTimer = setInterval(function() {
            if (currentViewMode !== 'now') { clearRefresh(); return; }
            const t = getCurrentTimestamp();
            const idx = findNearestIdxInData(t);
            selectedIdx = (idx >= 0) ? idx : 0;
            drawHeatmap();
            loadSnapshot(t, true);
        }, seconds * 1000);
    }

    function clearRefresh() {
        refreshInterval = 0;
        if (refreshTimer) { clearInterval(refreshTimer); refreshTimer = null; }
        document.querySelectorAll('.refresh-btn').forEach(function(btn) {
            btn.classList.remove('active');
        });
    }

    function drawHeatmap() {
        const dpr = window.devicePixelRatio || 1;
        const cw = canvas.clientWidth;
        const ch = canvas.clientHeight;
        canvas.width = cw * dpr;
        canvas.height = ch * dpr;
        ctx.scale(dpr, dpr);

        const ml = 10, mr = 10, mt = 10, mb = 30;
        const dw = cw - ml - mr;
        const dh = ch - mt - mb;

        const bgGrad = ctx.createLinearGradient(0, 0, 0, ch);
        bgGrad.addColorStop(0, '#14132a');
        bgGrad.addColorStop(1, '#0a0a18');
        ctx.fillStyle = bgGrad;
        ctx.fillRect(0, 0, cw, ch);

        ctx.save();
        ctx.beginPath();
        ctx.roundRect(ml, mt, dw, dh, 4);
        ctx.clip();

        ctx.fillStyle = '#0d0d1a';
        ctx.fillRect(ml, mt, dw, dh);

        const vd = getVisibleData();

        if (vd.length === 0) {
            ctx.restore();
            ctx.fillStyle = '#5a5a7a';
            ctx.font = '13px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('Waiting for data...', cw / 2, ch / 2);
            return;
        }

        const bw = dw / vd.length;

        if (hoveredIdx >= 0 && hoveredIdx < vd.length) {
            ctx.fillStyle = 'rgba(255,255,255,0.03)';
            ctx.fillRect(ml + hoveredIdx * bw, mt, bw, dh);
        }

        for (let i = 0; i < vd.length; i++) {
            const x = ml + i * bw;
            const v = metricValue(vd[i]);
            ctx.fillStyle = metricColor(v);
            ctx.fillRect(x, mt, Math.max(bw + 0.5, 1), dh);
        }

        if (hoveredIdx >= 0 && hoveredIdx < vd.length) {
            ctx.fillStyle = 'rgba(255,255,255,0.06)';
            ctx.fillRect(ml + hoveredIdx * bw, mt, Math.max(bw + 0.5, 1), dh);
        }

        if (selectedIdx >= 0 && selectedIdx < vd.length) {
            const sx = ml + selectedIdx * bw;
            const sw = Math.max(bw, 2);
            ctx.fillStyle = 'rgba(78,205,196,0.15)';
            ctx.fillRect(sx - 1, mt, sw + 2, dh);
            ctx.strokeStyle = 'rgba(78,205,196,0.8)';
            ctx.lineWidth = Math.max(1, Math.min(bw, 3));
            ctx.beginPath();
            ctx.moveTo(sx + sw / 2, mt);
            ctx.lineTo(sx + sw / 2, mt + dh);
            ctx.stroke();
            ctx.fillStyle = '#4ecdc4';
            ctx.beginPath();
            const triY = mt + dh - 1;
            const triSz = Math.max(4, Math.min(bw * 0.6, 6));
            ctx.moveTo(sx + sw / 2 - triSz, triY + triSz + 2);
            ctx.lineTo(sx + sw / 2 + triSz, triY + triSz + 2);
            ctx.lineTo(sx + sw / 2, triY);
            ctx.closePath();
            ctx.fill();
        }

        if (isDragging && hasDragged) {
            const x1 = Math.max(Math.min(dragStartX, dragCurrentX), ml);
            const x2 = Math.min(Math.max(dragStartX, dragCurrentX), ml + dw);
            if (x2 - x1 > 2) {
                ctx.fillStyle = 'rgba(78,205,196,0.12)';
                ctx.fillRect(x1, mt, x2 - x1, dh);
                ctx.strokeStyle = 'rgba(78,205,196,0.5)';
                ctx.lineWidth = 1;
                ctx.setLineDash([4, 3]);
                ctx.strokeRect(x1, mt, x2 - x1, dh);
                ctx.setLineDash([]);
                ctx.fillStyle = '#4ecdc4';
                ctx.font = 'bold 10px -apple-system, BlinkMacSystemFont, sans-serif';
                ctx.textAlign = 'center';
                const labelY = mt + dh / 2;
                const t1 = vd[Math.max(0, Math.min(Math.floor((x1 - ml) / bw), vd.length - 1))].t;
                const t2 = vd[Math.max(0, Math.min(Math.floor((x2 - ml) / bw), vd.length - 1))].t;
                const dur = t2 - t1;
                let durStr = dur >= 3600 ? (dur / 3600).toFixed(1) + 'h' : dur >= 60 ? Math.ceil(dur / 60) + 'm' : dur + 's';
                ctx.fillText(durStr, (x1 + x2) / 2, labelY);
            }
        }

        const lineGrad = ctx.createLinearGradient(ml, 0, ml + dw, 0);
        lineGrad.addColorStop(0, 'rgba(78,205,196,0)');
        lineGrad.addColorStop(0.5, 'rgba(78,205,196,0.15)');
        lineGrad.addColorStop(1, 'rgba(78,205,196,0)');
        ctx.strokeStyle = lineGrad;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(ml, mt);
        ctx.lineTo(ml + dw, mt);
        ctx.stroke();

        ctx.restore();

        ctx.fillStyle = '#5a5a7a';
        ctx.font = '10px -apple-system, BlinkMacSystemFont, sans-serif';
        ctx.textAlign = 'center';
        const labelCount = Math.min(20, Math.floor(dw / 70));
        const labelStep = Math.max(1, Math.floor(vd.length / labelCount));
        for (let i = 0; i < vd.length; i += labelStep) {
            const x = ml + i * bw + bw / 2;
            ctx.fillText(formatTimeShort(vd[i].t), x, ch - 6);
        }
        if (vd.length > 0) {
            const x = ml + (vd.length - 1) * bw + bw / 2;
            ctx.font = 'bold 10px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.fillStyle = '#4ecdc4';
            ctx.fillText('Now', x, ch - 6);
        }
        if (vd.length > 0 && zoomFrom !== null) {
            ctx.font = '10px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.fillStyle = '#6c6c8a';
            ctx.textAlign = 'left';
            const d0 = new Date(vd[0].t * 1000);
            ctx.fillText((d0.getMonth()+1) + '/' + d0.getDate(), ml, ch - 18);
        }
    }

    function showTooltip(e) {
        const vd = getVisibleData();
        if (vd.length === 0) { tooltip.style.display = 'none'; crosshairV.style.display = 'none'; return; }
        const rect = canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const cw = canvas.clientWidth;
        const ml = 10, mr = 10;
        const dw = cw - ml - mr;
        const bw = dw / vd.length;
        const idx = Math.floor((mx - ml) / bw);
        if (idx < 0 || idx >= vd.length) {
            tooltip.style.display = 'none';
            crosshairV.style.display = 'none';
            hoveredIdx = -1;
            drawHeatmap();
            return;
        }

        hoveredIdx = idx;
        drawHeatmap();

        crosshairV.style.display = 'block';
        const blockX = ml + idx * bw + bw / 2;
        crosshairV.style.left = (rect.left + blockX) + 'px';
        crosshairV.style.top = (rect.top + 10) + 'px';
        crosshairV.style.height = '140px';

        const d = vd[idx];
        const cpuV = d.cpu >= 0 ? d.cpu : -1;
        const memV = d.mem >= 0 ? d.mem : -1;
        const cpuPct = cpuV >= 0 ? Math.min(cpuV, 100) : 0;
        const memPct = memV >= 0 ? Math.min(memV, 100) : 0;
        const cpuColor = cpuV >= 0 ? (cpuV > 75 ? '#f87171' : cpuV > 50 ? '#ff9f43' : '#4ecdc4') : '#6c6c8a';
        const memColor = memV >= 0 ? (memV > 80 ? '#f87171' : '#ffe66d') : '#6c6c8a';

        let html = '<div class="tt-time">' + new Date(d.t * 1000).toLocaleString() + '</div>';
        html += '<div class="tt-row"><span class="tt-label">CPU</span><span class="tt-val" style="color:' + cpuColor + '">' + (cpuV >= 0 ? cpuV.toFixed(1) + '%' : 'N/A') + '</span>';
        html += '<div class="tt-bar-wrap"><div class="tt-bar" style="width:' + cpuPct + '%;background:' + cpuColor + '"></div></div></div>';
        html += '<div class="tt-row"><span class="tt-label">MEM</span><span class="tt-val" style="color:' + memColor + '">' + (memV >= 0 ? memV.toFixed(1) + '%' : 'N/A') + '</span>';
        html += '<div class="tt-bar-wrap"><div class="tt-bar" style="width:' + memPct + '%;background:' + memColor + '"></div></div></div>';
        html += '<div class="tt-row"><span class="tt-label">Disk R</span><span class="tt-val">' + diskLabel(d.dr) + '</span></div>';
        html += '<div class="tt-row"><span class="tt-label">Disk W</span><span class="tt-val">' + diskLabel(d.dw) + '</span></div>';
        html += '<div class="tt-row"><span class="tt-label">Net Up</span><span class="tt-val" style="color:#22d3ee">' + diskLabel(d.nu) + '</span></div>';
        html += '<div class="tt-row"><span class="tt-label">Net Down</span><span class="tt-val" style="color:#60a5fa">' + diskLabel(d.nd) + '</span></div>';

        tooltip.innerHTML = html;
        tooltip.style.display = 'block';
        let tx = e.clientX + 16, ty = e.clientY + 16;
        const tw = tooltip.offsetWidth, th = tooltip.offsetHeight;
        if (tx + tw > window.innerWidth - 12) tx = e.clientX - tw - 16;
        if (ty + th > window.innerHeight - 12) ty = e.clientY - th - 16;
        tooltip.style.left = tx + 'px';
        tooltip.style.top = ty + 'px';
    }

    canvas.addEventListener('mousedown', function(e) {
        if (e.button !== 0) return;
        const rect = canvas.getBoundingClientRect();
        isDragging = true;
        hasDragged = false;
        dragStartX = e.clientX - rect.left;
        dragCurrentX = dragStartX;
    });

    canvas.addEventListener('mousemove', function(e) {
        if (isDragging && hasDragged) {
            const rect = canvas.getBoundingClientRect();
            dragCurrentX = e.clientX - rect.left;
            tooltip.style.display = 'none';
            crosshairV.style.display = 'none';
            hoveredIdx = -1;
            drawHeatmap();
            return;
        }
        if (isDragging) {
            const rect = canvas.getBoundingClientRect();
            const cx = e.clientX - rect.left;
            if (Math.abs(cx - dragStartX) > DRAG_THRESHOLD) {
                hasDragged = true;
                dragCurrentX = cx;
                tooltip.style.display = 'none';
                crosshairV.style.display = 'none';
                hoveredIdx = -1;
                drawHeatmap();
                return;
            }
            return;
        }
        showTooltip(e);
    });

    canvas.addEventListener('mouseup', function(e) {
        if (e.button !== 0) return;
        if (!isDragging) return;
        isDragging = false;

        if (hasDragged) {
            const vd = getVisibleData();
            if (vd.length === 0) { hasDragged = false; drawHeatmap(); return; }
            const cw = canvas.clientWidth;
            const ml = 10, mr = 10;
            const dw = cw - ml - mr;
            const bw = dw / vd.length;

            let x1 = Math.min(dragStartX, dragCurrentX);
            let x2 = Math.max(dragStartX, dragCurrentX);
            x1 = Math.max(x1, ml);
            x2 = Math.min(x2, ml + dw);

            if (x2 - x1 < 5) { hasDragged = false; drawHeatmap(); return; }

            let idx1 = Math.floor((x1 - ml) / bw);
            let idx2 = Math.ceil((x2 - ml) / bw) - 1;
            idx1 = Math.max(0, Math.min(idx1, vd.length - 1));
            idx2 = Math.max(0, Math.min(idx2, vd.length - 1));

            let from = vd[idx1].t;
            let to = vd[idx2].t;
            if (to - from < MIN_ZOOM_SECONDS) {
                const mid = Math.floor((from + to) / 2);
                from = mid - Math.floor(MIN_ZOOM_SECONDS / 2);
                to = mid + Math.ceil(MIN_ZOOM_SECONDS / 2);
            }
            zoomFrom = from;
            zoomTo = to;
            selectedIdx = -1;
            hasDragged = false;
            updateZoomBar();
            drawHeatmap();
            return;
        }

        hasDragged = false;
        const rect = canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const idx = findNearestBar(mx);
        if (idx < 0) return;

        selectedIdx = idx;
        canvas.focus();
        const vd = getVisibleData();
        const t = vd[idx].t;
        setViewMode('fixed');
        timeInput.value = timestampToInputVal(t);
        loadSnapshot(t);
        drawHeatmap();
    });

    canvas.addEventListener('mouseleave', function() {
        if (isDragging) {
            isDragging = false;
            hasDragged = false;
        }
        tooltip.style.display = 'none';
        crosshairV.style.display = 'none';
        hoveredIdx = -1;
        drawHeatmap();
    });

    canvas.addEventListener('wheel', function(e) {
        e.preventDefault();
        const vd = getVisibleData();
        if (vd.length === 0) return;

        const rect = canvas.getBoundingClientRect();
        const mx = e.clientX - rect.left;
        const cw = canvas.clientWidth;
        const ml = 10, mr = 10;
        const dw = cw - ml - mr;
        const bw = dw / vd.length;
        let cursorIdx = Math.floor((mx - ml) / bw);
        cursorIdx = Math.max(0, Math.min(cursorIdx, vd.length - 1));
        const cursorTime = vd[cursorIdx].t;

        const currentFrom = (zoomFrom !== null) ? zoomFrom : vd[0].t;
        const currentTo = (zoomTo !== null) ? zoomTo : vd[vd.length - 1].t;
        let currentRange = currentTo - currentFrom;
        if (currentRange <= 0) currentRange = 86400;

        const factor = e.deltaY > 0 ? 1.5 : 1 / 1.5;
        let newRange = currentRange * factor;

        const fullFrom = heatmapData.length > 0 ? heatmapData[0].t : 0;
        const fullTo = heatmapData.length > 0 ? heatmapData[heatmapData.length - 1].t : 0;
        const fullRange = fullTo - fullFrom;
        if (fullRange <= 0) return;

        if (newRange >= fullRange) {
            zoomFrom = null;
            zoomTo = null;
            selectedIdx = -1;
            updateZoomBar();
            drawHeatmap();
            return;
        }

        if (newRange < MIN_ZOOM_SECONDS) newRange = MIN_ZOOM_SECONDS;

        const ratio = currentRange > 0 ? (cursorTime - currentFrom) / currentRange : 0.5;
        let newFrom = Math.floor(cursorTime - ratio * newRange);
        let newTo = Math.floor(cursorTime + (1 - ratio) * newRange);

        if (newFrom < fullFrom) { newTo += (fullFrom - newFrom); newFrom = fullFrom; }
        if (newTo > fullTo) { newFrom -= (newTo - fullTo); newTo = fullTo; }
        newFrom = Math.max(newFrom, fullFrom);

        zoomFrom = newFrom;
        zoomTo = newTo;
        selectedIdx = -1;
        updateZoomBar();
        drawHeatmap();
    }, { passive: false });

    canvas.addEventListener('keydown', function(e) {
        const vd = getVisibleData();
        if (vd.length === 0) return;

        let handled = true;
        if (e.key === 'ArrowLeft' || e.key === 'ArrowRight') {
            const dir = (e.key === 'ArrowLeft') ? -1 : 1;
            const step = e.shiftKey ? 10 : 1;
            if (selectedIdx < 0) selectedIdx = 0;
            selectedIdx = Math.max(0, Math.min(selectedIdx + dir * step, vd.length - 1));
            const t = vd[selectedIdx].t;
            setViewMode('fixed');
            timeInput.value = timestampToInputVal(t);
            loadSnapshot(t);
            drawHeatmap();
        } else if (e.key === 'Home') {
            selectedIdx = 0;
            const t = vd[0].t;
            setViewMode('fixed');
            timeInput.value = timestampToInputVal(t);
            loadSnapshot(t);
            drawHeatmap();
        } else if (e.key === 'End') {
            selectedIdx = vd.length - 1;
            const t = vd[vd.length - 1].t;
            setViewMode('fixed');
            timeInput.value = timestampToInputVal(t);
            loadSnapshot(t);
            drawHeatmap();
        } else {
            handled = false;
        }
        if (handled) e.preventDefault();
    });

    function updateZoomBar() {
        if (zoomFrom !== null && zoomTo !== null) {
            zoomBar.classList.add('visible');
            zoomRangeEl.textContent = formatDateTime(zoomFrom) + '  —  ' + formatDateTime(zoomTo);
        } else {
            zoomBar.classList.remove('visible');
        }
    }

    document.getElementById('zoom-reset').addEventListener('click', function() {
        zoomFrom = null;
        zoomTo = null;
        selectedIdx = -1;
        updateZoomBar();
        drawHeatmap();
    });

    document.querySelectorAll('.preset-btn').forEach(function(btn) {
        btn.addEventListener('click', function() {
            const offset = parseInt(this.dataset.offset);
            setViewMode('now', offset);
            const t = getCurrentTimestamp();

            if (zoomFrom !== null && zoomTo !== null) {
                if (t < zoomFrom || t > zoomTo) {
                    zoomFrom = null;
                    zoomTo = null;
                    updateZoomBar();
                }
            }

            const idx = findNearestIdxInData(t);
            selectedIdx = (idx >= 0) ? idx : 0;
            canvas.focus();
            loadSnapshot(t);
            drawHeatmap();

            if (refreshInterval > 0) {
                setRefresh(refreshInterval);
            }
        });
    });

    document.querySelectorAll('.refresh-btn').forEach(function(btn) {
        btn.addEventListener('click', function() {
            if (this.classList.contains('disabled')) return;
            const seconds = parseInt(this.dataset.interval);
            if (refreshInterval === seconds) {
                clearRefresh();
                return;
            }
            setRefresh(seconds);
        });
    });

    function handleTimeInput() {
        const val = timeInput.value;
        if (!val) return;
        const t = inputValToTimestamp(val);

        const fullFrom = heatmapData.length > 0 ? heatmapData[0].t : 0;
        const fullTo = heatmapData.length > 0 ? heatmapData[heatmapData.length - 1].t : 0;
        if (t > fullTo && fullTo > 0) {
            timeInput.classList.add('input-error');
            setTimeout(() => timeInput.classList.remove('input-error'), 2000);
            return;
        }

        setViewMode('fixed');

        if (zoomFrom !== null && zoomTo !== null) {
            if (t < zoomFrom || t > zoomTo) {
                zoomFrom = null;
                zoomTo = null;
                updateZoomBar();
            }
        }

        const idx = findNearestIdxInData(t);
        selectedIdx = (idx >= 0) ? idx : 0;
        canvas.focus();
        loadSnapshot(t);
        drawHeatmap();
    }

    document.getElementById('go-btn').addEventListener('click', handleTimeInput);
    timeInput.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') handleTimeInput();
    });

    function updateTimeInputRange() {
        if (heatmapData.length === 0) return;
        timeInput.min = timestampToInputVal(heatmapData[0].t);
        timeInput.max = timestampToInputVal(heatmapData[heatmapData.length - 1].t);
    }

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
            zoomFrom = null;
            zoomTo = null;
            selectedIdx = -1;
            updateZoomBar();
            updateTimeInputRange();
            drawHeatmap();
            if (heatmapData.length > 0) {
                document.getElementById('skeleton').classList.add('hidden');
            }
        } catch(e) {}
    }

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
                updateTimeInputRange();
                drawHeatmap();
            }
        } catch(e) {}
    }

    async function loadSnapshot(t, silent) {
        const loading = document.getElementById('snap-loading');
        const error = document.getElementById('snap-error');
        const content = document.getElementById('snapshot-content');
        const noSnap = document.getElementById('no-snapshot');
        const container = document.getElementById('detail-container');

        if (silent) {
            try {
                const resp = await fetch(API_BASE + '/api/snapshot?time=' + t);
                if (!resp.ok) return;
                const json = await resp.json();
                renderSnapshot(json, t);
            } catch(e) {}
            return;
        }

        const prevHeight = container.offsetHeight;
        if (prevHeight > 200) container.style.minHeight = prevHeight + 'px';

        loading.style.display = 'block';
        error.style.display = 'none';
        content.style.display = 'none';
        noSnap.style.display = 'none';

        try {
            const resp = await fetch(API_BASE + '/api/snapshot?time=' + t);
            if (!resp.ok) {
                let msg = 'Failed to load snapshot';
                try { const err = await resp.json(); msg = err.message || msg; } catch(e2) {}
                document.getElementById('error-text').textContent = msg;
                error.style.display = 'flex';
                loading.style.display = 'none';
                return;
            }
            const json = await resp.json();
            renderSnapshot(json, t);
            loading.style.display = 'none';
        } catch(e) {
            document.getElementById('error-text').textContent = 'Network error';
            error.style.display = 'flex';
            loading.style.display = 'none';
        } finally {
            container.style.minHeight = '';
        }
    }

    function renderSnapshot(data, reqTime) {
        const content = document.getElementById('snapshot-content');
        const noSnap = document.getElementById('no-snapshot');
        content.style.display = 'block';
        noSnap.style.display = 'none';
        content.className = 'fade-in';

        const dt = new Date(data.timestamp * 1000);
        let timeStr = dt.toLocaleString();
        if (data.meta && data.meta.aggregated) {
            const fromStr = formatDateTime(data.meta.aggregated_from);
            timeStr += ' <span class="snap-time-sub">(avg from ' + fromStr + ')</span>';
        } else if (data.meta && data.meta.delta_seconds > 0) {
            timeStr += ' <span class="snap-time-sub">(' + data.meta.delta_seconds + 's earlier)</span>';
        }
        document.getElementById('snap-time').innerHTML = timeStr;

        const cpuEl = document.getElementById('snap-cpu');
        const memEl = document.getElementById('snap-mem');
        const cpuVal = data.system.cpu;
        const memVal = data.system.mem;

        cpuEl.textContent = cpuVal >= 0 ? cpuVal.toFixed(1) + '%' : 'N/A';
        cpuEl.style.color = cpuVal >= 0 ? (cpuVal > 75 ? '#f87171' : cpuVal > 50 ? '#ff9f43' : '#4ecdc4') : '';
        memEl.textContent = memVal >= 0 ? memVal.toFixed(1) + '%' : 'N/A';
        memEl.style.color = memVal >= 0 ? (memVal > 80 ? '#f87171' : '#ffe66d') : '';

        const cpuBar = document.getElementById('snap-cpu-bar');
        const memBar = document.getElementById('snap-mem-bar');
        if (cpuVal >= 0) { cpuBar.style.width = Math.min(cpuVal, 100) + '%'; cpuBar.style.background = cpuVal > 75 ? 'linear-gradient(90deg,#e94560,#f87171)' : cpuVal > 50 ? 'linear-gradient(90deg,#ff9f43,#fb923c)' : 'linear-gradient(90deg,#4ecdc4,#38b2ac)'; }
        else { cpuBar.style.width = '0%'; }
        if (memVal >= 0) { memBar.style.width = Math.min(memVal, 100) + '%'; memBar.style.background = memVal > 80 ? 'linear-gradient(90deg,#e94560,#f87171)' : 'linear-gradient(90deg,#ffe66d,#fbbf24)'; }
        else { memBar.style.width = '0%'; }

        document.getElementById('snap-dr').textContent = data.system.disk_read_bps >= 0 ? formatBytes(data.system.disk_read_bps) + '/s' : 'N/A';
        document.getElementById('snap-dw').textContent = data.system.disk_write_bps >= 0 ? formatBytes(data.system.disk_write_bps) + '/s' : 'N/A';
        document.getElementById('snap-nu').textContent = data.system.net_up_bps >= 0 ? formatBytes(data.system.net_up_bps) + '/s' : 'N/A';
        document.getElementById('snap-nd').textContent = data.system.net_down_bps >= 0 ? formatBytes(data.system.net_down_bps) + '/s' : 'N/A';

        currentSnapshotData = data.processes;
        renderTable();
        prevSnapshot = data.processes.map(p => ({pid: p.pid, name: p.name, cpu: p.cpu, mem: p.mem, ior: p.ior, iow: p.iow}));
    }

    function formatBytes(b) {
        if (b < 1024) return b.toFixed(0) + ' B';
        if (b < 1048576) return (b/1024).toFixed(1) + ' KB';
        return (b/1048576).toFixed(1) + ' MB';
    }

    function renderTable() {
        if (!currentSnapshotData) return;
        const tbody = document.getElementById('process-tbody');
        let procs = [...currentSnapshotData];

        procs.sort((a, b) => {
            if (sortKey === 'cpu') return sortDesc ? b.cpu - a.cpu : a.cpu - b.cpu;
            if (sortKey === 'mem') return sortDesc ? b.mem - a.mem : a.mem - b.mem;
            if (sortKey === 'ior') {
                const ad = a.ior >= 0 ? a.ior : -1e18, bd = b.ior >= 0 ? b.ior : -1e18;
                return sortDesc ? bd - ad : ad - bd;
            }
            if (sortKey === 'iow') {
                const ad = a.iow >= 0 ? a.iow : -1e18, bd = b.iow >= 0 ? b.iow : -1e18;
                return sortDesc ? bd - ad : ad - bd;
            }
            if (sortKey === 'name') return sortDesc ? b.name.localeCompare(a.name) : a.name.localeCompare(b.name);
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
                    if (cpuDelta > 3) { spikeClass = 'spike-cpu'; badge += '<span class="badge cpu-spike">&#9650; +' + cpuDelta.toFixed(1) + '%</span>'; }
                    else if (cpuDelta < -3) { badge += '<span class="badge cpu-spike">&#9660; ' + cpuDelta.toFixed(1) + '%</span>'; }
                    if (memDelta > 10) { if (!spikeClass) spikeClass = 'spike-mem'; badge += '<span class="badge mem-spike">&#9650; +' + memDelta.toFixed(0) + 'MB</span>'; }
                    else if (memDelta < -10) { badge += '<span class="badge mem-spike">&#9660; ' + memDelta.toFixed(0) + 'MB</span>'; }
                }
            }

            const barW = Math.min(p.cpu, 100);
            const barClass = cpuBarClass(p.cpu);

            html += '<tr class="' + spikeClass + '">' +
                '<td><div class="cpu-bar-wrap"><span class="cpu-val">' + p.cpu.toFixed(1) + '%</span><div class="cpu-bar-track"><div class="cpu-bar ' + barClass + '" style="width:' + barW + '%"></div></div></div></td>' +
                '<td>' + p.mem.toFixed(1) + ' MB</td>' +
                '<td class="disk-cell">' + diskLabel(p.ior) + '</td>' +
                '<td class="disk-cell">' + diskLabel(p.iow) + '</td>' +
                '<td class="pid-cell">' + p.pid + '</td>' +
                '<td class="name-cell" title="' + escapeHtml(p.name) + '">' + escapeHtml(p.name) + '</td>' +
                '<td>' + badge + '</td></tr>';
        }
        tbody.innerHTML = html;
    }

    function escapeHtml(s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
    }

    document.getElementById('metric-tabs').addEventListener('click', function(e) {
        if (!e.target.classList.contains('metric-tab')) return;
        document.querySelectorAll('.metric-tab').forEach(t => t.classList.remove('active'));
        e.target.classList.add('active');
        currentMetric = e.target.dataset.metric;
        drawHeatmap();
    });

    document.querySelectorAll('.process-table th[data-sort]').forEach(th => {
        th.addEventListener('click', function() {
            const key = this.dataset.sort;
            if (sortKey === key) { sortDesc = !sortDesc; }
            else { sortKey = key; sortDesc = (key === 'cpu' || key === 'mem' || key === 'ior' || key === 'iow'); }
            document.querySelectorAll('.process-table th').forEach(t => t.classList.remove('sorted'));
            this.classList.add('sorted');
            this.querySelector('.sort-arrow').innerHTML = sortDesc ? '&#9660;' : '&#9650;';
            renderTable();
        });
    });

    async function updateRealtime() {
        try {
            if (heatmapData.length > 0) {
                const last = heatmapData[heatmapData.length - 1];
                const cpuEl = document.getElementById('rt-cpu');
                const memEl = document.getElementById('rt-mem');
                const cpuFill = document.getElementById('rt-cpu-fill');
                const memFill = document.getElementById('rt-mem-fill');
                const drEl = document.getElementById('rt-dr');
                const dwEl = document.getElementById('rt-dw');
                const nuEl = document.getElementById('rt-nu');
                const ndEl = document.getElementById('rt-nd');

                if (last.cpu >= 0) {
                    cpuEl.textContent = last.cpu.toFixed(1) + '%';
                    cpuEl.className = 'rt-value' + (last.cpu > 75 ? ' danger' : last.cpu > 50 ? ' warn' : '');
                    cpuFill.style.width = Math.min(last.cpu, 100) + '%';
                } else { cpuEl.textContent = '--'; cpuFill.style.width = '0%'; }

                if (last.mem >= 0) {
                    memEl.textContent = last.mem.toFixed(1) + '%';
                    memEl.className = 'rt-value' + (last.mem > 80 ? ' danger' : '');
                    memFill.style.width = Math.min(last.mem, 100) + '%';
                } else { memEl.textContent = '--'; memFill.style.width = '0%'; }

                drEl.textContent = (last.dr >= 0) ? formatBytes(last.dr) + '/s' : '--';
                dwEl.textContent = (last.dw >= 0) ? formatBytes(last.dw) + '/s' : '--';
                nuEl.textContent = (last.nu >= 0) ? formatBytes(last.nu) + '/s' : '--';
                ndEl.textContent = (last.nd >= 0) ? formatBytes(last.nd) + '/s' : '--';
            }
        } catch(e) {}
    }

    async function init() {
        await loadHeatmap();
        setInterval(incrementalUpdate, 5000);
        setInterval(updateRealtime, 2000);
        updateRealtime();
        setViewMode('now', 0);
    }

    window.addEventListener('resize', function() { drawHeatmap(); });
    init();
})();
