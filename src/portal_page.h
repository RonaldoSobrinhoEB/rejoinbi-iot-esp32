#pragma once

static const char CONFIG_PORTAL_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#171717">
  <link rel="icon" type="image/png" href="/icon.png">
  <title>Rejoin BI Monitor</title>
  <style>
    :root {
      color-scheme: light;
      --ink: oklch(20% 0.008 260);
      --muted: oklch(50% 0.012 260);
      --line: oklch(88% 0.008 260);
      --surface: oklch(99% 0.004 260);
      --soft: oklch(96% 0.006 260);
      --brand: oklch(22% 0.01 260);
      --success: oklch(55% 0.16 150);
      --danger: oklch(55% 0.2 25);
      --warning: oklch(68% 0.14 75);
      --info: oklch(55% 0.14 250);
      --radius: 14px;
      --shadow: 0 18px 50px color-mix(in oklch, var(--ink) 15%, transparent);
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      background: var(--soft);
      color: var(--ink);
    }
    button, input, select { font: inherit; }
    button, input, select { min-height: 44px; }
    .shell {
      width: min(1080px, calc(100% - 28px));
      margin: 28px auto;
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: 20px;
      box-shadow: var(--shadow);
      overflow: hidden;
      display: grid;
      grid-template-columns: minmax(240px, 0.72fr) minmax(0, 1.55fr);
    }
    .brand {
      background: var(--brand);
      color: oklch(96% 0.006 260);
      padding: 36px 30px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 720px;
    }
    .mark {
      width: 58px;
      height: 58px;
      border-radius: 50%;
    }
    .mark img {
      display: block;
      width: 100%;
      height: 100%;
      object-fit: contain;
    }
    .brand h1 {
      margin: 24px 0 10px;
      font-size: 1.65rem;
      line-height: 1.15;
      letter-spacing: -0.035em;
    }
    .brand p {
      margin: 0;
      max-width: 28ch;
      color: oklch(77% 0.01 260);
      line-height: 1.55;
      font-size: 0.92rem;
    }
    .live-status {
      border-top: 1px solid oklch(36% 0.01 260);
      padding-top: 22px;
      display: grid;
      gap: 12px;
    }
    .status-line { display: flex; align-items: center; gap: 10px; font-size: 0.88rem; }
    .dot { width: 10px; height: 10px; border-radius: 50%; background: var(--muted); flex: none; }
    .dot.healthy { background: var(--success); box-shadow: 0 0 0 5px color-mix(in oklch, var(--success) 18%, transparent); }
    .dot.error { background: var(--danger); box-shadow: 0 0 0 5px color-mix(in oklch, var(--danger) 18%, transparent); }
    .dot.waiting { background: var(--warning); }
    .content { padding: 30px; }
    .content-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 18px; margin-bottom: 26px; }
    .content-head h2 { margin: 0 0 6px; font-size: 1.35rem; letter-spacing: -0.025em; }
    .content-head p { margin: 0; color: var(--muted); font-size: 0.9rem; }
    .chip {
      display: inline-flex;
      align-items: center;
      min-height: 30px;
      padding: 5px 10px;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: var(--soft);
      color: var(--muted);
      font-size: 0.76rem;
      font-weight: 700;
      white-space: nowrap;
    }
    .section { padding: 22px 0; border-top: 1px solid var(--line); }
    .section:first-of-type { border-top: 0; padding-top: 0; }
    .section-title { margin: 0 0 5px; font-size: 1rem; }
    .section-copy { margin: 0 0 18px; color: var(--muted); font-size: 0.84rem; line-height: 1.5; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
    .field { display: grid; gap: 7px; }
    .field.full { grid-column: 1 / -1; }
    .wifi-picker { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 9px; }
    .scan-button {
      border: 1px solid var(--line);
      border-radius: 10px;
      padding: 10px 14px;
      background: var(--soft);
      color: var(--ink);
      font-weight: 750;
      cursor: pointer;
      white-space: nowrap;
    }
    .scan-button:hover { background: color-mix(in oklch, var(--soft) 55%, var(--line)); }
    .scan-button:focus-visible { outline: 3px solid color-mix(in oklch, var(--info) 30%, transparent); outline-offset: 2px; }
    .scan-button:disabled { cursor: wait; opacity: 0.55; }
    .network-select { background: var(--soft); }
    label { font-size: 0.78rem; font-weight: 700; color: var(--ink); }
    input, select {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 10px;
      background: var(--surface);
      color: var(--ink);
      padding: 10px 12px;
      outline: none;
      transition: border-color 180ms ease, box-shadow 180ms ease;
    }
    input:focus, select:focus { border-color: var(--info); box-shadow: 0 0 0 3px color-mix(in oklch, var(--info) 18%, transparent); }
    .hint { color: var(--muted); font-size: 0.72rem; line-height: 1.4; }
    .check { display: flex; align-items: center; gap: 9px; min-height: 44px; }
    .check input { width: 18px; min-height: 18px; height: 18px; accent-color: var(--brand); }
    .check label { font-weight: 600; }
    .actions { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 18px; }
    .button {
      border: 1px solid var(--brand);
      border-radius: 10px;
      padding: 10px 16px;
      background: var(--brand);
      color: oklch(97% 0.004 260);
      font-weight: 750;
      cursor: pointer;
      transition: transform 180ms cubic-bezier(.22,1,.36,1), opacity 180ms ease, background 180ms ease;
    }
    .button:hover { background: oklch(29% 0.012 260); }
    .button:active { transform: translateY(1px); }
    .button:focus-visible { outline: 3px solid color-mix(in oklch, var(--info) 35%, transparent); outline-offset: 2px; }
    .button:disabled { cursor: wait; opacity: 0.55; }
    .button.secondary { background: var(--surface); color: var(--ink); border-color: var(--line); }
    .button.danger { background: var(--surface); color: var(--danger); border-color: color-mix(in oklch, var(--danger) 35%, var(--line)); }
    .feedback {
      display: none;
      margin: 16px 0 0;
      padding: 12px 14px;
      border: 1px solid var(--line);
      border-radius: 10px;
      background: var(--soft);
      font-size: 0.82rem;
      line-height: 1.45;
    }
    .feedback.show { display: block; }
    .feedback.success { border-color: color-mix(in oklch, var(--success) 40%, var(--line)); color: oklch(40% 0.13 150); }
    .feedback.error { border-color: color-mix(in oklch, var(--danger) 40%, var(--line)); color: oklch(45% 0.17 25); }
    .pin-panel { display: none; }
    .pin-panel.visible { display: block; }
    .pin-input { letter-spacing: 0.35em; text-align: center; font-size: 1.12rem; font-weight: 800; }
    .meta { margin-top: 12px; color: oklch(70% 0.008 260); font-size: 0.73rem; line-height: 1.5; overflow-wrap: anywhere; }
    .hidden { display: none !important; }
    @media (max-width: 760px) {
      .shell { grid-template-columns: 1fr; margin: 14px auto; width: min(100% - 18px, 620px); }
      .brand { min-height: auto; padding: 24px; gap: 30px; }
      .brand p { max-width: 50ch; }
      .content { padding: 24px 20px; }
    }
    @media (max-width: 520px) {
      .grid { grid-template-columns: 1fr; }
      .field.full { grid-column: auto; }
      .wifi-picker { grid-template-columns: 1fr; }
      .content-head { display: grid; }
      .actions .button { width: 100%; }
    }
  </style>
</head>
<body>
  <main class="shell">
    <aside class="brand">
      <div>
        <div class="mark"><img src="/icon.png" width="58" height="58" alt=""></div>
        <h1>Monitor Rejoin BI</h1>
        <p>Configure o dispositivo, autentique sua conta e acompanhe a disponibilidade da plataforma pelos LEDs.</p>
      </div>
      <div class="live-status" aria-live="polite">
        <div class="status-line"><span id="stateDot" class="dot waiting"></span><strong id="stateLabel">Carregando estado</strong></div>
        <div id="stateMessage" class="meta">Consultando o ESP32...</div>
        <div id="deviceMeta" class="meta"></div>
      </div>
    </aside>

    <section class="content">
      <header class="content-head">
        <div>
          <h2>Configuração do dispositivo</h2>
          <p>Nenhuma senha da plataforma é gravada no firmware.</p>
        </div>
        <span id="modeChip" class="chip">Modo de configuração</span>
      </header>

      <form id="configForm" class="section">
        <h3 class="section-title">Conexão e monitoramento</h3>
        <p class="section-copy">Use o endereço completo do seu ambiente, sempre com HTTPS.</p>
        <div class="grid">
          <div class="field full">
            <label for="ssid">Rede Wi-Fi</label>
            <div class="wifi-picker">
              <input id="ssid" name="ssid" maxlength="32" required autocomplete="off" placeholder="Pesquise ou digite o nome da rede">
              <button id="scanWifiButton" class="scan-button" type="button">Pesquisar redes</button>
            </div>
            <select id="wifiNetworks" class="network-select hidden" aria-label="Redes Wi-Fi encontradas">
              <option value="">Selecione uma rede encontrada</option>
            </select>
            <span class="hint">O ESP32 clássico detecta somente redes Wi-Fi de 2,4 GHz.</span>
          </div>
          <div class="field">
            <label for="wifiPassword">Senha do Wi-Fi</label>
            <input id="wifiPassword" name="wifiPassword" type="password" maxlength="63" autocomplete="new-password" placeholder="Deixe vazio para manter">
          </div>
          <div class="field full">
            <label for="platformUrl">Endereço da plataforma</label>
            <input id="platformUrl" name="platformUrl" type="url" maxlength="180" required placeholder="https://empresa.rejoinbi.com.br">
          </div>
          <div class="field">
            <label for="intervalMinutes">Verificar a cada</label>
            <select id="intervalMinutes" name="intervalMinutes">
              <option value="1">1 minuto</option>
              <option value="2">2 minutos</option>
              <option value="5" selected>5 minutos</option>
              <option value="10">10 minutos</option>
              <option value="15">15 minutos</option>
              <option value="30">30 minutos</option>
              <option value="60">60 minutos</option>
            </select>
          </div>
          <div class="field">
            <label for="ledMode">Tipo de LED</label>
            <select id="ledMode" name="ledMode">
              <option value="single">Um LED comum</option>
              <option value="dual">Dois LEDs, vermelho e verde</option>
              <option value="rgb">LED RGB endereçável</option>
            </select>
          </div>
          <div id="singlePinField" class="field">
            <label for="singlePin">GPIO do LED</label>
            <input id="singlePin" name="singlePin" type="number" min="0" max="48" value="2">
          </div>
          <div id="redPinField" class="field hidden">
            <label for="redPin">GPIO vermelho</label>
            <input id="redPin" name="redPin" type="number" min="0" max="48" value="2">
          </div>
          <div id="greenPinField" class="field hidden">
            <label for="greenPin">GPIO verde</label>
            <input id="greenPin" name="greenPin" type="number" min="0" max="48" value="4">
          </div>
          <div id="rgbPinField" class="field hidden">
            <label for="rgbPin">GPIO do RGB</label>
            <input id="rgbPin" name="rgbPin" type="number" min="0" max="48" value="48">
          </div>
          <div id="activeLowField" class="field">
            <div class="check">
              <input id="activeLow" name="activeLow" type="checkbox" value="1">
              <label for="activeLow">LED acende em nível baixo</label>
            </div>
            <span class="hint">Ative para LEDs integrados com lógica invertida.</span>
          </div>
          <div class="field full">
            <div class="check">
              <input id="allowInsecureTls" name="allowInsecureTls" type="checkbox" value="1">
              <label for="allowInsecureTls">Aceitar certificado HTTPS privado ou não reconhecido</label>
            </div>
            <span class="hint">Mantenha desativado em produção. Ativar reduz a proteção contra interceptação.</span>
          </div>
        </div>
        <div class="actions"><button class="button" type="submit">Salvar e conectar</button></div>
        <div id="configFeedback" class="feedback" role="status"></div>
        <div id="wifiConnectionFeedback" class="feedback" role="status" aria-live="polite"></div>
      </form>

      <form id="loginForm" class="section">
        <h3 class="section-title">Autenticação na plataforma</h3>
        <p class="section-copy">A senha permanece apenas na memória durante este login. O ESP32 salva somente a sessão criada pela plataforma.</p>
        <div class="grid">
          <div class="field">
            <label for="email">E-mail</label>
            <input id="email" name="email" type="email" maxlength="254" required autocomplete="email" placeholder="seu@email.com">
          </div>
          <div class="field">
            <label for="password">Senha</label>
            <input id="password" name="password" type="password" maxlength="128" required autocomplete="current-password" placeholder="Sua senha">
          </div>
        </div>
        <div class="actions"><button id="loginButton" class="button" type="submit" disabled>Entrar e iniciar monitoramento</button></div>
        <div id="loginFeedback" class="feedback" role="status"></div>
      </form>

      <form id="pinForm" class="section pin-panel">
        <h3 class="section-title">Confirme o código PIN</h3>
        <p class="section-copy">Digite o código de seis números enviado ao e-mail da conta.</p>
        <div class="field">
          <label for="pin">Código PIN</label>
          <input id="pin" name="pin" class="pin-input" inputmode="numeric" autocomplete="one-time-code" minlength="6" maxlength="6" pattern="[0-9]{6}" placeholder="000000" required>
        </div>
        <div class="actions"><button class="button" type="submit">Validar PIN</button></div>
        <div id="pinFeedback" class="feedback" role="status"></div>
      </form>

      <section class="section">
        <h3 class="section-title">Ações do dispositivo</h3>
        <p class="section-copy">A verificação manual usa a mesma sessão do monitor automático.</p>
        <div class="actions">
          <button id="checkButton" class="button secondary" type="button">Verificar agora</button>
          <button id="restartButton" class="button secondary" type="button">Reiniciar ESP32</button>
          <button id="resetButton" class="button danger" type="button">Apagar configuração</button>
        </div>
        <div id="actionFeedback" class="feedback" role="status"></div>
      </section>
    </section>
  </main>

  <script>
    const byId = id => document.getElementById(id);
    let hydrated = false;

    function feedback(id, message, ok) {
      const box = byId(id);
      box.textContent = message || '';
      const kind = ok === true ? 'success' : ok === false ? 'error' : '';
      box.className = `feedback show ${kind}`.trim();
    }

    async function request(path, options = {}) {
      const response = await fetch(path, { cache: 'no-store', ...options });
      const data = await response.json().catch(() => ({ ok: false, message: `Resposta HTTP ${response.status}` }));
      if (!response.ok || data.ok === false) throw new Error(data.message || `Erro HTTP ${response.status}`);
      return data;
    }

    function bodyFrom(form) {
      return new URLSearchParams(new FormData(form));
    }

    function setBusy(form, busy) {
      const button = form.querySelector('button[type="submit"]');
      if (button) button.disabled = busy;
    }

    function updateLedFields() {
      const mode = byId('ledMode').value;
      byId('singlePinField').classList.toggle('hidden', mode !== 'single');
      byId('redPinField').classList.toggle('hidden', mode !== 'dual');
      byId('greenPinField').classList.toggle('hidden', mode !== 'dual');
      byId('rgbPinField').classList.toggle('hidden', mode !== 'rgb');
      byId('activeLowField').classList.toggle('hidden', mode === 'rgb');
    }

    function signalDescription(rssi) {
      if (rssi >= -55) return 'sinal excelente';
      if (rssi >= -67) return 'sinal bom';
      if (rssi >= -75) return 'sinal regular';
      return 'sinal fraco';
    }

    async function scanWifiNetworks() {
      const button = byId('scanWifiButton');
      const select = byId('wifiNetworks');
      const originalText = button.textContent;
      button.disabled = true;
      button.textContent = 'Pesquisando...';
      try {
        const data = await request('/api/wifi-scan');
        select.replaceChildren();
        const placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.textContent = data.networks.length ? 'Selecione uma rede encontrada' : 'Nenhuma rede 2,4 GHz encontrada';
        select.appendChild(placeholder);
        data.networks.forEach(network => {
          const option = document.createElement('option');
          option.value = network.ssid;
          option.textContent = `${network.ssid} · ${signalDescription(network.rssi)} · ${network.secure ? 'protegida' : 'aberta'}`;
          select.appendChild(option);
        });
        select.classList.remove('hidden');
        feedback(
          'configFeedback',
          data.networks.length
            ? `${data.networks.length} rede(s) compatível(is) encontrada(s). Selecione uma delas.`
            : 'Nenhuma rede 2,4 GHz foi encontrada. Aproxime o ESP32 do roteador ou habilite a faixa 2,4 GHz.',
          data.networks.length > 0
        );
      } catch (error) {
        feedback('configFeedback', error.message, false);
      } finally {
        button.disabled = false;
        button.textContent = originalText;
      }
    }

    function hydrate(data) {
      if (hydrated) return;
      byId('ssid').value = data.wifiSsid || '';
      byId('platformUrl').value = data.platformUrl || '';
      byId('intervalMinutes').value = String(data.intervalMinutes || 5);
      byId('ledMode').value = data.ledMode || 'single';
      byId('singlePin').value = data.singlePin ?? 2;
      byId('redPin').value = data.redPin ?? 2;
      byId('greenPin').value = data.greenPin ?? 4;
      byId('rgbPin').value = data.rgbPin ?? 48;
      byId('activeLow').checked = Boolean(data.activeLow);
      byId('allowInsecureTls').checked = Boolean(data.allowInsecureTls);
      byId('email').value = data.userEmail || '';
      updateLedFields();
      hydrated = true;
    }

    function renderStatus(data) {
      hydrate(data);
      const dot = byId('stateDot');
      const healthy = data.state === 'healthy';
      const failed = ['auth_failed', 'offline', 'platform_error'].includes(data.state);
      dot.className = `dot ${healthy ? 'healthy' : failed ? 'error' : 'waiting'}`;
      byId('stateLabel').textContent = data.stateLabel || 'Aguardando';
      byId('stateMessage').textContent = data.message || '';
      byId('deviceMeta').textContent = `Firmware: v${data.firmwareVersion || '1.0'} · Wi-Fi: ${data.wifiConnected ? data.ip : (data.wifiStatus || 'desconectado')} · Última verificação: ${data.lastCheck || 'ainda não realizada'}`;
      byId('modeChip').textContent = data.configMode ? 'Modo de configuração' : 'Monitoramento ativo';
      byId('pinForm').classList.toggle('visible', Boolean(data.pendingPin));

      const loginButton = byId('loginButton');
      loginButton.disabled = !data.wifiConnected;
      loginButton.title = data.wifiConnected ? '' : 'Aguarde a confirmação da conexão Wi-Fi.';

      if (data.wifiConnected) {
        feedback(
          'wifiConnectionFeedback',
          `Wi-Fi conectado com sucesso. IP: ${data.ip}. Você já pode entrar na plataforma.`,
          true
        );
      } else if (data.wifiConnectPending || data.state === 'connecting') {
        feedback('wifiConnectionFeedback', data.message || 'Testando a conexão com o Wi-Fi...', null);
      } else if (data.wifiSsid) {
        feedback(
          'wifiConnectionFeedback',
          data.message || `Não foi possível conectar: ${data.wifiStatus || 'erro desconhecido'}.`,
          false
        );
      } else {
        const wifiFeedback = byId('wifiConnectionFeedback');
        wifiFeedback.textContent = '';
        wifiFeedback.className = 'feedback';
      }
    }

    async function refreshStatus() {
      try { renderStatus(await request('/api/status')); }
      catch (error) { byId('stateMessage').textContent = error.message; }
    }

    byId('ledMode').addEventListener('change', updateLedFields);
    byId('scanWifiButton').addEventListener('click', scanWifiNetworks);
    byId('wifiNetworks').addEventListener('change', event => {
      if (event.target.value) {
        byId('ssid').value = event.target.value;
        byId('wifiPassword').focus();
      }
    });

    byId('configForm').addEventListener('submit', async event => {
      event.preventDefault();
      setBusy(event.currentTarget, true);
      try {
        const data = await request('/api/config', { method: 'POST', body: bodyFrom(event.currentTarget) });
        feedback('configFeedback', data.message, null);
        hydrated = false;
        await refreshStatus();
      } catch (error) { feedback('configFeedback', error.message, false); }
      finally { setBusy(event.currentTarget, false); }
    });

    byId('loginForm').addEventListener('submit', async event => {
      event.preventDefault();
      setBusy(event.currentTarget, true);
      try {
        const data = await request('/api/login', { method: 'POST', body: bodyFrom(event.currentTarget) });
        feedback('loginFeedback', data.message, true);
        if (!data.requirePin) byId('password').value = '';
        await refreshStatus();
      } catch (error) { feedback('loginFeedback', error.message, false); }
      finally { setBusy(event.currentTarget, false); }
    });

    byId('pinForm').addEventListener('submit', async event => {
      event.preventDefault();
      setBusy(event.currentTarget, true);
      try {
        const data = await request('/api/pin', { method: 'POST', body: bodyFrom(event.currentTarget) });
        feedback('pinFeedback', data.message, true);
        byId('pin').value = '';
        byId('password').value = '';
        await refreshStatus();
      } catch (error) { feedback('pinFeedback', error.message, false); }
      finally { setBusy(event.currentTarget, false); }
    });

    byId('checkButton').addEventListener('click', async () => {
      try { const data = await request('/api/check', { method: 'POST' }); feedback('actionFeedback', data.message, true); await refreshStatus(); }
      catch (error) { feedback('actionFeedback', error.message, false); }
    });

    byId('restartButton').addEventListener('click', async () => {
      if (!confirm('Reiniciar o ESP32 agora?')) return;
      try { await request('/api/restart', { method: 'POST' }); feedback('actionFeedback', 'Reiniciando o dispositivo...', true); }
      catch (error) { feedback('actionFeedback', error.message, false); }
    });

    byId('resetButton').addEventListener('click', async () => {
      const confirmation = prompt('Digite APAGAR para remover Wi-Fi, sessão e configurações:');
      if (confirmation !== 'APAGAR') return;
      try {
        await request('/api/reset', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'confirm=APAGAR' });
        feedback('actionFeedback', 'Configuração removida. O ESP32 será reiniciado.', true);
      } catch (error) { feedback('actionFeedback', error.message, false); }
    });

    refreshStatus();
    setInterval(refreshStatus, 3000);
  </script>
</body>
</html>
)HTML";
