#pragma once

#include <Arduino.h>

static const char WEB_INDEX_HTML[] PROGMEM = R"LAP_INDEX_HTML(
<!DOCTYPE html>
<html lang="pt-BR">

<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="theme-color" content="#101827">
  <meta name="description" content="RaceMetrics LapTimer - Sistema de cronometragem e telemetria por GPS.">
  <title>RaceMetrics | LapTimer</title>
  <link rel="stylesheet" href="style.css">
</head>

<body>

  <header class="header">

    <span id="connection" class="badge">Conectando...</span>

    <a href="index.html" class="logo-link">
      <div class="brand-logo">

        <div class="brand-symbol">
          <span>R</span><span>M</span>
        </div>

        <div class="brand-text">
          <span class="brand-name">RACE<span>METRICS</span></span>
          <span class="brand-subtitle">GPS · TELEMETRY · LAP TIMER</span>
        </div>

      </div>
    </a>

    <nav class="tabs">
      <button data-tab="dashboard" class="active">Painel</button>
      <button data-tab="tracks">Pistas</button>
      <button data-tab="sessions">Telemetria</button>
      <button data-tab="settings">Configurações</button>
    </nav>

  </header>

  <main>

    <!-- PAINEL -->
    <section id="dashboard" class="page active">

      <h1>Sobre o projeto</h1>

      <div class="card projeto">

        <h2>RaceMetrics | LapTimer</h2>

        <p>
          O RaceMetrics é um protótipo de cronometragem e coleta de dados
          desenvolvido para registrar o desempenho de um veículo durante
          um percurso, utilizando um módulo GPS conectado ao ESP32.
        </p>

        <p>
          O funcionamento utiliza uma linha de partida e chegada definida
          por dois pontos GPS, identificados como A e B. O veículo percorre
          o circuito em um único sentido e, ao cruzar novamente essa linha,
          o sistema identifica automaticamente a conclusão de uma volta.
        </p>

        <p>
          Além da contagem das voltas, o equipamento permite registrar
          informações de telemetria, como velocidade e aceleração,
          possibilitando uma análise mais detalhada do desempenho durante
          o percurso.
        </p>

        <h2>Como funciona a análise dos dados?</h2>

        <p>
          Durante a utilização do equipamento, o ESP32 registra as
          informações de cada sessão e armazena os dados em um arquivo
          CSV. Esse arquivo pode ser baixado posteriormente pela aba
          <strong>Telemetria</strong>.
        </p>

        <p>
          Depois de baixar o arquivo, é possível utilizar o analisador
          desenvolvido em Python para visualizar e interpretar os dados
          coletados, facilitando a comparação das voltas e a análise
          do comportamento do veículo.
        </p>

        <h3>Etapas para realizar a análise</h3>

        <ol>
          <li>Conecte seu dispositivo à rede Wi-Fi criada pelo ESP32.</li>
          <li>Acesse a aba <strong>Telemetria</strong> e localize a sessão desejada.</li>
          <li>Faça o download do arquivo CSV da sessão.</li>
          <li>Acesse o analisador de telemetria pelo botão abaixo.</li>
          <li>Envie o arquivo CSV baixado para realizar a análise.</li>
        </ol>

        <div class="analysis-access">

          <h3>Analisador de telemetria</h3>

          <p>
            Acesse a aplicação desenvolvida em Python para enviar o CSV
            e visualizar os dados coletados pelo equipamento.
          </p>

          <a
            href="https://projeto-iot---andr-t-e-gabriel-d-jemy2waxpnd4kytevwrigk.streamlit.app/"
            target="_blank"
            rel="noopener noreferrer"
            class="analysis-button">
            https://projeto-iot---andr-t-e-gabriel-d-jemy2waxpnd4kytevwrigk.streamlit.app/
          </a>

          <p class="muted">
            O envio do CSV é realizado manualmente após o download.
            Para acessar o analisador online, é necessário ter conexão
            com a internet.
          </p>

        </div>

      </div>

      <div class="card">
        <h2>Telemetria ao vivo</h2>
        <div class="param-boxes">
          <div class="param-row"><span>Velocidade</span><strong id="speed">?</strong></div>
          <div class="param-row"><span>Roll / Pitch</span><strong><span id="roll">?</span>? / <span id="pitch">?</span>?</strong></div>
          <div class="param-row"><span>Volta</span><strong id="lap">?</strong></div>
          <div class="param-row"><span>Tempo da volta</span><strong id="lapTime">?</strong></div>
          <div class="param-row"><span>Sat?lites / HDOP</span><strong><span id="satellites">?</span> / <span id="hdop">?</span></strong></div>
        </div>
      </div>

      <!-- IDs mantidos ocultos para compatibilidade com o script.js -->
      <div class="hidden" aria-hidden="true">
        <span id="device">—</span>
        <span id="gps">—</span>
        <span id="selected">—</span>
        <span id="count">—</span>
      </div>

    </section>


    <!-- CADASTRO DE PISTAS -->
    <section id="tracks" class="page">

      <div class="row">

        <h1>Cadastro de pistas</h1>

        <button id="newTrack">
          + Nova pista
        </button>

      </div>

      <p class="muted">
        Cadastre os circuitos que serão utilizados durante as sessões.
        A linha de partida e chegada é definida por dois pontos GPS (A e B).
        O cruzamento dessa linha permite detectar a conclusão de uma volta.
      </p>

      <div id="trackList" class="list"></div>

      <form id="trackForm" class="card hidden">

        <h2 id="formTitle">Nova pista</h2>

        <input type="hidden" id="trackId">

        <label>
          Nome da pista
          <input
            id="trackName"
            maxlength="70"
            required
            placeholder="Ex.: Kartódromo Municipal">
        </label>

        <h3>Coordenadas da linha de chegada</h3>

        <div class="grid two">

          <label>
            Latitude A
            <input
              id="latA"
              type="text"
              inputmode="text"
              autocomplete="off"
              required>
          </label>

          <label>
            Longitude A
            <input
              id="lonA"
              type="text"
              inputmode="text"
              autocomplete="off"
              required>
          </label>

          <label>
            Latitude B
            <input
              id="latB"
              type="text"
              inputmode="text"
              autocomplete="off"
              required>
          </label>

          <label>
            Longitude B
            <input
              id="lonB"
              type="text"
              inputmode="text"
              autocomplete="off"
              required>
          </label>

        </div>

        <div class="row">

          <button type="submit">
            Salvar pista
          </button>

          <button
            type="button"
            class="secondary"
            id="cancelTrack">
            Cancelar
          </button>

        </div>

      </form>

    </section>


    <!-- TELEMETRIA -->
    <section id="sessions" class="page">

      <h1>Sessões de telemetria</h1>

      <div class="card projeto">

        <h2>Dados das sessões registrados</h2>

        <p>
          As sessões ficam armazenadas no equipamento e podem ser
          baixadas em formato CSV para análise posterior.
        </p>

      </div>

      <div class="row">

        <h2>Histórico de sessões</h2>

        <button id="refreshSessions" class="secondary">
          ↻ Atualizar
        </button>

      </div>

      <p class="muted">
        Consulte as sessões registradas pelo ESP32 e selecione
        uma delas para baixar os dados correspondentes.
      </p>

      <div id="sessionList" class="list"></div>

    </section>


    <!-- CONFIGURAÇÕES -->
    <section id="settings" class="page">

      <h1>Configurações</h1>

      <form id="sessionForm" class="card">
        <h2>Controle da sessão</h2>
        <p class="muted">Selecione a pista e inicie o treino. A gravação começa com GPS válido e continua até você parar, mesmo com o veículo parado.</p>
        <p id="recordingStatus" role="status" aria-live="polite">Consultando sessão...</p>

        <label>
          Pista da sessão
          <select id="activeTrack"></select>
        </label>

        <label>
          Intervalo mínimo entre voltas (segundos)
          <input
            id="minLap"
            type="number"
            min="1"
            max="3600"
            required>
        </label>

        <div class="actions">
          <button id="startSession" type="submit" disabled>Iniciar sessão</button>
          <button id="stopSession" type="button" class="secondary" disabled>Parar sessão</button>
        </div>
      </form>

      <form id="settingsForm" class="card">
        <h2>Equipamento e Wi-Fi</h2>

        <label>
          Nome do dispositivo
          <input
            id="deviceName"
            maxlength="40"
            required>
        </label>

        <label>
          Senha do dispositivo
          <input
            id="devicePassword"
            type="password"
            maxlength="40"
            minlength="8"
            placeholder="Opcional; mínimo de 8 caracteres"
          >
