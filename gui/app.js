/**
 * @file app.js
 * @brief Simulação interativa da Estação Autónoma de Exploração Planetária.
 *
 * Replica fielmente a lógica do sistema C (buffer circular, drones,
 * braços robóticos, toggle de análise, terminação limpa) e apresenta
 * os resultados numa interface gráfica em tempo real.
 */

/* ============================================================
 * CONSTANTS (mirror macros.h)
 * ============================================================ */
const BOARD_CAPACITY = 10;
const NUM_DRONES = 3;
const NUM_ANALYZERS = 2;
const DRONE_DELIVERY_TIME = 5;   // seconds (scaled by speed)
const ANALYSIS_MIN_TIME = 1;
const ANALYSIS_MAX_TIME = 3;

/* ============================================================
 * STARS BACKGROUND
 * ============================================================ */
(function initStars() {
  const canvas = document.getElementById('stars-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  let stars = [];

  function resize() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    stars = Array.from({ length: 120 }, () => ({
      x: Math.random() * canvas.width,
      y: Math.random() * canvas.height,
      r: Math.random() * 1.2 + 0.3,
      a: Math.random(),
      da: (Math.random() - 0.5) * 0.01
    }));
  }

  function draw() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    for (const s of stars) {
      s.a += s.da;
      if (s.a > 1 || s.a < 0.1) s.da *= -1;
      ctx.beginPath();
      ctx.arc(s.x, s.y, s.r, 0, Math.PI * 2);
      ctx.fillStyle = `rgba(255,255,255,${s.a * 0.5})`;
      ctx.fill();
    }
    requestAnimationFrame(draw);
  }

  window.addEventListener('resize', resize);
  resize();
  draw();
})();

/* ============================================================
 * SIMULATION ENGINE
 * ============================================================ */
class PlanetaryStation {
  constructor() {
    // --- Shared data (mirrors shared_data_t) ---
    this.board = new Array(BOARD_CAPACITY).fill(null);
    this.count = 0;
    this.in = 0;
    this.out = 0;
    this.nextSampleId = 1;
    this.analysisActive = true;
    this.terminate = false;

    // --- Statistics ---
    this.totalCreated = 0;
    this.totalDeposited = 0;
    this.totalAnalyzed = 0;
    this.totalWaitFull = 0;
    this.totalWaitEmpty = 0;
    this.startTime = null;
    this.uptimeInterval = null;

    // --- Entity states ---
    this.drones = Array.from({ length: NUM_DRONES }, (_, i) => ({
      id: i + 1,
      status: 'idle',       // idle | collecting | depositing | waiting | done
      statusText: 'Parado',
      progress: 0,
      timer: null,
      currentSampleId: null
    }));

    this.analyzers = Array.from({ length: NUM_ANALYZERS }, (_, i) => ({
      id: i + 1,
      status: 'idle',       // idle | analyzing | waiting | standby | done
      statusText: 'Parado',
      progress: 0,
      timer: null,
      currentSampleId: null,
      analysisTime: 0
    }));

    // --- Speed ---
    this.speed = 5;
    this.running = false;

    // --- Init UI ---
    this._initUI();
    this._bindSpeed();
  }

  /* ----------------------------------------------------------
   * UI INITIALIZATION
   * ---------------------------------------------------------- */
  _initUI() {
    // Render drone cards
    const droneList = document.getElementById('drone-list');
    droneList.innerHTML = this.drones.map(d => `
      <div class="entity-card drone" id="drone-${d.id}">
        <div class="entity-name">🚁 Drone ${d.id}</div>
        <div class="entity-status" id="drone-status-${d.id}">Parado</div>
        <div class="entity-progress">
          <div class="entity-progress-bar" id="drone-progress-${d.id}" style="width:0%"></div>
        </div>
      </div>
    `).join('');

    // Render analyzer cards
    const analyzerList = document.getElementById('analyzer-list');
    analyzerList.innerHTML = this.analyzers.map(a => `
      <div class="entity-card analyzer" id="analyzer-${a.id}">
        <div class="entity-name">🦾 Analisador ${a.id}</div>
        <div class="entity-status" id="analyzer-status-${a.id}">Parado</div>
        <div class="entity-progress">
          <div class="entity-progress-bar" id="analyzer-progress-${a.id}" style="width:0%"></div>
        </div>
      </div>
    `).join('');

    // Render board slots
    const boardGrid = document.getElementById('board-grid');
    boardGrid.innerHTML = Array.from({ length: BOARD_CAPACITY }, (_, i) => `
      <div class="board-slot" id="slot-${i}">
        <span class="slot-index">${i}</span>
        <span class="sample-icon" style="opacity:0.2">🪨</span>
        <span class="sample-id">—</span>
      </div>
    `).join('');

    this._updateBoard();
    this._updateStats();
  }

