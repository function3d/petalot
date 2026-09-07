// ============================================================================
// INCLUDES COMPATIBLES CON ESP32 Y ESP8266
// ============================================================================
#ifdef ESP32
    #include <AsyncTCP.h>
#elif defined(ESP8266)
    #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESPAsyncHTTPUpdateServer.h> 

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
AsyncWebServer server(80);
ESPAsyncHTTPUpdateServer httpUpdater;

// ============================================================================
// WEB AUTH (optional)
// Define WEB_USER/WEB_PASSWORD before including this file to require a login
// on /set, /reset and /update. Empty password = auth disabled (default).
// ============================================================================
#ifndef WEB_USER
  #define WEB_USER "admin"
#endif
#ifndef WEB_PASSWORD
  #define WEB_PASSWORD ""
#endif

static bool isAuthorized(AsyncWebServerRequest *request) {
  if (WEB_PASSWORD[0] == '\0') return true;
  return request->authenticate(WEB_USER, WEB_PASSWORD);
}

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title data-i18n="gs.title">PETALOT Control</title>
  <style>
    :root {
      --bg: #181c20;          /* Fondo oscuro profundo */
      --card-bg: #262c32;     /* Fondo de tarjetas gris azulado */
      --text: #f0f4f8;        /* Texto principal claro (blanco hueso) */
      --muted: #788898;       /* Texto secundario atenuado */
      --accent: #00ae42;      /* Azul eléctrico para elementos activos */
      --danger: #fb2c36;      /* Rojo plano para alertas y resets */
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, sans-serif; }
    body { background: var(--bg); padding: 1rem; color: var(--text); display: flex; justify-content: center; }
    
    .container { width: 100%; max-width: 500px; display: flex; flex-direction: column; gap: 0.75rem; }
    .card { background: var(--card-bg); padding: 1rem; border:1px solid #363e46; border-radius: 6px; display: flex; flex-direction: column; gap: 0.25rem; }
    
    /* Header */
    .header { flex-direction: row; justify-content: space-between; align-items: center; background:none; border:0; }
    .header h1 { font-size: 1.45rem; font-weight: 800; }
    .header a { color: var(--muted); font-size: 0.8rem; text-decoration: none; }
    .header-right { text-align: right; font-size: 0.8rem; color: var(--muted); }
    .header-right span { color: var(--text); font-weight: 600; }
    #lang-select { background: #181c20; color: var(--text); border: 1px solid #363e46; border-radius: 4px; font-size: 0.75rem; padding: 0.15rem 0.3rem; margin-top: 0.35rem; }

    /*version*/
    #version { text-align:right; font-size: 0.7rem; color: var(--muted); margin-top: 0.25rem; }

    .help-text { color: var(--muted); font-size: 0.7em; margin-bottom: 6px; }
    .help-text a { color: var(--accent); text-decoration: none; }
    .help-text a:hover { text-decoration: underline; }

    .icon { width: 18px; height: 18px; margin-right: 4px; color: #888888; opacity: 0.5; transition: all 0.3s ease; }
    .fire-icon { margin-right: 2px; }
    .fire-on, .motor-on { color: #ff5722; animation: pulse 1.8s infinite alternate ease-in-out; transform-origin: center; }
    .motor-on { animation: spin 3.5s linear infinite; opacity: 1; }
    .conn-icon { width: 12px; height: 12px; margin-right: 6px; border-radius: 50%; }
    .conn-off { background: var(--danger); transition: background 0.3s; opacity: 1 }
    .conn-on { background: var(--accent); animation: pulse 1.8s infinite ease-in-out; }
    @keyframes pulse {
      0% { opacity: 1; }
      100% { opacity: 0.2; }
    }
    @keyframes spin {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }

    /* Grid Panel */
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 0.75rem; }
    .ui-card .title-wrapper {flex-direction: row-reverse; display: flex; flex-wrap: wrap; align-items: center; justify-content: flex-end; }
    .ui-card .title { color: var(--muted); font-size: 0.7rem; font-weight: 700; text-transform: uppercase; letter-spacing: 0.05em; }
    .ui-card .row { display: flex; justify-content: space-between; align-items: center; margin-top: 0.45rem; }
    .ui-card .value { font-size: 1.4rem; font-weight: 700; }
    .ui-card .unit { font-size: 0.9rem; color: var(--muted); font-weight: normal; }
    .ui-card .msg { font-size: 0.7rem; color: var(--muted); margin-top: 0.25rem; }
    .ui-card .msg.warn { color: var(--danger); font-weight: 600; }

    /* Botones Planos en Tema Oscuro */
    .btn-group { display: flex; background: #363e46; border-radius: 4px; overflow: hidden; }
    .btn-group button { background: none; border: none; padding: 0.4rem 0.8rem; font-size: 1rem; font-weight: bold; cursor: pointer; color: var(--text); }
    .btn-group button:hover { background: #4a5a6a; }
    
    /* Toggle Switch Simplificado */
    .switch { position: relative; width: 40px; height: 22px; display: inline-block; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #475569; border-radius: 22px; transition: 0.2s; }
    .slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: 0.2s; }
    input:checked + .slider { background: var(--accent); }
    input:checked + .slider:before { transform: translateX(18px); }

    /* Ajustes / Acordeón */
    .trigger { font-weight: 700; cursor: pointer; font-size: 1rem; padding: 0.2rem 0; }
    .content { display: none; flex-direction: column; gap: 0.6rem; margin-top: 0.5rem; border-top: 1px solid #334155; padding-top: 0.75rem; }
    .card.open .content { display: flex; }
    .card .toggle::after { content: "▼"; }
    .card.open .toggle::after { content: "▲"; }
    .toggle { float:right; }

    .form-group { display: flex; flex-direction: column; gap: 0.15rem; }
    .form-group.row-layout { flex-direction: row; align-items: center; justify-content: space-between; padding: 0.25rem 0; }
    .form-group label { font-size: 0.8rem; color: var(--muted); font-weight: 700; margin-top: 5px; }
    .form-group input[type="text"], .form-group input[type="number"], .form-group input[type="password"] { width: 100%; padding: 0.4rem; border: 1px solid #363e46; border-radius: 4px; font-size: 0.85rem; color: var(--text); background: #181c20; }
    .form-group input[type="text"]:focus, .form-group input[type="number"]:focus, .form-group input[type="password"]:focus { border-color: var(--accent); outline-style: none; }
    .form-group input:disabled { opacity: .5; }

    /* Botonera */
    .actions { display: flex; flex-wrap: wrap; gap: 0.4rem; margin-top: 0.5rem; }
    .btn { padding: 0.5rem 0.75rem; border: none; border-radius: 4px; font-weight: 700; cursor: pointer; color: white; font-size: 0.8rem; background: var(--accent); }
    .btn-danger { background: var(--danger); }
    .float-right { margin-left: auto; }
  </style>
</head>
<body>

  <div class="container">
    
    <div class="card header">
      <div>
          <h1>PETALOT</h1>
          <a href="https://linktr.ee/function.3d" target="_blank">linktr.ee/function.3d</a>
          <br>
          <select id="lang-select"></select>
      </div>
      <div class="header-right">
        ≈<span id="tele-Fs">0</span>m (<span id="tele-Ts">0s</span>) <span data-i18n="gs.ses">ses</span><br>
        ≈<span id="tele-Ft">0</span>m (<span id="tele-Tt">0s</span>) <span data-i18n="gs.tot">tot</span>
      </div>
    </div>

    <div class="grid">
      
      <div class="card ui-card">
        <div class="title-wrapper">
          <div class="title" data-i18n="gs.status">Status</div>
          <div id="conn-icon" class="icon conn-icon conn-off"></div>
        </div>
        <div class="row">
          <div class="value" id="val-status">...</div>
          <label class="switch">
            <input type="checkbox" id="ctrl-status" onchange="sendAction('status', this.checked ? 1 : 0)">
            <span class="slider"></span>
          </label>
        </div>
        <div class="msg warn" id="warn-status"></div>
      </div>

      <div class="card ui-card">
        <div class="title-wrapper">
          <div class="title" id="title-temp" data-i18n="gs.temp">Temp</div>
          <svg id="fire-icon" class="icon fire-icon fire-off" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z"></path>
          </svg>
        </div>
        <div class="row">
          <div>
            <span class="value" id="val-temp">0</span><span class="unit"> °C</span>
          </div>
          
          <div class="btn-group">
            <button onclick="sendAction('To', -5)">-</button>
            <button onclick="sendAction('To', 5)">+</button>
          </div>
        </div>
        <div class="msg" id="msg-temp"></div>
        <div class="msg warn" id="warn-temp"></div>
      </div>

      <div class="card ui-card">
        <div class="title-wrapper">
        <div class="title" data-i18n="gs.speed">Speed</div>
        <svg id="motor-icon" class="icon motor-icon motor-off" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.1a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/>
          <circle cx="12" cy="12" r="3"/>
        </svg>
        </div>
        <div class="row">
          <div>
            <span class="value" id="val-speed">0</span>
            <span class="unit">cm/min</span>
          </div>
          <div class="btn-group">
            <button onclick="sendAction('Vo', -5)">-</button>
            <button onclick="sendAction('Vo', 5)">+</button>
          </div>
        </div>
        <div class="msg warn" id="warn-speed"></div>
        <div class="msg" id="msg-speed"></div>
      </div>

      <div class="card ui-card">
        <div class="title" data-i18n="gs.sensor">Sensor</div>
        <div class="row">
          <div class="value" id="val-filament">...</div>
          <label class="switch">
            <input type="checkbox" id="ctrl-filament" onchange="sendAction('Fenable', this.checked)">
            <span class="slider"></span>
          </label>
        </div>
        <div class="msg warn" id="warn-filament"></div>
      </div>

    </div>

    <div class="card" id="settings-card">
      <div class="trigger" onclick="document.getElementById('settings-card').classList.toggle('open')"><span data-i18n="gs.settings">Settings</span><span class="toggle"></span></div>
      <form id="settings-form" class="content" onsubmit="event.preventDefault();">
      <div class="grid">
        <div class="form-group row-layout"><label data-i18n="st.startOnPower">Start up at power on</label><label class="switch"><input type="checkbox" name="StartOnPower"><span class="slider"></span></label></div>
        <small class="help-text" data-i18n="st.startOnPowerHelp">If you disable it, you'll only be able to start the machine by pressing the sensor</small>
        <div class="form-group row-layout"><label data-i18n="st.motorOnTo">Motor starting at target temp</label><label class="switch"><input type="checkbox" name="MotorOnTo"><span class="slider"></span></label></div>
        <small class="help-text" data-i18n="st.motorOnToHelp">If enabled, the motor will only run once the target temperature is reached</small>
        <div id="setting-oled" class="form-group row-layout"><label data-i18n="st.display">Use OLED Display</label><label class="switch"><input type="checkbox" name="UseDisplay"><span class="slider"></span></label></div>
        <small id="setting-oled-help" class="help-text" data-i18n="st.displayHelp">Turn on the display if your machine has one</small>

        <div class="form-group"><label data-i18n="st.toffset">Temperature Offset</label><input type="number" name="TOffset"><small class="help-text" data-i18n="st.toffsetHelp">Adjust the temperature if you notice it's off</small></div>
        <div class="form-group"><label data-i18n="st.stopDelay">Stop Delay (sec)</label><input type="number" name="Stopdelay"><small class="help-text" data-i18n="st.stopDelayHelp">Seconds to finish processing after strip end passes the sensor</small></div>
        <div class="form-group"><label data-i18n="st.maxTime">Max Time (min)</label><input type="number" name="Maxtime"><small class="help-text" data-i18n="st.maxTimeHelp">Maximum machine run time</small></div>
        <div class="form-group"><label data-i18n="st.sensorTimeout">Sensor timeout (min)</label><input type="number" name="NoFilamentTime"><small class="help-text" data-i18n="st.sensorTimeoutHelp">Minutes to run without sensor activity. If disabled, only Max Time applies</small></div>
        
        <div class="form-group"><label data-i18n="st.ssid">SSID</label><input type="text" name="ssid"><small class="help-text" data-i18n="st.ssidHelp">Your home/work Wi-Fi name</small></div>
        <div class="form-group"><label data-i18n="st.password">SSID Password</label><input type="password" name="password"><small class="help-text" data-i18n="st.passwordHelp">Your Wi-Fi password</small></div>
        
        <div class="form-group"><label data-i18n="st.ip">IP Address</label><input type="text" name="LocalIP"><small class="help-text" data-i18n-html="st.ipHelp">DHCP used if blank. Try <a href="http://petalot.local">petalot.local</a> first; check router for IP if inaccessible</small></div>
        <div class="form-group"><label data-i18n="st.subnet">Subnet</label><input type="text" name="Subnet"><small class="help-text" data-i18n="st.subnetHelp">255.255.255.0 if left blank</small></div>
        <div class="form-group"><label data-i18n="st.gateway">Gateway</label><input type="text" name="Gateway"><small class="help-text" data-i18n="st.gatewayHelp">PETALOT does not require an Internet connection; 0.0.0.0 if left blank</small></div>

        <div style="display:none" class="form-group"><label data-i18n="st.analog">Analog Read</label><input type="text" id="tele-AR" disabled></div>
      </div>
        <div class="actions">
          <button type="button" class="btn" onclick="saveSettings()" data-i18n="btn.save">Save</button>
          <button type="button" class="btn btn-danger float-right" onclick="factoryReset();" data-i18n="btn.factoryReset">Factory Reset</button>
          <button type="button" class="btn btn-danger" onclick="firmwareUpdate()" style="display:none">Update</button>
        </div>
      </form>
    </div>
    <div id="version">
    -.-.-
    </div>
  </div>

  <script>
    const I18N = {
      en: {
        'gs.title': 'PETALOT Control',
        'gs.ses': 'ses', 'gs.tot': 'tot',
        'gs.status': 'Status', 'gs.speed': 'Speed', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Settings',
        'st.startOnPower': 'Start up at power on',
        'st.startOnPowerHelp': "If you disable it, you'll only be able to start the machine by pressing the sensor",
        'st.motorOnTo': 'Motor starting at target temp',
        'st.motorOnToHelp': 'If enabled, the motor will only run once the target temperature is reached',
        'st.display': 'Use OLED Display',
        'st.displayHelp': 'Turn on the display if your machine has one',
        'st.toffset': 'Temperature Offset',
        'st.toffsetHelp': "Adjust the temperature if you notice it's off",
        'st.stopDelay': 'Stop Delay (sec)',
        'st.stopDelayHelp': 'Seconds to finish processing after strip end passes the sensor',
        'st.maxTime': 'Max Time (min)',
        'st.maxTimeHelp': 'Maximum machine run time',
        'st.sensorTimeout': 'Sensor timeout (min)',
        'st.sensorTimeoutHelp': 'Minutes to run without sensor activity. If disabled, only Max Time applies',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Your home/work Wi-Fi name',
        'st.password': 'SSID Password',
        'st.passwordHelp': 'Your Wi-Fi password',
        'st.ip': 'IP Address',
        'st.ipHelp': 'DHCP used if blank. Try <a href="http://petalot.local">petalot.local</a> first; check router for IP if inaccessible',
        'st.subnet': 'Subnet',
        'st.subnetHelp': '255.255.255.0 if left blank',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT does not require an Internet connection; 0.0.0.0 if left blank',
        'st.analog': 'Analog Read',
        'btn.save': 'Save',
        'btn.factoryReset': 'Factory Reset',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'Running',
        't.stopped': 'Stopped',
        't.checkThermistor': 'Check thermistor',
        't.speedWarn': 'Speeds >25 cm/s may cause print failures. Test before batch production',
        't.detected': 'detected',
        't.notDetected': 'no detected',
        't.sensorDisabled': 'Sensor disabled',
        't.confirmSave': 'Are you sure?',
        't.restarting': 'Restarting...',
        't.confirmReset': 'Factory reset? The statistics, temperature and temperature offset will not be reset',
        't.done': 'Done'
      },
      es: {
        'gs.title': 'Control PETALOT',
        'gs.ses': 'ses', 'gs.tot': 'tot',
        'gs.status': 'Estado', 'gs.speed': 'Velocidad', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Ajustes',
        'st.startOnPower': 'Arrancar al encender',
        'st.startOnPowerHelp': 'Si lo desactivas, solo podrás arrancar la máquina pulsando el sensor',
        'st.motorOnTo': 'Motor arranca a la temperatura objetivo',
        'st.motorOnToHelp': 'Si está activado, el motor solo funcionará cuando se alcance la temperatura objetivo',
        'st.display': 'Usar pantalla OLED',
        'st.displayHelp': 'Enciende la pantalla si tu máquina tiene una',
        'st.toffset': 'Desplazamiento de temperatura',
        'st.toffsetHelp': 'Ajusta la temperatura si notas que no cuadra',
        'st.stopDelay': 'Retardo de parada (s)',
        'st.stopDelayHelp': 'Segundos para terminar el proceso tras pasar el final de la tira por el sensor',
        'st.maxTime': 'Tiempo máximo (min)',
        'st.maxTimeHelp': 'Tiempo máximo de funcionamiento de la máquina',
        'st.sensorTimeout': 'Tiempo de espera del sensor (min)',
        'st.sensorTimeoutHelp': 'Minutos de funcionamiento sin actividad del sensor. Si está desactivado, solo aplica el Tiempo máximo',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Nombre de tu red Wi-Fi (casa/trabajo)',
        'st.password': 'Contraseña SSID',
        'st.passwordHelp': 'Tu contraseña de Wi-Fi',
        'st.ip': 'Dirección IP',
        'st.ipHelp': 'Usa DHCP si lo dejas vacío. Prueba <a href="http://petalot.local">petalot.local</a> primero; revisa el router si no accedes',
        'st.subnet': 'Máscara de subred',
        'st.subnetHelp': '255.255.255.0 si lo dejas vacío',
        'st.gateway': 'Puerta de enlace',
        'st.gatewayHelp': 'PETALOT no necesita conexión a Internet; 0.0.0.0 si lo dejas vacío',
        'st.analog': 'Lectura analógica',
        'btn.save': 'Guardar',
        'btn.factoryReset': 'Restablecer de fábrica',
        'msg.minmax': 'mín: {min}, máx: {max}',
        't.running': 'En marcha',
        't.stopped': 'Parado',
        't.checkThermistor': 'Comprueba el termistor',
        't.speedWarn': 'Velocidades >25 cm/s pueden causar fallos de impresión. Prueba antes de producción',
        't.detected': 'detectado',
        't.notDetected': 'no detectado',
        't.sensorDisabled': 'Sensor desactivado',
        't.confirmSave': '¿Estás seguro?',
        't.restarting': 'Reiniciando...',
        't.confirmReset': '¿Restablecer de fábrica? No se restablecerán las estadísticas, temperatura ni desplazamiento',
        't.done': 'Hecho'
      },
      pt: {
        'gs.title': 'Controle PETALOT',
        'gs.ses': 'ses', 'gs.tot': 'tot',
        'gs.status': 'Status', 'gs.speed': 'Velocidade', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Configurações',
        'st.startOnPower': 'Iniciar ao ligar',
        'st.startOnPowerHelp': 'Se desativar, só poderá iniciar a máquina pressionando o sensor',
        'st.motorOnTo': 'Motor inicia na temperatura alvo',
        'st.motorOnToHelp': 'Se ativado, o motor só funcionará quando for atingida a temperatura alvo',
        'st.display': 'Usar display OLED',
        'st.displayHelp': 'Ligue o display se a sua máquina tiver um',
        'st.toffset': 'Deslocamento de temperatura',
        'st.toffsetHelp': 'Ajuste a temperatura se notar que está errada',
        'st.stopDelay': 'Atraso de parada (s)',
        'st.stopDelayHelp': 'Segundos para terminar o processo após a ponta da tira passar pelo sensor',
        'st.maxTime': 'Tempo máximo (min)',
        'st.maxTimeHelp': 'Tempo máximo de funcionamento da máquina',
        'st.sensorTimeout': 'Tempo limite do sensor (min)',
        'st.sensorTimeoutHelp': 'Minutos de funcionamento sem atividade do sensor. Se desativado, só o Tempo máximo se aplica',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Nome da sua rede Wi-Fi',
        'st.password': 'Senha SSID',
        'st.passwordHelp': 'Sua senha do Wi-Fi',
        'st.ip': 'Endereço IP',
        'st.ipHelp': 'DHCP usado se em branco. Tente <a href="http://petalot.local">petalot.local</a> primeiro',
        'st.subnet': 'Máscara de sub-rede',
        'st.subnetHelp': '255.255.255.0 se deixar em branco',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT não requer conexão com a Internet; 0.0.0.0 se deixar em branco',
        'st.analog': 'Leitura analógica',
        'btn.save': 'Salvar',
        'btn.factoryReset': 'Restaurar de fábrica',
        'msg.minmax': 'mín: {min}, máx: {max}',
        't.running': 'Em funcionamento',
        't.stopped': 'Parado',
        't.checkThermistor': 'Verifique o termistor',
        't.speedWarn': 'Velocidades >25 cm/s podem causar falhas de impressão. Teste antes da produção',
        't.detected': 'detectado',
        't.notDetected': 'não detectado',
        't.sensorDisabled': 'Sensor desativado',
        't.confirmSave': 'Tem certeza?',
        't.restarting': 'Reiniciando...',
        't.confirmReset': 'Restaurar de fábrica? Estatísticas, temperatura e deslocamento não serão resetados',
        't.done': 'Concluído'
      },
      fr: {
        'gs.title': 'Contrôle PETALOT',
        'gs.ses': 'sess', 'gs.tot': 'tot',
        'gs.status': 'État', 'gs.speed': 'Vitesse', 'gs.sensor': 'Capteur', 'gs.temp': 'Temp',
        'gs.settings': 'Paramètres',
        'st.startOnPower': 'Démarrer à la mise sous tension',
        'st.startOnPowerHelp': "Si désactivé, vous ne pourrez démarrer la machine qu'en appuyant sur le capteur",
        'st.motorOnTo': 'Moteur démarre à la température cible',
        'st.motorOnToHelp': "Si activé, le moteur ne tournera qu'une fois la température cible atteinte",
        'st.display': "Utiliser l'écran OLED",
        'st.displayHelp': "Allumez l'écran si votre machine en a un",
        'st.toffset': 'Décalage de température',
        'st.toffsetHelp': "Ajustez la température si elle semble fausse",
        'st.stopDelay': "Délai d'arrêt (s)",
        'st.stopDelayHelp': "Secondes pour terminer le processus après le passage de l'extrémité de la bande par le capteur",
        'st.maxTime': 'Temps max (min)',
        'st.maxTimeHelp': 'Temps de fonctionnement maximal de la machine',
        'st.sensorTimeout': 'Délai capteur (min)',
        'st.sensorTimeoutHelp': "Minutes de fonctionnement sans activité du capteur. Si désactivé, seul le Temps max s'applique",
        'st.ssid': 'SSID',
        'st.ssidHelp': "Nom de votre réseau Wi-Fi",
        'st.password': 'Mot de passe SSID',
        'st.passwordHelp': 'Votre mot de passe Wi-Fi',
        'st.ip': 'Adresse IP',
        'st.ipHelp': "DHCP si vide. Essayez <a href=\"http://petalot.local\">petalot.local</a> d'abord",
        'st.subnet': 'Masque de sous-réseau',
        'st.subnetHelp': '255.255.255.0 si vide',
        'st.gateway': 'Passerelle',
        'st.gatewayHelp': "PETALOT ne nécessite pas de connexion Internet ; 0.0.0.0 si vide",
        'st.analog': 'Lecture analogique',
        'btn.save': 'Enregistrer',
        'btn.factoryReset': 'Réinitialiser',
        'msg.minmax': 'min : {min}, max : {max}',
        't.running': 'En marche',
        't.stopped': 'Arrêté',
        't.checkThermistor': 'Vérifiez la thermistance',
        't.speedWarn': 'Des vitesses >25 cm/s peuvent causer des défauts. Testez avant la production',
        't.detected': 'détecté',
        't.notDetected': 'non détecté',
        't.sensorDisabled': 'Capteur désactivé',
        't.confirmSave': 'Êtes-vous sûr ?',
        't.restarting': 'Redémarrage...',
        't.confirmReset': "Réinitialiser ? Les statistiques, la température et le décalage ne seront pas réinitialisés",
        't.done': 'Terminé'
      },
      de: {
        'gs.title': 'PETALOT Steuerung',
        'gs.ses': 'ses', 'gs.tot': 'ges',
        'gs.status': 'Status', 'gs.speed': 'Geschwindigkeit', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Einstellungen',
        'st.startOnPower': 'Beim Einschalten starten',
        'st.startOnPowerHelp': 'Wenn deaktiviert, kann die Maschine nur durch Drücken des Sensors gestartet werden',
        'st.motorOnTo': 'Motor startet bei Zieltemperatur',
        'st.motorOnToHelp': 'Wenn aktiviert, läuft der Motor erst, sobald die Zieltemperatur erreicht ist',
        'st.display': 'OLED-Display verwenden',
        'st.displayHelp': 'Display einschalten, falls die Maschine eines hat',
        'st.toffset': 'Temperaturoffset',
        'st.toffsetHelp': 'Temperatur anpassen, falls sie abweicht',
        'st.stopDelay': 'Stoppverzögerung (s)',
        'st.stopDelayHelp': 'Sekunden zum Beenden des Prozesses, nachdem das Bandende den Sensor passiert hat',
        'st.maxTime': 'Maximale Zeit (min)',
        'st.maxTimeHelp': 'Maximale Laufzeit der Maschine',
        'st.sensorTimeout': 'Sensor-Timeout (min)',
        'st.sensorTimeoutHelp': 'Minuten ohne Sensoraktivität. Wenn deaktiviert, gilt nur die maximale Zeit',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Name deines WLAN-Netzwerks',
        'st.password': 'WLAN-Passwort',
        'st.passwordHelp': 'Dein WLAN-Passwort',
        'st.ip': 'IP-Adresse',
        'st.ipHelp': 'DHCP wenn leer. Versuche zuerst <a href="http://petalot.local">petalot.local</a>',
        'st.subnet': 'Subnetzmaske',
        'st.subnetHelp': '255.255.255.0 wenn leer',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT benötigt keine Internetverbindung; 0.0.0.0 wenn leer',
        'st.analog': 'Analoger Wert',
        'btn.save': 'Speichern',
        'btn.factoryReset': 'Zurücksetzen',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'Läuft',
        't.stopped': 'Gestoppt',
        't.checkThermistor': 'Thermistor prüfen',
        't.speedWarn': 'Geschwindigkeiten >25 cm/s können Druckfehler verursachen. Vor der Produktion testen',
        't.detected': 'erkannt',
        't.notDetected': 'nicht erkannt',
        't.sensorDisabled': 'Sensor deaktiviert',
        't.confirmSave': 'Sicher?',
        't.restarting': 'Neustart...',
        't.confirmReset': 'Zurücksetzen? Statistiken, Temperatur und Offset werden nicht zurückgesetzt',
        't.done': 'Fertig'
      },
      it: {
        'gs.title': 'Controllo PETALOT',
        'gs.ses': 'sess', 'gs.tot': 'tot',
        'gs.status': 'Stato', 'gs.speed': 'Velocità', 'gs.sensor': 'Sensore', 'gs.temp': 'Temp',
        'gs.settings': 'Impostazioni',
        'st.startOnPower': "Avvio all'accensione",
        'st.startOnPowerHelp': "Se disattivato, puoi avviare la macchina solo premendo il sensore",
        'st.motorOnTo': 'Motore avvia alla temperatura target',
        'st.motorOnToHelp': 'Se attivato, il motore gira solo al raggiungimento della temperatura target',
        'st.display': 'Usa display OLED',
        'st.displayHelp': 'Accendi il display se la macchina ne ha uno',
        'st.toffset': 'Offset temperatura',
        'st.toffsetHelp': 'Regola la temperatura se noti uno scostamento',
        'st.stopDelay': 'Ritardo di arresto (s)',
        'st.stopDelayHelp': 'Secondi per terminare il processo dopo il passaggio della fine del nastro dal sensore',
        'st.maxTime': 'Tempo massimo (min)',
        'st.maxTimeHelp': 'Tempo massimo di funzionamento della macchina',
        'st.sensorTimeout': 'Timeout sensore (min)',
        'st.sensorTimeoutHelp': 'Minuti di funzionamento senza attività del sensore. Se disattivato, vale solo il Tempo massimo',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Nome della tua rete Wi-Fi',
        'st.password': 'Password SSID',
        'st.passwordHelp': 'La tua password Wi-Fi',
        'st.ip': 'Indirizzo IP',
        'st.ipHelp': 'DHCP se vuoto. Prova prima <a href="http://petalot.local">petalot.local</a>',
        'st.subnet': 'Subnet mask',
        'st.subnetHelp': '255.255.255.0 se vuoto',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT non richiede connessione Internet; 0.0.0.0 se vuoto',
        'st.analog': 'Lettura analogica',
        'btn.save': 'Salva',
        'btn.factoryReset': 'Ripristino',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'In funzione',
        't.stopped': 'Fermata',
        't.checkThermistor': 'Controlla il termistore',
        't.speedWarn': 'Velocità >25 cm/s possono causare difetti. Test prima della produzione',
        't.detected': 'rilevato',
        't.notDetected': 'non rilevato',
        't.sensorDisabled': 'Sensore disattivato',
        't.confirmSave': 'Sicuro?',
        't.restarting': 'Riavvio...',
        't.confirmReset': 'Ripristino? Statistiche, temperatura e offset non verranno azzerati',
        't.done': 'Fatto'
      },
      zh: {
        'gs.title': 'PETALOT 控制',
        'gs.ses': '本次', 'gs.tot': '累计',
        'gs.status': '状态', 'gs.speed': '速度', 'gs.sensor': '传感器', 'gs.temp': '温度',
        'gs.settings': '设置',
        'st.startOnPower': '开机启动',
        'st.startOnPowerHelp': '如果禁用，只能通过按下传感器启动机器',
        'st.motorOnTo': '到达目标温度后启动电机',
        'st.motorOnToHelp': '启用后，电机只有在达到目标温度后才会运行',
        'st.display': '使用 OLED 显示屏',
        'st.displayHelp': '如果机器有显示屏则开启',
        'st.toffset': '温度偏移',
        'st.toffsetHelp': '如果温度偏差，请调整此值',
        'st.stopDelay': '停止延迟（秒）',
        'st.stopDelayHelp': '带材末端经过传感器后完成加工所需的秒数',
        'st.maxTime': '最大时间（分钟）',
        'st.maxTimeHelp': '机器最大运行时间',
        'st.sensorTimeout': '传感器超时（分钟）',
        'st.sensorTimeoutHelp': '无传感器活动的运行分钟数。如果禁用，则仅应用最大时间',
        'st.ssid': 'SSID',
        'st.ssidHelp': '您的家庭/工作 Wi-Fi 名称',
        'st.password': 'SSID 密码',
        'st.passwordHelp': '您的 Wi-Fi 密码',
        'st.ip': 'IP 地址',
        'st.ipHelp': '留空则使用 DHCP。先尝试 <a href="http://petalot.local">petalot.local</a>',
        'st.subnet': '子网掩码',
        'st.subnetHelp': '留空则使用 255.255.255.0',
        'st.gateway': '网关',
        'st.gatewayHelp': 'PETALOT 不需要互联网连接；留空则为 0.0.0.0',
        'st.analog': '模拟读取',
        'btn.save': '保存',
        'btn.factoryReset': '恢复出厂设置',
        'msg.minmax': '最小：{min}，最大：{max}',
        't.running': '运行中',
        't.stopped': '已停止',
        't.checkThermistor': '检查热敏电阻',
        't.speedWarn': '超过 25 cm/s 的速度可能导致打印失败。量产前请测试',
        't.detected': '已检测',
        't.notDetected': '未检测到',
        't.sensorDisabled': '传感器已禁用',
        't.confirmSave': '确定吗？',
        't.restarting': '正在重启...',
        't.confirmReset': '恢复出厂设置？统计数据、温度和偏移不会被重置',
        't.done': '完成'
      }
    };

    const LANGS = { en: 'English', es: 'Español', pt: 'Português', fr: 'Français', de: 'Deutsch', it: 'Italiano', zh: '中文' };

    let lang = 'en';

    function t(key, args) {
      const str = (I18N[lang] && I18N[lang][key]) || I18N.en[key] || key;
      return (args ? str.replace(/\{(\w+)\}/g, (m, k) => (args[k] !== undefined ? args[k] : m)) : str);
    }

    function detectLang() {
      let l = (localStorage.getItem('petalot.lang') || navigator.language || 'en').slice(0, 2).toLowerCase();
      return I18N[l] ? l : 'en';
    }

    function applyI18n() {
      document.documentElement.lang = lang;
      document.querySelectorAll('[data-i18n]').forEach(el => { el.textContent = t(el.dataset.i18n); });
      document.querySelectorAll('[data-i18n-html]').forEach(el => { el.innerHTML = t(el.dataset.i18nHtml); });
      const sel = document.getElementById('lang-select');
      if (sel.value !== lang) sel.value = lang;
    }

    function setLang(l) {
      lang = I18N[l] ? l : 'en';
      localStorage.setItem('petalot.lang', lang);
      applyI18n();
      fetchConf();
      fetchTele();
    }

    function toHHMMSS(segundos) {
      const horas = Math.floor(segundos / 3600);
      const minutos = Math.floor((segundos % 3600) / 60);
      const segRestantes = segundos % 60;
      
      return `${horas}h${String(minutos)}m${String(segRestantes)}s`;
    }

    function fetchTele() {
      fetch('/tele')
        .then(res => res.json())
        .then(data => {
          document.getElementById('tele-Fs').innerText = Math.round(data.Fs) / 100;
          document.getElementById('tele-Ft').innerText = Math.round(data.Ft) / 100;
          document.getElementById('tele-Ts').innerText = toHHMMSS(data.Ts);
          document.getElementById('tele-Tt').innerText = toHHMMSS(data.Tt);
          
          document.getElementById('val-status').innerText = data.status ? t('t.running') : t('t.stopped');
          document.getElementById('ctrl-status').checked = data.status;
          document.getElementById('warn-status').innerText = (!data.status && data.LastStopReason) ? data.LastStopReason : '';

          document.getElementById('val-temp').innerText = Math.round(data.T);
          document.getElementById('title-temp').innerText = `${t('gs.temp')} (${data.To})`;
          document.getElementById('warn-temp').innerText = (data.T>0) ? '' : t('t.checkThermistor');

          document.getElementById('val-speed').innerText = data.Vo;
          document.getElementById('warn-speed').innerText = (data.Vo>25) ? t('t.speedWarn') : '';

          document.getElementById('val-filament').innerText = data.F ? t('t.detected') : t('t.notDetected');
          document.getElementById('ctrl-filament').checked = data.Fenable;
          document.getElementById('warn-filament').innerText = (!data.Fenable) ? t('t.sensorDisabled') : '';
          
          document.getElementById('tele-AR').value = data.AR || 0;

          updateIcons(data);
        
          setTimeout(fetchTele, 2000);
        })
        .catch(err => {
          document.getElementById('conn-icon').classList.remove('conn-on');
          document.getElementById('conn-icon').classList.add('conn-off');
          const fireIcon = document.getElementById('fire-icon');
          const motorIcon = document.getElementById('motor-icon');
          fireIcon.classList.remove('fire-on');
          fireIcon.classList.add('fire-off');
          motorIcon.classList.remove('motor-on');
          motorIcon.classList.add('motor-off');
          setTimeout(fetchTele, 2000);
        });
    }

    function sendAction(param, value) {
      fetch(`/set?${param}=${value}`).then(res => res.json()).then(() => fetchTele());
    }

    let conf = {};

    function fetchConf() {
      fetch('/get')
        .then(res => res.json())
        .then(data => {
          conf = data;
          if (data.version) {
            document.getElementById('version').innerText = `v${data.version}`;
            
            let versionParts = data.version.split('.');
            let semanticVersion = parseInt(versionParts[0] + versionParts[1] + versionParts[2]);

            if (semanticVersion > 152) {
                document.getElementById('setting-oled').style.display = 'flex';
                document.getElementById('setting-oled-help').style.display = 'block';
            } else {
                document.getElementById('setting-oled').style.display = 'none';
                document.getElementById('setting-oled-help').style.display = 'none';
            }
          }

          document.getElementById('msg-temp').innerText = t('msg.minmax', {min: data.minT, max: data.maxT});
          document.getElementById('msg-speed').innerText = t('msg.minmax', {min: data.minV, max: data.maxV});

          const form = document.getElementById('settings-form');
          
          Object.keys(data).forEach(key => {
            if(form.elements[key]) {
              if(form.elements[key].type === 'checkbox') {
                form.elements[key].checked = (data[key] === true || data[key] === 'true' || data[key] == 1);
              } else {
                form.elements[key].value = data[key];
              }
            }
          });

          updateNetworkFields();

        });
    }

    function saveSettings(reboot) {
      if (!confirm(t('t.confirmSave'))) return;
      
      const form = document.getElementById('settings-form');
      const params = new URLSearchParams();
      
      Array.from(form.elements).forEach(el => {
        if (!el.name) return;
        if (el.name === 'ssid' && el.value != conf.ssid) {
          reboot = true;
        }
        if (el.name === 'password' && el.value != conf.password) {
          reboot = true;
        }
        if (el.name === 'LocalIP' && el.value != conf.LocalIP) {
          reboot = true;
        }
        if (el.name === 'Subnet' && el.value != conf.Subnet) {
          reboot = true;
        }
        if (el.name === 'Gateway' && el.value != conf.Gateway) {
          reboot = true;
        }
        if (el.type === 'checkbox') {
          params.append(el.name, el.checked ? 'true' : 'false');
        } else {
          params.append(el.name, el.value);
        }
      });
      
      if (reboot) params.append('reboot', '1');

      fetch(`/set?${params.toString()}`)
        .then(() => {
          if (reboot) {
            alert(t('t.restarting'));
            setTimeout(() => window.location.reload(), 9000);
          } else {
            fetchConf();
          }
        });
    }

    function factoryReset() {
      if (confirm(t('t.confirmReset'))) {
        fetch('/reset').then(() => alert(t('t.done')));
      }
    }

    function firmwareUpdate() {
      fetch('/set?status=0').then(() => window.location.href = '/update');
    }

    const inputSSID = document.getElementsByName('ssid')[0];
    const inputPassword = document.getElementsByName('password')[0];
    const inputIP = document.getElementsByName('LocalIP')[0];
    const inputSubnet = document.getElementsByName('Subnet')[0];
    const inputGateway = document.getElementsByName('Gateway')[0];

    function updateNetworkFields() {
      const isSsidEmpty = inputSSID.value.trim() === '';
      inputIP.disabled = isSsidEmpty;
      inputSubnet.disabled = isSsidEmpty;
      inputGateway.disabled = isSsidEmpty;
      inputPassword.disabled = isSsidEmpty;
    }

    function updateIcons(data) {
      const fireIcon = document.getElementById('fire-icon');
      const motorIcon = document.getElementById('motor-icon');
      const connIcon = document.getElementById('conn-icon');

      connIcon.classList.remove('conn-off');
      connIcon.classList.add('conn-on');

      if (data.status) {
        if (data.status === 2) {
          motorIcon.classList.remove('motor-off');
          motorIcon.classList.add('motor-on');
        } else {
          motorIcon.classList.remove('motor-on');
          motorIcon.classList.add('motor-off');
        }
          fireIcon.classList.remove('fire-off');
          fireIcon.classList.add('fire-on');
      } else {
          fireIcon.classList.remove('fire-on');
          fireIcon.classList.add('fire-off');
          motorIcon.classList.remove('motor-on');
          motorIcon.classList.add('motor-off');
      }
    }

    window.onload = () => {
      const sel = document.getElementById('lang-select');
      Object.keys(LANGS).forEach(code => {
        const o = document.createElement('option');
        o.value = code;
        o.textContent = LANGS[code];
        sel.appendChild(o);
      });

      sel.addEventListener('change', () => setLang(sel.value));

      lang = detectLang();
      applyI18n();
      fetchConf();
      fetchTele();
      updateNetworkFields();

      inputSSID.addEventListener('input', updateNetworkFields);
          };
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// HANDLERS
// ============================================================================

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void tele(AsyncWebServerRequest *request) {
    StaticJsonDocument<384> teleData;
    teleData["status"] = (status == "working") ? ((stepper.motorEnabled) ? 2 : 1) : 0;
    teleData["T"]    = T;
    teleData["AR"]   = AR;
    teleData["To"]   = To;
    teleData["Vo"]   = Vo;
    teleData["F"]    = F;
    teleData["Fenable"] = Fenable;
    teleData["Ft"]   = Ft;
    teleData["Fs"]   = Fs;
    teleData["Tt"]   = Tt;
    teleData["Ts"]   = Ts;
    teleData["LastStopReason"] = LastStopReason;
    teleData["Output"] = String(map((int)Output, 0, 255, 0, 100)) + "%";
    String r;
    serializeJson(teleData, r);
    request->send(200, "application/json", r);
}

void get(AsyncWebServerRequest *request) {
    request->send(200, "application/json", printConf()); 
}

void reset(AsyncWebServerRequest *request) {
    if (!isAuthorized(request)) {
        request->requestAuthentication();
        return;
    }
    String stats = request->arg("stats");
    request->redirect("/");
    if (stats.toInt() == 1)
      factoryReset(true);
    else
      factoryReset();
}

void set(AsyncWebServerRequest *request) {
    if (!isAuthorized(request)) {
        request->requestAuthentication();
        return;
    }
    String ToChange = request->arg("To");
    if (ToChange != "") {
        if (ToChange.toInt() + To <= maxT && ToChange.toInt() + To >= minT) {
            To += ToChange.toInt();
            saveConfiguration(false);
        }
        tele(request);
        return;
    }
    
    String VoChange = request->arg("Vo");
    if (VoChange != "") {
        if (VoChange.toInt() + Vo <= maxV && VoChange.toInt() + Vo >= minV) {
            Vo += VoChange.toInt();
            saveConfiguration(false);
        }
        tele(request);
        return;
    }
    
    String statusChange = request->arg("status");
    if (statusChange != "") {
        if (statusChange.toFloat() || statusChange == "1" || statusChange == "true")
            start();
        else
            stop();
        saveConfiguration(false);
        tele(request);
        return;
    }
    
    String FeChange = request->arg("Fenable");
    if (FeChange != "") {
        if (FeChange.toFloat() || FeChange == "true")
            Fenable = true;
        else
            Fenable = false;
        saveConfiguration(false);
        tele(request);
        return;
    }

    // Bulk update of settings form fields
    if (request->hasArg("Gate")) Gate = request->arg("Gate").toInt();
    if (request->hasArg("TOffset")) TOffset = request->arg("TOffset").toInt();
    
    if (request->hasArg("StartOnPower")) StartOnPower = (request->arg("StartOnPower") == "true");
    if (request->hasArg("MotorOnTo")) MotorOnTo = (request->arg("MotorOnTo") == "true");
    if (request->hasArg("UseDisplay")) UseDisplay = (request->arg("UseDisplay") == "true");
    if (request->hasArg("UseDisplay")) displayInitialized = false;
    
    if (request->hasArg("ssid")) request->arg("ssid").toCharArray(ssid, sizeof(ssid));
    if (request->hasArg("password")) request->arg("password").toCharArray(password, sizeof(password));
    if (request->hasArg("LocalIP")) LocalIP = request->arg("LocalIP");
    if (request->hasArg("Subnet")) Subnet = request->arg("Subnet");
    if (request->hasArg("Gateway")) Gateway = request->arg("Gateway");
    if (request->hasArg("Stopdelay")) Stopdelay = request->arg("Stopdelay").toInt();
    if (request->hasArg("Maxtime")) Maxtime = request->arg("Maxtime").toInt();
    if (request->hasArg("NoFilamentTime")) NoFilamentTime = request->arg("NoFilamentTime").toInt();

    String Reboot = request->arg("reboot");
    if (Reboot.toInt() == 1) {
        saveConfiguration(true);
    } else {
        saveConfiguration(false);
        loadConfiguration();
    }
    get(request);
}

void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
}

// ============================================================================
// INICIALIZACIÓN DEL SERVIDOR
// ============================================================================
void InitServer() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/get", HTTP_GET, get);
    server.on("/tele", HTTP_GET, tele);
    server.on("/set", HTTP_GET, set); 
    server.on("/reset", HTTP_GET, reset);
    
    server.onNotFound([](AsyncWebServerRequest *request) {
        handleNotFound(request);
    });
    
    httpUpdater.setup(&server, WEB_USER, WEB_PASSWORD);
    server.begin();
}

void serverTask() {}