</label>
        <p class="muted">A senha será aplicada ao Wi-Fi quando ele for reativado.</p>

        <button type="submit">
          Salvar configurações
        </button>

      </form>

    </section>


    <!-- MENSAGENS DO SISTEMA -->
    <p id="message" role="status"></p>

  </main>

  <script src="script.js"></script>

</body>
</html>
)LAP_INDEX_HTML";

static const char WEB_APP_JS[] PROGMEM = R"LAP_APP_JS(
const DEMO = false;

const demoTracks = [
  {
    id: 'interlagos',
    name: 'Interlagos',
    latA: -23.7031,
    lonA: -46.6991,
    latB: -23.7032,
    lonB: -46.6993,
  },
];

let demoSettings = {
  activeTrack: 'interlagos',
  minLapSeconds: 15,
  deviceName: 'RaceMetrics ESP32',
};

const demoSessions = [
  {
    id: 'sessao_001.csv',
    name: 'Treino livre #01',
    track: 'Interlagos',
    date: '31/10/2026 09:45',
    size: 18420,
  },
  {
    id: 'sessao_002.csv',
    name: 'Treino livre #02',
    track: 'Interlagos',
    date: '31/10/2026 10:12',
    size: 22650,
  },
];

const $ = (id) => document.getElementById(id);