  _bindSpeed() {
    const slider = document.getElementById('speed-slider');
    const label = document.getElementById('speed-label');
    slider.addEventListener('input', () => {
      this.speed = parseInt(slider.value);
      label.textContent = `${this.speed}×`;
    });
  }

  /* ----------------------------------------------------------
   * LOGGING
   * ---------------------------------------------------------- */
  _log(module, moduleClass, msg, msgClass = 'msg') {
    const logBody = document.getElementById('log-body');
    const now = new Date();
    const ts = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.innerHTML = `<span class="timestamp">[${ts}]</span> <span class="${moduleClass}">[${module}]</span> <span class="${msgClass}">${msg}</span>`;
    logBody.appendChild(entry);
    logBody.scrollTop = logBody.scrollHeight;

    // Keep max 200 entries
    while (logBody.children.length > 200) {
      logBody.removeChild(logBody.firstChild);
    }
  }

  _logMain(msg, cls) { this._log('MAIN', 'module-main', msg, cls); }
  _logDrone(id, msg, cls) { this._log(`DRONE ${id}`, 'module-drone', msg, cls); }
  _logAnalyzer(id, msg, cls) { this._log(`ANALISADOR ${id}`, 'module-analyzer', msg, cls); }

  /* ----------------------------------------------------------
   * TIME HELPERS
   * ---------------------------------------------------------- */
  _realMs(seconds) {
    return (seconds / this.speed) * 1000;
  }

  /* ----------------------------------------------------------
   * BOARD UPDATE
   * ---------------------------------------------------------- */
  _updateBoard() {
    for (let i = 0; i < BOARD_CAPACITY; i++) {
      const slot = document.getElementById(`slot-${i}`);
      const sample = this.board[i];
      if (sample) {
        slot.className = 'board-slot occupied';
        slot.querySelector('.sample-icon').textContent = '🪨';
        slot.querySelector('.sample-icon').style.opacity = '1';
        slot.querySelector('.sample-id').textContent = `#${sample.id}`;
      } else {
        slot.className = 'board-slot';
        slot.querySelector('.sample-icon').textContent = '🪨';
        slot.querySelector('.sample-icon').style.opacity = '0.2';
        slot.querySelector('.sample-id').textContent = '—';
      }

      // Mark in/out pointers
      if (i === this.in) slot.classList.add('pointer-in');
      if (i === this.out && this.count > 0) slot.classList.add('pointer-out');
    }

    // Capacity bar
    const pct = (this.count / BOARD_CAPACITY) * 100;
    document.getElementById('capacity-fill').style.width = `${pct}%`;
    document.getElementById('capacity-pct').textContent = `${Math.round(pct)}%`;
    document.getElementById('board-count-badge').textContent = `${this.count} / ${BOARD_CAPACITY}`;
  }

  /* ----------------------------------------------------------
   * STATS UPDATE
   * ---------------------------------------------------------- */
  _updateStats() {
    document.getElementById('stat-created').textContent = this.totalCreated;
    document.getElementById('stat-deposited').textContent = this.totalDeposited;
    document.getElementById('stat-analyzed').textContent = this.totalAnalyzed;
    document.getElementById('stat-wait-full').textContent = this.totalWaitFull;
    document.getElementById('stat-wait-empty').textContent = this.totalWaitEmpty;
  }

  _updateUptime() {
    if (!this.startTime) return;
    const elapsed = Math.floor((Date.now() - this.startTime) / 1000);
    const m = String(Math.floor(elapsed / 60)).padStart(2, '0');
    const s = String(elapsed % 60).padStart(2, '0');
    document.getElementById('stat-uptime').textContent = `${m}:${s}`;
  }

