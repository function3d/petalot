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
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>PETALOT Control</title>
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
      </div>
      <div class="header-right">
        ≈<span id="tele-Fs">0</span>m (<span id="tele-Ts">0s</span>) ses<br>
        ≈<span id="tele-Ft">0</span>m (<span id="tele-Tt">0s</span>) tot
      </div>
    </div>

    <div class="grid">
      
      <div class="card ui-card">
        <div class="title-wrapper">
          <div class="title">Status</div>
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
          <div class="title" id="title-temp">Temp</div>
          <svg id="fire-icon" class="icon fire-icon fire-off" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z"></path>
          </svg>
        </div>
        <div class="row">
          <div>
            <span class="value" id="val-temp">0</span><span class="unit"> °C</span>
            <!--<span class="msg" id="val-output"></span>-->
          </div>
          
          <div class="btn-group">
            <button onclick="sendAction('To', -5)">-</button>
            <button onclick="sendAction('To', 5)">+</button>
          </div>
        </div>
        <div class="msg" id="msg-temp">min: 160, max: 220</div>
        <div class="msg warn" id="warn-temp"></div>
      </div>

      <div class="card ui-card">
        <div class="title-wrapper">
        <div class="title">Speed</div>
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
        <div class="msg" id="msg-speed">min: 5, max: 35</div>
      </div>

      <div class="card ui-card">
        <div class="title">Sensor</div>
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
      <div class="trigger" onclick="document.getElementById('settings-card').classList.toggle('open')">Settings<span class="toggle"></span></div>
      <form id="settings-form" class="content" onsubmit="event.preventDefault();">
      <div class="grid">
        <div class="form-group row-layout"><label>Start up at power on</label><label class="switch"><input type="checkbox" name="StartOnPower"><span class="slider"></span></label></div>
        <small class="help-text">If you disable it, you'll only be able to start the machine by pressing the sensor</small>
        <div class="form-group row-layout"><label>Motor starting at target temp</label><label class="switch"><input type="checkbox" name="MotorOnTo"><span class="slider"></span></label></div>
        <small class="help-text">If enabled, the motor will only run once the target temperature is reached</small>
        <div id="setting-oled" class="form-group row-layout"><label>Use OLED Display</label><label class="switch"><input type="checkbox" name="UseDisplay"><span class="slider"></span></label></div>
        <small id="setting-oled-help" class="help-text">Turn on the display if your machine has one</small>

        <!--<div class="form-group"><label>Gate %</label><input type="number" name="Gate"></div>-->
        <div class="form-group"><label>Temperature Offset</label><input type="number" name="TOffset"><small class="help-text">Adjust the temperature if you notice it's off</small></div>
        <div class="form-group"><label>Stop Delay (sec)</label><input type="number" name="Stopdelay"><small class="help-text">Seconds to finish processing after strip end passes the sensor</small></div>
        <div class="form-group"><label>Max Time (min)</label><input type="number" name="Maxtime"><small class="help-text">Maximum machine run time</small></div>
        <div class="form-group"><label>Sensor timeout (min)</label><input type="number" name="NoFilamentTime"><small class="help-text">Minutes to run without sensor activity. If disabled, only Max Time applies</small></div>
        
        <div class="form-group"><label>SSID</label><input type="text" name="ssid"><small class="help-text">Your home/work Wi-Fi name</small></div>
        <div class="form-group"><label>SSID Password</label><input type="password" name="password"><small class="help-text">Your Wi-Fi password</small></div>
        
        <div class="form-group"><label>IP Address</label><input type="text" name="LocalIP"><small class="help-text">DHCP used if blank. Try <a href="http://petalot.local">petalot.local</a> first; check router for IP if inaccessible</small></div>
        <div class="form-group"><label>Subnet</label><input type="text" name="Subnet"><small class="help-text">255.255.255.0 if left blank</small></div>
        <div class="form-group"><label>Gateway</label><input type="text" name="Gateway"><small class="help-text">PETALOT does not require an Internet connection; 0.0.0.0 if left blank</small></div>

        <div style="display:none" class="form-group"><label>Analog Read</label><input type="text" id="tele-AR" disabled></div>
      </div>
        <div class="actions">
          <button type="button" class="btn" onclick="saveSettings()">Save</button>
          <button type="button" class="btn btn-danger float-right" onclick="factoryReset();">Factory Reset</button>
          <!--<button type="button" class="btn btn-danger" onclick="firmwareUpdate()">Update</button>-->
        </div>
      </form>
    </div>
    <div id="version">
    -.-.-
    </div>
  </div>

  <script>
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
          
          document.getElementById('val-status').innerText = data.status ? 'Running' : 'Stopped';
          document.getElementById('ctrl-status').checked = data.status;
          document.getElementById('warn-status').innerText = (!data.status && data.LastStopReason) ? data.LastStopReason : '';

          document.getElementById('val-temp').innerText = Math.round(data.T);
          document.getElementById('title-temp').innerText = `Temp (${data.To})`;
          //document.getElementById('val-output').innerText = data.Output ? ` (${data.Output})` : '';
          document.getElementById('warn-temp').innerText = (data.T>0) ? '' : 'Check thermistor';

          document.getElementById('val-speed').innerText = data.Vo;
          document.getElementById('warn-speed').innerText = (data.Vo>25) ? 'Speeds >25 cm/s may cause print failures. Test before batch production' : '';

          document.getElementById('val-filament').innerText = data.F ? 'detected' : 'no detected';
          document.getElementById('ctrl-filament').checked = data.Fenable;
          document.getElementById('warn-filament').innerText = (!data.Fenable) ? 'Sensor disabled' : '';
          
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
            // 1. Update the UI text with the full version string ("1.4.5.260624")
            document.getElementById('version').innerText = `v${data.version}`;
            
            // 2. Split the string by dots into an array: ['1', '4', '5', '260624']
            let versionParts = data.version.split('.');
            
            // 3. Reconstruct only the semantic version digits (Major + Minor + Patch)
            // This extracts ['1', '4', '5'] and joins them back into "145"
            let semanticVersion = parseInt(versionParts[0] + versionParts[1] + versionParts[2]);

            // 4. Run your conditional check exactly as before
            if (semanticVersion > 152) {
                document.getElementById('setting-oled').style.display = 'flex';
                document.getElementById('setting-oled-help').style.display = 'block';
            } else {
                document.getElementById('setting-oled').style.display = 'none';
                document.getElementById('setting-oled-help').style.display = 'none';
            }
          }

          document.getElementById('msg-temp').innerText = `min: ${data.minT}, max: ${data.maxT}`;
          document.getElementById('msg-speed').innerText = `min: ${data.minV}, max: ${data.maxV}`;

          const form = document.getElementById('settings-form');
          
          Object.keys(data).forEach(key => {
            if(form.elements[key]) {
              // Si el elemento es un checkbox, evalúa si es true o 'true'
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
      if (!confirm('Are you sure?')) return;
      
      const form = document.getElementById('settings-form');
      const params = new URLSearchParams();
      
      // Procesamos todos los elementos del formulario manualmente para asegurar booleanos limpios
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
            alert("Restarting...");
            setTimeout(() => window.location.reload(), 9000);
          } else {
            fetchConf();
          }
        });
    }

    function factoryReset() {
      if (confirm('Factory reset? The statistics, temperature and temperature offset will not be reset')) {
        fetch('/reset').then(() => alert('Done'));
        //setTimeout(() => window.location.reload(), 1000);
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

    // Función para actualizar el estado de los tres campos
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
      fetchConf();
      fetchTele();

      // 1. Aplica la regla al cargar la página (por si SSID arranca vacío)
      updateNetworkFields();

      // 2. Escucha cada cambio en el campo SSID
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
    //request->send(200, "text/plain", "Factory reset done");
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

    // Actualización masiva de campos desde el formulario
    if (request->hasArg("Gate")) Gate = request->arg("Gate").toInt();
    if (request->hasArg("TOffset")) TOffset = request->arg("TOffset").toInt();
    
    // Al venir de checkboxes, validamos los strings "true" enviados por JavaScript
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