let tracks = [];
let sessions = [];
let settings = {};
let sessionBusy = false;
let latestStatus = null;

const safe = (value) =>
  String(value ?? '').replace(/[&<>"']/g, (character) => ({
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#39;',
  }[character]));

function msg(message) {
  const element = $('message');
  element.textContent = message;
  element.style.display = 'block';

  setTimeout(() => {
    element.style.display = 'none';
  }, 3500);
}

async function api(path, options) {
  if (DEMO) {
    const method = options?.method || 'GET';
    const body = options?.body ? JSON.parse(options.body) : {};

    if (path === '/status') {
      return {
        device: demoSettings.deviceName,
        gpsFix: true,
      };
    }

    if (path === '/tracks' && method === 'GET') {
      return demoTracks;
    }

    if (path === '/sessions') {
      return demoSessions;
    }

    if (path === '/settings' && method === 'GET') {
      return demoSettings;
    }

    if (path === '/settings') {
      demoSettings = { ...body };
      return demoSettings;
    }

    if (path === '/tracks' && method === 'POST') {
      demoTracks.push({
        ...body,
        id: 'track_' + Date.now(),
      });
      return body;
    }

    if (path.startsWith('/tracks/')) {
      const id = decodeURIComponent(path.split('/').pop());
      const index = demoTracks.findIndex((track) => track.id === id);

      if (method === 'DELETE' && index >= 0) {
        demoTracks.splice(index, 1);
      }

      if (method === 'PUT' && index >= 0) {
        demoTracks[index] = { ...body, id };
      }

      return { ok: true };
    }

    return {};
  }

  const response = await fetch('/api' + path, options);

  if (!response.ok) {
    const text = await response.text();
    let message = text;
    try { message = JSON.parse(text).error || text; } catch (_) {}
    throw Error(message || 'HTTP ' + response.status);
  }

  return response.json();
}