  /* ----------------------------------------------------------
   * ENTITY UI UPDATE
   * ---------------------------------------------------------- */
  _updateDroneUI(drone) {
    const card = document.getElementById(`drone-${drone.id}`);
    const statusEl = document.getElementById(`drone-status-${drone.id}`);
    const progressEl = document.getElementById(`drone-progress-${drone.id}`);

    card.className = `entity-card drone ${drone.status}`;
    statusEl.textContent = drone.statusText;
    progressEl.style.width = `${drone.progress}%`;

    if (drone.status === 'collecting') card.classList.add('active');
    if (drone.status === 'waiting') card.classList.add('waiting');
  }

  _updateAnalyzerUI(analyzer) {
    const card = document.getElementById(`analyzer-${analyzer.id}`);
    const statusEl = document.getElementById(`analyzer-status-${analyzer.id}`);
    const progressEl = document.getElementById(`analyzer-progress-${analyzer.id}`);

    card.className = `entity-card analyzer ${analyzer.status}`;
    statusEl.textContent = analyzer.statusText;
    progressEl.style.width = `${analyzer.progress}%`;

    if (analyzer.status === 'analyzing') card.classList.add('analyzing');
    if (analyzer.status === 'waiting') card.classList.add('waiting');
    if (analyzer.status === 'standby') card.classList.add('standby');
  }

  _updateAnalysisBadge() {
    const badge = document.getElementById('analysis-badge');
    const text = document.getElementById('analysis-state-text');
    if (this.analysisActive) {
      badge.className = 'analysis-indicator active';
      text.textContent = 'ATIVA';
    } else {
      badge.className = 'analysis-indicator inactive';
      text.textContent = 'DESATIVADA';
    }
  }

  /* ----------------------------------------------------------
   * DRONE LOGIC
   * ---------------------------------------------------------- */
  _startDrone(drone) {
    if (this.terminate) return;

    drone.status = 'collecting';
    drone.statusText = `A recolher amostra... (${DRONE_DELIVERY_TIME}s)`;
    drone.progress = 0;
    this._updateDroneUI(drone);
    this._logDrone(drone.id, 'A recolher amostra no terreno...');

    // Animate progress during collection
    const totalMs = this._realMs(DRONE_DELIVERY_TIME);
    const steps = 20;
    const stepMs = totalMs / steps;
    let step = 0;

    drone.timer = setInterval(() => {
      if (this.terminate) {
        clearInterval(drone.timer);
        return;
      }
      step++;
      drone.progress = (step / steps) * 100;
      this._updateDroneUI(drone);

      if (step >= steps) {
        clearInterval(drone.timer);
        this._droneDeposit(drone);
      }
    }, stepMs);
  }

  _droneDeposit(drone) {
    if (this.terminate) return;

    // Check if board is full
    if (this.count >= BOARD_CAPACITY) {
      drone.status = 'waiting';
      drone.statusText = 'Tabuleiro cheio — à espera...';
      drone.progress = 100;
      this.totalWaitFull++;
      this._updateDroneUI(drone);
      this._updateStats();
      this._logDrone(drone.id, 'Tabuleiro cheio. A aguardar espaço...', 'msg-warn');
      // Will be retried when an analyzer frees a slot
      return;
    }

    // Create sample
    const sample = {
      id: this.nextSampleId++,
      droneId: drone.id,
      createdAt: new Date()
    };

    this.totalCreated++;

    // Deposit in circular buffer
    this.board[this.in] = sample;
    this.in = (this.in + 1) % BOARD_CAPACITY;
    this.count++;
    this.totalDeposited++;

    drone.currentSampleId = sample.id;
    drone.status = 'depositing';
    drone.statusText = `Depositou amostra #${sample.id}`;
    drone.progress = 0;
    this._updateDroneUI(drone);
    this._updateBoard();
    this._updateStats();
    this._logDrone(drone.id, `Amostra ${sample.id} recolhida e depositada. Ocupação: ${this.count}/${BOARD_CAPACITY}.`, 'msg-ok');

    // Wake up waiting analyzers
    this._wakeAnalyzers();

    // Start next collection cycle
    setTimeout(() => this._startDrone(drone), 300);
  }

  /* ----------------------------------------------------------
   * ANALYZER LOGIC
   * ---------------------------------------------------------- */
  _startAnalyzer(analyzer) {
    if (this.terminate) return;
    this._analyzerTryPickup(analyzer);
  }

  _analyzerTryPickup(analyzer) {
    if (this.terminate) return;

    // Check standby
    if (!this.analysisActive) {
      analyzer.status = 'standby';
      analyzer.statusText = 'Sistema de análise em standby';
      analyzer.progress = 0;
      this._updateAnalyzerUI(analyzer);
      // Will be woken up by toggleAnalysis
      return;
    }

    // Check if board is empty
    if (this.count === 0) {
      analyzer.status = 'waiting';
      analyzer.statusText = 'Tabuleiro vazio — à espera...';
      analyzer.progress = 0;
      this.totalWaitEmpty++;
      this._updateAnalyzerUI(analyzer);
      this._updateStats();
      // Will be woken up by drone deposit
      return;
    }

    // Pick up sample from circular buffer
    const sample = this.board[this.out];
    this.board[this.out] = null;
    this.out = (this.out + 1) % BOARD_CAPACITY;
    this.count--;

    analyzer.currentSampleId = sample.id;
    this._updateBoard();
    this._logAnalyzer(analyzer.id, `Amostra ${sample.id} retirada do tabuleiro. Ocupação: ${this.count}/${BOARD_CAPACITY}.`);

    // Wake up waiting drones
    this._wakeDrones();

    // Analyze outside critical section (random 1-3 seconds)
    const analysisTime = Math.floor(Math.random() * (ANALYSIS_MAX_TIME - ANALYSIS_MIN_TIME + 1)) + ANALYSIS_MIN_TIME;
    analyzer.analysisTime = analysisTime;
    analyzer.status = 'analyzing';
    analyzer.statusText = `A analisar amostra #${sample.id} (${analysisTime}s)`;
    analyzer.progress = 0;
    this._updateAnalyzerUI(analyzer);
    this._logAnalyzer(analyzer.id, `A analisar amostra ${sample.id} durante ${analysisTime} segundo(s).`);

    // Animate progress during analysis
    const totalMs = this._realMs(analysisTime);
    const steps = 20;
    const stepMs = totalMs / steps;
    let step = 0;

    analyzer.timer = setInterval(() => {
      if (this.terminate) {
        clearInterval(analyzer.timer);
        return;
      }
      step++;
      analyzer.progress = (step / steps) * 100;
      this._updateAnalyzerUI(analyzer);

      if (step >= steps) {
        clearInterval(analyzer.timer);
        this.totalAnalyzed++;
        this._updateStats();
        this._logAnalyzer(analyzer.id, `Amostra ${sample.id} analisada e descartada.`, 'msg-ok');

        // Try next pickup after small delay
        setTimeout(() => this._analyzerTryPickup(analyzer), 200);
      }
    }, stepMs);
  }

  /* ----------------------------------------------------------
   * WAKE MECHANISMS (simulates cond_broadcast)
   * ---------------------------------------------------------- */
  _wakeDrones() {
    for (const drone of this.drones) {
      if (drone.status === 'waiting') {
        this._droneDeposit(drone);
      }
    }
  }

  _wakeAnalyzers() {
    for (const analyzer of this.analyzers) {
      if (analyzer.status === 'waiting' || analyzer.status === 'standby') {
        if (this.analysisActive && this.count > 0) {
          this._analyzerTryPickup(analyzer);
        }
      }
    }
  }