async function load() {
  try {
    const data = await Promise.all([
      api('/status'),
      api('/tracks'),
      api('/sessions'),
      api('/settings'),
    ]);

    const [status, trackList, sessionList, currentSettings] = data;

    tracks = trackList;
    sessions = sessionList;
    settings = currentSettings;

    $('connection').textContent = '● Conectado';
    $('device').textContent = status.device || 'ESP32';
    $('gps').textContent = status.gpsFix ? 'Fix GPS' : 'Sem fix';
    if (status.storageReady === false) {
      msg('Wi-Fi ativo, mas o armazenamento está indisponível; pistas e logs não serão salvos.');
    }
    renderStatus(status);

    $('selected').textContent =
      tracks.find((track) => track.id === settings.activeTrack)?.name || 'Nenhuma';

    $('count').textContent = sessions.length;

    renderTracks();
    renderSessions();
    renderSettings();

  } catch (error) {
    $('connection').textContent = '● Desconectado';
    msg('Falha de comunicação: ' + error.message);
  }
}

function renderStatus(status) {
  latestStatus = status;
  renderRecordingControls();
  $('speed').textContent = `${Number(status.speedKmh || 0).toFixed(1)} km/h`;
  $('roll').textContent = Number(status.roll || 0).toFixed(1);
  $('pitch').textContent = Number(status.pitch || 0).toFixed(1);
  $('lap').textContent = status.lap || 0;
  $('lapTime').textContent = `${Number(status.lapTimeSeconds || 0).toFixed(2)} s`;
  $('satellites').textContent = status.satellites ?? 0;
  $('hdop').textContent = Number(status.hdop || 0).toFixed(2);
}

function renderRecordingControls() {
  const status = latestStatus;
  const active = Boolean(status?.recordingRequested || status?.sessionId);
  const unavailable = !status || status.storageReady === false;
  const labels = {
    idle: 'Parada — nenhuma amostra está sendo gravada.',
    waiting_gps: 'Aguardando GPS válido para começar a gravação.',
    recording: 'Gravando sessão',
    gps_lost: 'Sessão aberta — aguardando o retorno do GPS.',
    error: 'Falha na gravação. Pare a sessão antes de tentar novamente.',
    stop_error: 'Gravação parada, mas a finalização falhou. Tente Parar novamente.',
    unavailable: 'Armazenamento indisponível.',
  };
  $('recordingStatus').textContent = sessionBusy ? 'Processando...'
    : !status ? 'Sem comunicação com o equipamento.'
    : (labels[status.sessionState] || 'Consultando sessão...') +
      (status.sessionId ? ` (${status.sessionId})` : '');
  $('startSession').disabled = sessionBusy || unavailable || active;
  $('stopSession').disabled = sessionBusy || unavailable || !active;
  $('activeTrack').disabled = sessionBusy || unavailable || active;
  $('minLap').disabled = sessionBusy || unavailable || active;
}

async function refreshStatus() {
  if (sessionBusy) return;
  try {
    const status = await api('/status');
    if (sessionBusy) return;
    $('connection').textContent = '● Conectado';
    $('device').textContent = status.device || 'ESP32';
    $('gps').textContent = status.gpsFix ? 'Fix GPS' : 'Sem fix';
    renderStatus(status);
  } catch (error) {
    $('connection').textContent = '● Desconectado';
    latestStatus = null;
    renderRecordingControls();
  }
}

function renderTracks() {
  $('trackList').innerHTML = tracks.length
    ? tracks.map((track) => `
        <div class="item">
          <div>
            <strong>${safe(track.name)}</strong>
            <p>A: ${track.latA}, ${track.lonA} · B: ${track.latB}, ${track.lonB}</p>
          </div>

          <div class="actions">
            <button class="secondary" data-edit="${safe(track.id)}">Editar</button>
            <button class="secondary" data-delete="${safe(track.id)}">Excluir</button>
          </div>
        </div>
      `).join('')
    : '<p class="muted">Nenhuma pista cadastrada.</p>';
}