  /* ----------------------------------------------------------
   * PUBLIC: START / STOP / TOGGLE
   * ---------------------------------------------------------- */
  start() {
    if (this.running) return;
    this.running = true;
    this.terminate = false;

    // Reset state
    this.board = new Array(BOARD_CAPACITY).fill(null);
    this.count = 0;
    this.in = 0;
    this.out = 0;
    this.nextSampleId = 1;
    this.analysisActive = true;
    this.totalCreated = 0;
    this.totalDeposited = 0;
    this.totalAnalyzed = 0;
    this.totalWaitFull = 0;
    this.totalWaitEmpty = 0;
    this.startTime = Date.now();

    // Clear log
    document.getElementById('log-body').innerHTML = '';

    // UI
    document.getElementById('btn-start').disabled = true;
    document.getElementById('btn-toggle').disabled = false;
    document.getElementById('btn-stop').disabled = false;
    const statusBadge = document.getElementById('system-status');
    statusBadge.className = 'status-badge online';
    document.getElementById('status-text').textContent = 'A executar';
    this._updateAnalysisBadge();
    this._updateBoard();
    this._updateStats();

    // Logging startup sequence
    this._logMain('========================================');
    this._logMain('Estação Autónoma de Exploração Planetária');
    this._logMain('========================================');
    this._logMain('Sistema a iniciar...');
    this._logMain('Shared memory criada e inicializada.', 'msg-ok');
    this._logMain('Handlers de sinais instalados.', 'msg-ok');
    this._logMain('  Ctrl-C (SIGINT)  → Terminar sistema');
    this._logMain('  Ctrl-Z (SIGTSTP) → Ativar/Desativar análise');

    // Start uptime counter
    this.uptimeInterval = setInterval(() => this._updateUptime(), 1000);

    // Start drones (staggered)
    setTimeout(() => {
      this._logMain(`Processo de exploração criado.`, 'msg-ok');
      this.drones.forEach((drone, i) => {
        setTimeout(() => {
          this._logMain(`Drone ${drone.id} criado.`, 'msg-ok');
          this._startDrone(drone);
        }, i * 200);
      });
    }, 300);

    // Start analyzers (staggered)
    setTimeout(() => {
      this._logMain(`Processo de análise criado.`, 'msg-ok');
      this.analyzers.forEach((analyzer, i) => {
        setTimeout(() => {
          this._logMain(`Analisador ${analyzer.id} criado.`, 'msg-ok');
          this._startAnalyzer(analyzer);
        }, i * 200);
      });
      this._logMain('Sistema iniciado. A aguardar sinais...');
    }, 600);
  }

  toggleAnalysis() {
    if (!this.running) return;

    this.analysisActive = !this.analysisActive;
    this._updateAnalysisBadge();

    if (this.analysisActive) {
      this._logMain('SIGTSTP recebido. Sistema de análise ATIVADO.', 'msg-ok');
      // Wake analyzers that were in standby
      this._wakeAnalyzers();
    } else {
      this._logMain('SIGTSTP recebido. Sistema de análise DESATIVADO.', 'msg-warn');
      // Analyzers will go to standby on their next cycle
      for (const a of this.analyzers) {
        if (a.status === 'waiting') {
          a.status = 'standby';
          a.statusText = 'Sistema de análise em standby';
          a.progress = 0;
          this._updateAnalyzerUI(a);
          this._logAnalyzer(a.id, 'Sistema de análise em standby.', 'msg-warn');
        }
      }
    }
  }

  stop() {
    if (!this.running) return;

    this._logMain('SIGINT recebido. A terminar sistema...', 'msg-warn');
    this.terminate = true;

    // Clear all timers
    for (const d of this.drones) {
      if (d.timer) clearInterval(d.timer);
      d.status = 'done';
      d.statusText = 'Terminado';
      d.progress = 0;
      this._updateDroneUI(d);
      this._logDrone(d.id, 'Drone terminado.');
    }

    for (const a of this.analyzers) {
      if (a.timer) clearInterval(a.timer);
      a.status = 'done';
      a.statusText = 'Terminado';
      a.progress = 0;
      this._updateAnalyzerUI(a);
      this._logAnalyzer(a.id, 'Analisador terminado.');
    }

    if (this.uptimeInterval) clearInterval(this.uptimeInterval);

    // Final stats
    this._logMain('Todos os processos filhos terminaram.');
    this._logMain('=== Estatísticas Finais ===');
    this._logMain(`Amostras criadas:     ${this.totalCreated}`);
    this._logMain(`Amostras depositadas: ${this.totalDeposited}`);
    this._logMain(`Amostras analisadas:  ${this.totalAnalyzed}`);
    this._logMain(`Esperas (cheio):      ${this.totalWaitFull}`);
    this._logMain(`Esperas (vazio):      ${this.totalWaitEmpty}`);
    this._logMain('===========================');
    this._logMain('Shared memory libertada.');
    this._logMain('Recursos libertados. Fim.');
    this._logMain('========================================');

    // UI
    this.running = false;
    document.getElementById('btn-start').disabled = false;
    document.getElementById('btn-toggle').disabled = true;
    document.getElementById('btn-stop').disabled = true;
    const statusBadge = document.getElementById('system-status');
    statusBadge.className = 'status-badge offline';
    document.getElementById('status-text').textContent = 'Terminado';
  }
}

/* ============================================================
 * INIT
 * ============================================================ */
window.sim = new PlanetaryStation();