function renderSessions() {
  $('sessionList').innerHTML = sessions.length
    ? sessions.map((session) => `
        <div class="item">
          <div>
            <strong>${safe(session.name || session.id)}</strong>
            <p>
              ${safe(session.track || 'Pista desconhecida')} ·
              ${safe(session.date || 'Sem data')} ·
              ${Number(session.size || 0)} bytes
            </p>
          </div>

          <a href="/api/session?id=${encodeURIComponent(session.id)}" download>
            <button type="button">Telemetria CSV</button>
          </a>
          <a href="/api/session?id=${encodeURIComponent(session.id)}&file=metadata.json" download>
            <button type="button">Metadados JSON</button>
          </a>
          ${session.hasLaps ? `
            <a href="/api/session?id=${encodeURIComponent(session.id)}&file=laps.json" download>
              <button type="button">Voltas JSON</button>
            </a>
          ` : ''}
        </div>
      `).join('')
    : '<p class="muted">Nenhuma sessão registrada.</p>';
}

function renderSettings() {
  $('activeTrack').innerHTML =
    '<option value="">Sem pista (somente telemetria)</option>' +
    tracks.map((track) =>
      `<option value="${safe(track.id)}">${safe(track.name)}</option>`
    ).join('');

  $('activeTrack').value = settings.activeTrack || '';
  $('minLap').value = settings.minLapSeconds ?? 15;
  $('deviceName').value = settings.deviceName || 'RaceMetrics';
  $('devicePassword').value = '';
}

function openTrack(track) {
  $('trackForm').classList.remove('hidden');
  $('formTitle').textContent = track ? 'Editar pista' : 'Nova pista';

  $('trackId').value = track?.id ?? '';
  $('trackName').value = track?.name ?? '';
  $('latA').value = track?.latA ?? '';
  $('lonA').value = track?.lonA ?? '';
  $('latB').value = track?.latB ?? '';
  $('lonB').value = track?.lonB ?? '';

  $('trackForm').scrollIntoView({ behavior: 'smooth' });
}

document.querySelectorAll('[data-tab]').forEach((button) => {
  button.addEventListener('click', () => {
    document.querySelectorAll('[data-tab]').forEach((tabButton) => {
      tabButton.classList.toggle('active', tabButton === button);
    });

    document.querySelectorAll('.page').forEach((page) => {
      page.classList.toggle('active', page.id === button.dataset.tab);
    });
  });
});

$('newTrack').addEventListener('click', () => openTrack(null));

$('cancelTrack').addEventListener('click', () => {
  $('trackForm').classList.add('hidden');
});

$('trackList').addEventListener('click', async (event) => {
  const button = event.target.closest('button');

  if (!button) return;

  const id = button.dataset.edit || button.dataset.delete;

  if (button.dataset.edit) {
    openTrack(tracks.find((track) => track.id === id));
    return;
  }

  if (button.dataset.delete && confirm('Excluir esta pista?')) {
    try {
      await api('/tracks/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ id }).toString(),
      });

      await load();
      msg('Pista excluída');
    } catch (error) {
      msg(error.message);
    }
  }
});

$('trackForm').addEventListener('submit', async (event) => {
  event.preventDefault();

  const id = $('trackId').value;

  const track = {
    name: $('trackName').value.trim(),
  };

  for (const key of ['latA', 'lonA', 'latB', 'lonB']) {
    track[key] = Number($(key).value);
  }

  if (!track.name || (track.latA === track.latB && track.lonA === track.lonB)) {
    return msg('Informe um nome e dois pontos distintos.');
  }

  try {
    await api(id ? '/tracks/update' : '/tracks', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ ...track, id }).toString(),
    });

    $('trackForm').classList.add('hidden');

    await load();
    msg('Pista salva');

  } catch (error) {
    msg(error.message);
  }
});

async function controlSession(action) {
  if (sessionBusy) return;
  sessionBusy = true;
  renderRecordingControls();
  try {
    const payload = action === 'start' ? {
      activeTrack: $('activeTrack').value,
      minLapSeconds: Number($('minLap').value),
    } : {};
    const status = await api('/recording/' + action, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams(payload).toString(),
    });
    renderStatus(status);
    await load();
    msg(action === 'start' ? 'Início solicitado. A gravação aguarda GPS válido.' : 'Sessão parada.');
  } catch (error) {
    msg(error.message);
  } finally {
    sessionBusy = false;
    renderRecordingControls();
    await refreshStatus();
  }
}

$('sessionForm').addEventListener('submit', async (event) => {
  event.preventDefault();
  await controlSession('start');
});
$('stopSession').addEventListener('click', () => controlSession('stop'));

$('settingsForm').addEventListener('submit', async (event) => {
  event.preventDefault();

  try {
    const settingsPayload = {
      deviceName: $('deviceName').value.trim(),
    };
    const password = $('devicePassword').value;
    if (password) settingsPayload.devicePassword = password;

    await api('/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams(settingsPayload).toString(),
    });

    await load();
    msg(password
      ? 'Configurações salvas; a senha será aplicada ao reativar o Wi-Fi.'
      : 'Configurações salvas');

  } catch (error) {
    msg(error.message);
  }
});

$('refreshSessions').addEventListener('click', load);

load();
setInterval(refreshStatus, 1000);

if (DEMO) {
  document.querySelector('#sessions .muted').textContent =
    'Visualização demonstrativa: os arquivos reais serão fornecidos pelo ESP32.';

  $('sessionList').addEventListener('click', (event) => {
    const link = event.target.closest('a');

    if (!link) return;

    event.preventDefault();

    const csv =
      'timestamp,latitude,longitude,speed_kmh,lap\n' +
      '0,-23.7031,-46.6991,78,1\n' +
      '1000,-23.7032,-46.6992,82,1\n';

    const params = new URL(link.href).searchParams;
    const filename = params.get('file') || 'telemetry.csv';
    const session = sessions.find((item) => item.id === params.get('id'));
    const content = filename === 'metadata.json'
      ? JSON.stringify({ ...session, demo: true }, null, 2)
      : filename === 'laps.json' ? '[]' : csv;
    const url = URL.createObjectURL(new Blob([content], {
      type: filename === 'telemetry.csv' ? 'text/csv' : 'application/json',
    }));

    const downloadLink = document.createElement('a');
    downloadLink.href = url;
    downloadLink.download = `${params.get('id')}_${filename}`;

    downloadLink.click();

    setTimeout(() => URL.revokeObjectURL(url), 1000);
  });
}
)LAP_APP_JS";

static const char WEB_STYLE_CSS[] PROGMEM = R"LAP_STYLE_CSS(
:root {
  --bg: #f8fafc;
  --header: #0f172a;
  --card: #ffffff;
  --border: #e2e8f0;
  --primary: #2563eb;
  --primary-hover: #1d4ed8;
  --secondary: #64748b;
  --text: #1e293b;
  --muted: #64748b;
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
    Roboto, sans-serif;
  color: var(--text);
  background: var(--bg);
}

/* Cabeçalho e logo */

.header {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24px;
  padding: 35px 20px 20px;
  background: var(--header);
  color: white;
}

/* LOGO RACEMETRICS */

.logo-link {
  display: inline-block;
  text-decoration: none;
  color: white;
}

.brand-logo {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16px;
}

.brand-symbol {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 76px;
  height: 66px;
  border: 2px solid #39FF14;
  border-radius: 10px;
  background: linear-gradient(145deg, #1e293b, #0f172a);
  box-shadow: 0 0 18px rgba(255, 204, 0, 0.12);
  transform: skew(-7deg);
}

.brand-symbol span {
  color: #39FF14;
  font-size: 35px;
  font-weight: 900;
  letter-spacing: -5px;
  transform: skew(7deg);
}

.brand-text {
  display: flex;
  flex-direction: column;
  text-align: left;
  gap: 5px;
}

.brand-name {
  color: white;
  font-size: 30px;
  font-weight: 900;
  letter-spacing: 2px;
  line-height: 1;
}

.brand-name span {
  color: #39FF14;
}

.brand-subtitle {
  color: #94a3b8;
  font-size: 10px;
  font-weight: bold;
  letter-spacing: 3px;
}

@media (max-width: 600px) {
  .brand-logo {
    gap: 11px;
  }

  .brand-symbol {
    width: 60px;
    height: 55px;
  }

  .brand-symbol span {
    font-size: 28px;
  }

  .brand-name {
    font-size: 23px;
  }

  .brand-subtitle {
    font-size: 8px;
    letter-spacing: 2px;
  }
}

.badge {
  position: absolute;
  top: 20px;
  right: 25px;
  padding: 7px 12px;
  border: 1px solid #475569;
  border-radius: 30px;
  color: white;
  font-size: 12px;
}

/* Menu */

.tabs {
  display: flex;
  justify-content: center;
  align-items: center;
  flex-wrap: wrap;
  gap: 24px;
}

.tabs button {
  padding: 8px 4px;
  border: 0;
  border-radius: 0;
  background: transparent;
  color: white;
  font-weight: bold;
  cursor: pointer;
  white-space: nowrap;
}

.tabs button:hover,
.tabs button.active {
  color: #39FF14;
}

/* Botões */

button {
  padding: 11px 16px;
  border: 0;
  border-radius: 6px;
  background: var(--primary);
  color: white;
  font-weight: bold;
  cursor: pointer;
}

button:hover {
  background: var(--primary-hover);
}

.tabs button:hover,
.tabs button.active {
  background: transparent;
}

.secondary {
  background: var(--secondary);
}

/* Conteúdo principal */

main {
  max-width: 1100px;
  margin: 28px auto;
  padding: 0 20px;
}

h1 {
  font-size: 26px;
  color: var(--text);
}

h2 {
  margin: 10px 0;
  font-size: 19px;
}

.page {
  display: none;
}

.page.active {
  display: block;
}

/* Cartões e organização */

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.card,
.item {
  margin-bottom: 14px;
  padding: 1.5rem;
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 8px;
}

.card label,
.muted {
  color: var(--muted);
}

.card h2 {
  color: var(--text);
}

.row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
}

.list {
  margin-top: 20px;
}

.item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.item p {
  margin: 7px 0;
  color: var(--muted);
  font-size: 13px;
}

.actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

/* Campos */

label {
  display: block;
  margin: 12px 0;
  font-size: 14px;
}

input,
select {
  display: block;
  width: 100%;
  margin-top: 7px;
  padding: 12px;
  border: 1px solid var(--border);
  border-radius: 6px;
  background: white;
  color: var(--text);
  font-size: 15px;
}

input:focus,
select:focus {
  outline: 2px solid #bfdbfe;
  border-color: var(--primary);
}

form button {
  margin-top: 12px;
}

button:disabled,
input:disabled,
select:disabled {
  opacity: 0.55;
  cursor: not-allowed;
}

#recordingStatus {
  padding: 12px;
  border-left: 3px solid var(--primary);
  background: var(--bg);
}

.hidden {
  display: none !important;
}

/* Mensagens */

#message {
  position: sticky;
  bottom: 12px;
  display: none;
  max-width: 600px;
  margin: 20px auto;
  padding: 12px;
  border-radius: 8px;
  background: var(--header);
  color: white;
}

/* Formulário de pistas */

.two {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

/* Tablets e celulares */

@media (max-width: 600px) {
  .header {
    padding: 25px 12px 18px;
    gap: 18px;
  }

  .logo-link img {
    width: 300px;
  }

  .badge {
    position: static;
  }

  .tabs {
    gap: 14px;
  }

  .tabs button {
    font-size: 13px;
  }

  main {
    margin-top: 18px;
    padding: 0 14px;
  }

  h1 {
    font-size: 22px;
  }

  .grid,
  .two {
    grid-template-columns: 1fr;
  }
}


.projeto {
  line-height: 1.7;
  margin-bottom: 25px;
}

.projeto p {
  color: #475569;
  text-align: justify;
}
)LAP_STYLE_CSS";
