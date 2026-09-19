#pragma once

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="icon" href="data:image/svg+xml,%3Csvg%20xmlns%3D'http://www.w3.org/2000/svg'%20viewBox%3D'0%200%2032%2032'%3E%3Crect%20width%3D'32'%20height%3D'32'%20rx%3D'7'%20fill%3D'%23181c20'/%3E%3Crect%20x%3D'7'%20y%3D'7'%20width%3D'18'%20height%3D'18'%20rx%3D'4'%20fill%3D'none'%20stroke%3D'%2300ae42'%20stroke-width%3D'2.5'/%3E%3Ctext%20x%3D'16'%20y%3D'22'%20font-size%3D'15'%20text-anchor%3D'middle'%20fill%3D'%2300ae42'%20font-family%3D'sans-serif'%20font-weight%3D'bold'%3EP%3C/text%3E%3C/svg%3E">
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
    .settings-select { background: #181c20; color: var(--text); border: 1px solid #363e46; border-radius: 4px; font-size: 0.75rem; padding: 0.15rem 0.3rem; margin-top: 0.35rem; }

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
    .conn-text { color: var(--muted); font-size: 0.7rem; font-weight: 600; margin-left: 2px; letter-spacing: 0; }
    .conn-text .conn-up { color: var(--accent); }
    .conn-text .conn-down { color: var(--danger); }
    @keyframes pulse {
      0% { opacity: 1; }
      100% { opacity: 0.2; }
    }
    @keyframes spin {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }

    /* Grid Panel */
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; }
    .ui-card .title-wrapper {display: flex; flex-wrap: wrap; align-items: center; }
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
    .switch { position: relative; width: 40px; height: 22px; display: inline-block; flex: none;}
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #475569; border-radius: 22px; transition: 0.2s; width: 40px; }
    .slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: 0.2s; }
    input:checked + .slider { background: var(--accent); }
    input:checked + .slider:before { transform: translateX(18px); }

    /* Ajustes / Acordeón */
    .trigger { font-weight: 700; cursor: pointer; font-size: 1rem; padding: 0.2rem 0; }
    .content { display: none; flex-direction: column; gap: 0.6rem; margin-top: 0.5rem; padding-top: 0.75rem; }
    .card.open .content { display: flex; }
    .card .toggle::after { content: "▼"; }
    .card.open .toggle::after { content: "▲"; }
    .toggle { float:right; }

    /* Settings Tabs */
    .tab-bar { display: flex; flex-wrap: wrap; gap: 0.35rem; border-bottom: 1px solid #334155; padding-bottom: 0.55rem; }
    .tab-btn { background: none; border: none; color: var(--muted); font-size: 0.8rem; font-weight: 700; padding: 0.3rem 0.7rem; border-radius: 4px; cursor: pointer; }
    .tab-btn:hover { background: #363e46; color: var(--text); }
    .tab-btn.active { background: #363e46; color: var(--text); }
    .tab-panel { display: none; }
    .tab-panel.active { display: flex; flex-direction: column; gap: 0.6rem; }

    .form-group { display: flex; flex-direction: column; gap: 0.15rem; }
    .form-group .row-layout { gap:1rem; display: flex; flex-direction: row; align-items: center; justify-content: space-between; padding: 0.25rem 0; }
    .form-group label, .form-group span.label { font-size: 0.8rem; color: var(--muted); font-weight: 700; margin-top: 5px; }
    .form-group input[type="text"], .form-group input[type="number"], .form-group input[type="password"] { width: 100%; padding: 0.4rem; border: 1px solid #363e46; border-radius: 4px; font-size: 0.85rem; color: var(--text); background: #181c20; }
    .form-group input[type="text"]:focus, .form-group input[type="number"]:focus, .form-group input[type="password"]:focus, .form-group select:focus { border-color: var(--accent); outline-style: none; }
    .form-group input:disabled { opacity: .5; }
    .form-group input[type="file"] { color: var(--text); font-size: 0.8rem; }

    .msg { font-size: 0.7rem; color: var(--muted); margin-top: 0.25rem; }
    .msg.warn { color: var(--danger); font-weight: 600; }

    /* Botonera */
    .actions { display: flex; flex-wrap: wrap; gap: 0.4rem; padding-top: 1rem; border-top: 1px solid #334155; }
    .btn { padding: 0.5rem 0.75rem; border: none; border-radius: 4px; font-weight: 700; cursor: pointer; color: white; font-size: 0.8rem; background: var(--accent); }
    .btn:disabled { background: var(--muted); cursor: not-allowed; }
    .btn-danger { background: var(--danger); }
    .float-right { margin-left: auto; }
  </style>
</head>
<body>

  <div class="container">

    <div class="card header">
      <div>
          <h1><img style="width: 28px;
    vertical-align: sub;" src="data:image/svg+xml,%3Csvg%20xmlns%3D'http://www.w3.org/2000/svg'%20viewBox%3D'0%200%2032%2032'%3E%3Crect%20width%3D'32'%20height%3D'32'%20rx%3D'7'%20fill%3D'%23181c20'/%3E%3Crect%20x%3D'7'%20y%3D'7'%20width%3D'18'%20height%3D'18'%20rx%3D'4'%20fill%3D'none'%20stroke%3D'%2300ae42'%20stroke-width%3D'2.5'/%3E%3Ctext%20x%3D'16'%20y%3D'22'%20font-size%3D'15'%20text-anchor%3D'middle'%20fill%3D'%2300ae42'%20font-family%3D'sans-serif'%20font-weight%3D'bold'%3EP%3C/text%3E%3C/svg%3E" />PETaLot</h1>
          <a href="https://linktr.ee/function.3d" target="_blank">linktr.ee/function.3d</a>
      </div>
      <div class="header-right">
        ≈<span id="tele-Fs">0</span>m (<span id="tele-Ts">0s</span>) <span data-i18n="gs.ses">ses</span><br>
        ≈<span id="tele-Ft">0</span>m (<span id="tele-Tt">0s</span>) <span data-i18n="gs.tot">tot</span>
      </div>
    </div>

    <div class="grid">

      <div class="card ui-card">
        <div class="title-wrapper">
          <div id="conn-icon" class="icon conn-icon conn-off"></div>
          <div class="title" data-i18n="gs.status">Status</div>
           <small class="conn-text">(<span id="conn-text" class="conn-down"></span>)</small>
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
          <svg id="fire-icon" class="icon fire-icon fire-off" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z"></path>
          </svg>
          <div class="title" id="title-temp" data-i18n="gs.temp">Temp</div>
        </div>
        <div class="row">
          <div>
            <span class="value" id="val-temp">0</span><span class="unit"> °C</span>
            <small class="msg" id="val-output"></small>
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
        <svg id="motor-icon" class="icon motor-icon motor-off" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.1a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/>
          <circle cx="12" cy="12" r="3"/>
        </svg>
        <div class="title" data-i18n="gs.speed">Speed</div>
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
      <div class="tab-bar">
        <button type="button" class="tab-btn active" data-tab="general" data-i18n="tab.general" onclick="showSettingsTab('general')">General</button>
        <button type="button" class="tab-btn" data-tab="network" data-i18n="tab.network" onclick="showSettingsTab('network')">Network</button>
        <button type="button" class="tab-btn" data-tab="advanced" data-i18n="tab.advanced" onclick="showSettingsTab('advanced')">Advanced</button>
      </div>

      <div class="tab-panel active" id="tab-general">
      <div class="grid">
        <div class="form-group"><span class="label" data-i18n="st.language">Language</span><select id="lang-select" class="settings-select"></select></div>

        <div class="form-group"><div class="row-layout"><span class="label" data-i18n="st.startOnPower">Start up at power on</span><label class="switch"><input type="checkbox" name="StartOnPower"><span class="slider"></span></label></div><small class="help-text" data-i18n="st.startOnPowerHelp">If you disable it, you'll only be able to start the machine by pressing the sensor</small></div>
        <div class="form-group"><div class="row-layout"><span class="label" data-i18n="st.motorOnTo">Motor starting at target temp</span><label class="switch"><input type="checkbox" name="MotorOnTo"><span class="slider"></span></label></div><small class="help-text" data-i18n="st.motorOnToHelp">If enabled, the motor will only run once the target temperature is reached</small></div>
        <div class="form-group"><span class="label" data-i18n="st.stopDelay">Stop Delay (sec)</span><input type="number" name="Stopdelay"><small class="help-text" data-i18n="st.stopDelayHelp">Seconds to finish processing after strip end passes the sensor</small></div>
        <div class="form-group"><span class="label" data-i18n="st.maxTime">Max Time (min)</span><input type="number" name="Maxtime"><small class="help-text" data-i18n="st.maxTimeHelp">Maximum machine run time</small></div>
        <div class="form-group"><span class="label" data-i18n="st.sensorTimeout">Sensor timeout (min)</span><input type="number" name="NoFilamentTime"><small class="help-text" data-i18n="st.sensorTimeoutHelp">Minutes to run without sensor activity. If disabled, only Max Time applies</small></div>
        <div id="setting-oled" class="form-group"><div class="row-layout"><span class="label" data-i18n="st.display">Use OLED Display</span><label class="switch"><input type="checkbox" name="UseDisplay"><span class="slider"></span></label></div><small id="setting-oled-help" class="help-text" data-i18n="st.displayHelp">Turn on the display if your machine has one</small></div>
      </div>
      </div>
      <div class="tab-panel" id="tab-network">
      <div class="grid">
        <div class="form-group"><span class="label" data-i18n="st.ssid">SSID</span><input type="text" name="ssid"><small class="help-text" data-i18n="st.ssidHelp">Your home/work Wi-Fi name</small></div>
        <div class="form-group"><span class="label" data-i18n="st.password">SSID Password</span><input type="password" name="password"><small class="help-text" data-i18n="st.passwordHelp">Your Wi-Fi password</small></div>

        <div class="form-group"><span class="label" data-i18n="st.ip">IP Address</span><input type="text" name="LocalIP"><small class="help-text" data-i18n-html="st.ipHelp">DHCP used if blank. Try <a href="http://petalot.local">petalot.local</a> first; check router for IP if inaccessible</small></div>
        <div class="form-group"><span class="label" data-i18n="st.subnet">Subnet</span><input type="text" name="Subnet"><small class="help-text" data-i18n="st.subnetHelp">255.255.255.0 if left blank</small></div>
        <div class="form-group"><span class="label" data-i18n="st.gateway">Gateway</span><input type="text" name="Gateway"><small class="help-text" data-i18n="st.gatewayHelp">PETALOT does not require an Internet connection; 0.0.0.0 if left blank</small></div>
      </div>
      </div>

      <div class="tab-panel" id="tab-advanced">
      <div class="grid">
        <!--<div class="form-group"><span class="label" data-i18n="st.control">Heating control</span><select name="ControlMode" class="settings-select" onchange="updateControlFields()"><option value="0">PID</option><option value="1">Bang-bang</option></select><small class="help-text" data-i18n="st.controlHelp">PID holds the temperature steady out of the box; Bang-bang is the simpler classic controller</small></div>
        <div class="form-group" data-mode="pid"><span class="label" data-i18n="st.kp">Kp (proportional)</span><input type="number" name="Kp" step="0.1"><small class="help-text" data-i18n="st.kpHelp">Response speed: if it oscillates, lower it</small></div>
        <div class="form-group" data-mode="pid"><span class="label" data-i18n="st.ki">Ki (integral)</span><input type="number" name="Ki" step="0.01"><small class="help-text" data-i18n="st.kiHelp">Reaches the target: if it stays below, raise it</small></div>
        <div class="form-group" data-mode="pid"><span class="label" data-i18n="st.kd">Kd (derivative)</span><input type="number" name="Kd" step="1"><small class="help-text" data-i18n="st.kdHelp">Damping: if it oscillates, raise it</small></div>
        <div class="form-group" data-mode="bang"><span class="label" data-i18n="st.hys">Hysteresis band</span><input type="number" name="HYS" step="0.1"><small class="help-text" data-i18n="st.hysHelp">Degrees each side of the target still driven at hold duty before cutting</small></div>
        <div class="form-group" data-mode="bang"><span class="label" data-i18n="st.ramp">Approach ramp</span><input type="number" name="RAMP" step="0.1"><small class="help-text" data-i18n="st.rampHelp">Over these degrees power ramps down from the maximum to the hold duty</small></div>
        <div class="form-group" data-mode="bang"><span class="label" data-i18n="st.hold">Hold duty</span><input type="number" name="HOLD" step="1"><small class="help-text" data-i18n="st.holdHelp">Minimum power (%) delivered near the target to keep the temperature stable</small></div>-->
        <div class="form-group"><span class="label" data-i18n="st.toffset">Temperature Offset</span><input type="number" name="TOffset"><small class="help-text" data-i18n="st.toffsetHelp">Adjust the temperature if you notice it's off</small></div>

        <div class="form-group"><span class="label" data-i18n="gs.update">Firmware Update</span><input type="file" id="up-firmware" accept=".bin,.bin.gz"><button type="button" class="btn" onclick="startUpdate()" data-i18n="btn.update">Update</button><div class="msg" id="update-msg"></div></div>

        <div class="form-group"><span class="label" data-i18n="st.updOnline">Online update</span><div class="msg" id="upd-status"></div><span id="upd-btns"><button type="button" class="btn" id="btn-check-upd" onclick="checkOnlineUpdate()" data-i18n="st.updCheck">Check for updates</button> <button type="button" class="btn" id="btn-install-upd" style="display:none" onclick="installOnlineUpdate()" data-i18n="st.updInstall">Install update</button></span></div>

        <div style="display:none" class="form-group"><span class="label" data-i18n="st.analog">Analog Read</span><input type="text" id="tele-AR" disabled></div>
      </div>
      </div>

        <div class="actions">
          <button type="button" class="btn" onclick="saveSettings()" data-i18n="btn.save">Save</button>
          <button type="button" class="btn btn-danger float-right" onclick="factoryReset();" data-i18n="btn.factoryReset">Factory Reset</button>
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
        'tab.general': 'General',
        'tab.run': 'Run',
        'tab.network': 'Network',
        'tab.advanced': 'Advanced',
        'st.startOnPower': 'Start up at power on',
        'st.startOnPowerHelp': "If you disable it, you'll only be able to start the machine by pressing the sensor",
        'st.motorOnTo': 'Motor starting at target temp',
        'st.motorOnToHelp': 'If enabled, the motor will only run once the target temperature is reached',
        'st.display': 'Use OLED Display',
        'st.language': 'Language',
        'st.displayHelp': 'Turn on the display if your machine has one',
        'st.control': 'Heating control',
        'st.controlHelp': 'PID holds the temperature steady out of the box; Bang-bang is the simpler classic controller',
        'st.kp': 'Kp (proportional)',
        'st.kpHelp': 'Response speed: if it oscillates, lower it',
        'st.ki': 'Ki (integral)',
        'st.kiHelp': 'Reaches the target: if it stays below, raise it',
        'st.kd': 'Kd (derivative)',
        'st.kdHelp': 'Damping: if it oscillates, raise it',
        'st.hys': 'Hysteresis band',
        'st.hysHelp': 'Degrees each side of the target still driven at hold duty before cutting',
        'st.ramp': 'Approach ramp',
        'st.rampHelp': 'Over these degrees power ramps down from the maximum to the hold duty',
        'st.hold': 'Hold duty',
        'st.holdHelp': 'Minimum power (%) delivered near the target to keep the temperature stable',
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
        'st.ipHelp': 'DHCP used if blank. Check router for assigned IP if manual IP is not set',
        'st.subnet': 'Subnet',
        'st.subnetHelp': '255.255.255.0 if left blank',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT does not require an Internet connection; 0.0.0.0 if left blank',
        'st.analog': 'Analog Read',
        'btn.save': 'Apply',
        'btn.factoryReset': 'Factory Reset',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'Running',
        't.connected': 'connected',
        't.disconnected': 'disconnected',

        't.stopped': 'Stopped',
        't.checkThermistor': 'Check thermistor',
        't.speedWarn': 'Speeds >25 cm/s may cause print failures. Test before batch production',
        't.detected': 'detected',
        't.notDetected': 'no detected',
        't.sensorDisabled': 'Sensor disabled',
        't.confirmSave': 'Are you sure?',
        't.restarting': 'Restarting...',
        't.confirmReset': 'Factory reset? The statistics, temperature, offset and heating tuning will not be reset',
        't.done': 'Done',
        'gs.update': 'Firmware Update',
        'btn.update': 'Update',
        'msg.updating': 'Updating... do not disconnect',
        'msg.updateError': 'Update error:',
        'msg.updateMismatch': 'This firmware does not match the registered PCB version ({pcb}). Flash the correct one from the /update page',
        'st.updOnline': 'Online update',
        'st.updCheck': 'Check for updates',
        'st.updInstall': 'Install update',
        'msg.updChecking': 'Checking for updates...',
        'msg.updAvailable': 'Version {ver} is available',
        'msg.updUptodate': 'You are on the latest version',
        'msg.updOffline': 'No internet connection',
        'msg.updInstalling': 'Installing update... do not disconnect'
      },
      es: {
        'gs.title': 'Control PETALOT',
        'gs.ses': 'ses', 'gs.tot': 'tot',
        'gs.status': 'Estado', 'gs.speed': 'Velocidad', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Ajustes',
        'tab.general': 'General',
        'tab.run': 'Funcionamiento',
        'tab.network': 'Red',
        'tab.advanced': 'Avanzado',
        'st.startOnPower': 'Arrancar al encender',
        'st.startOnPowerHelp': 'Si lo desactivas, solo podrás arrancar la máquina pulsando el sensor',
        'st.motorOnTo': 'Motor arranca a la temperatura objetivo',
        'st.motorOnToHelp': 'Si está activado, el motor solo funcionará cuando se alcance la temperatura objetivo',
        'st.display': 'Usar pantalla OLED',
        'st.language': 'Idioma',
        'st.displayHelp': 'Enciende la pantalla si tu máquina tiene una',
        'st.control': 'Control de calentamiento',
        'st.controlHelp': 'El PID mantiene la temperatura estable sin calibración; Bang-bang es el controlador clásico más simple',
        'st.kp': 'Kp (proporcional)',
        'st.kpHelp': 'Velocidad de respuesta: si oscila, bájalo',
        'st.ki': 'Ki (integral)',
        'st.kiHelp': 'Alcanza el objetivo: si se queda por debajo, súbelo',
        'st.kd': 'Kd (derivada)',
        'st.kdHelp': 'Amortiguación: si oscila, súbelo',
        'st.hys': 'Banda de histéresis',
        'st.hysHelp': 'Grados a cada lado del objetivo que se mantienen a potencia de retención antes de cortar',
        'st.ramp': 'Rampa de aproximación',
        'st.rampHelp': 'Grados en los que la potencia baja del máximo a la de retención',
        'st.hold': 'Potencia de retención',
        'st.holdHelp': 'Potencia mínima (%) cerca del objetivo para mantener la temperatura estable',
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
        'st.ipHelp': 'Usa DHCP si lo dejas vacío. Revisa el router para ver la IP asignada si no has puesto IP manual',
        'st.subnet': 'Máscara de subred',
        'st.subnetHelp': '255.255.255.0 si lo dejas vacío',
        'st.gateway': 'Puerta de enlace',
        'st.gatewayHelp': 'PETALOT no necesita conexión a Internet; 0.0.0.0 si lo dejas vacío',
        'st.analog': 'Lectura analógica',
        'btn.save': 'Aplicar',
        'btn.factoryReset': 'Restablecer de fábrica',
        'msg.minmax': 'mín: {min}, máx: {max}',
        't.running': 'En marcha',
        't.connected': 'conectado',
        't.disconnected': 'desconectado',

        't.stopped': 'Parado',
        't.checkThermistor': 'Comprueba el termistor',
        't.speedWarn': 'Velocidades >25 cm/s pueden causar fallos de impresión. Prueba antes de producción',
        't.detected': 'detectado',
        't.notDetected': 'no detectado',
        't.sensorDisabled': 'Sensor desactivado',
        't.confirmSave': '¿Estás seguro?',
        't.restarting': 'Reiniciando...',
        't.confirmReset': '¿Restablecer de fábrica? No se restablecerán las estadísticas, temperatura, desplazamiento ni la sintonía de calentamiento',
        't.done': 'Hecho',
        'gs.update': 'Actualización de firmware',
        'btn.update': 'Actualizar',
        'msg.updating': 'Actualizando... no desconectes',
        'msg.updateError': 'Error de actualización:',
        'st.updOnline': 'Actualización online',
        'st.updCheck': 'Buscar actualizaciones',
        'st.updInstall': 'Instalar actualización',
        'msg.updChecking': 'Buscando actualizaciones...',
        'msg.updAvailable': 'La versión {ver} está disponible',
        'msg.updUptodate': 'Estás en la última versión',
        'msg.updOffline': 'Sin conexión a internet',
        'msg.updInstalling': 'Instalando actualización... no desconectes',
        'msg.updateMismatch': 'Este firmware no coincide con la versión de PCB registrada ({pcb}). Instala la correcta desde la página /update'
      },
      pt: {
        'gs.title': 'Controle PETALOT',
        'gs.ses': 'ses', 'gs.tot': 'tot',
        'gs.status': 'Status', 'gs.speed': 'Velocidade', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Configurações',
        'tab.general': 'Geral',
        'tab.run': 'Funcionamento',
        'tab.network': 'Rede',
        'tab.advanced': 'Avançado',
        'st.startOnPower': 'Iniciar ao ligar',
        'st.startOnPowerHelp': 'Se desativar, só poderá iniciar a máquina pressionando o sensor',
        'st.motorOnTo': 'Motor inicia na temperatura alvo',
        'st.motorOnToHelp': 'Se ativado, o motor só funcionará quando for atingida a temperatura alvo',
        'st.display': 'Usar display OLED',
        'st.language': 'Idioma',
        'st.displayHelp': 'Ligue o display se a sua máquina tiver um',
        'st.control': 'Controle do aquecimento',
        'st.controlHelp': 'O PID mantém a temperatura estável sem calibração; Bang-bang é o controlador clássico mais simples',
        'st.kp': 'Kp (proporcional)',
        'st.kpHelp': 'Velocidade de resposta: se oscilar, diminua',
        'st.ki': 'Ki (integral)',
        'st.kiHelp': 'Atinge o alvo: se ficar abaixo, aumente',
        'st.kd': 'Kd (derivada)',
        'st.kdHelp': 'Amortecimento: se oscilar, aumente',
        'st.hys': 'Banda de histerese',
        'st.hysHelp': 'Graus de cada lado do alvo mantidos na potência de retenção antes de cortar',
        'st.ramp': 'Rampa de aproximação',
        'st.rampHelp': 'Graus nos quais a potência cai do máximo para a de retenção',
        'st.hold': 'Potência de retenção',
        'st.holdHelp': 'Potência mínima (%) perto do alvo para manter a temperatura estável',
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
        'st.ipHelp': 'Usa DHCP se deixar em branco. Verifique a IP atribuída no roteador se não definir IP manual',
        'st.subnet': 'Máscara de sub-rede',
        'st.subnetHelp': '255.255.255.0 se deixar em branco',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT não requer conexão com a Internet; 0.0.0.0 se deixar em branco',
        'st.analog': 'Leitura analógica',
        'btn.save': 'Aplicar',
        'btn.factoryReset': 'Restaurar de fábrica',
        'msg.minmax': 'mín: {min}, máx: {max}',
        't.running': 'Em funcionamento',
        't.connected': 'conectado',
        't.disconnected': 'desconectado',

        't.stopped': 'Parado',
        't.checkThermistor': 'Verifique o termistor',
        't.speedWarn': 'Velocidades >25 cm/s podem causar falhas de impressão. Teste antes da produção',
        't.detected': 'detectado',
        't.notDetected': 'não detectado',
        't.sensorDisabled': 'Sensor desativado',
        't.confirmSave': 'Tem certeza?',
        't.restarting': 'Reiniciando...',
        't.confirmReset': 'Restaurar de fábrica? Estatísticas, temperatura, deslocamento e ajuste de aquecimento não serão resetados',
        't.done': 'Concluído',
        'gs.update': 'Atualização de firmware',
        'btn.update': 'Atualizar',
        'msg.updating': 'Atualizando... não desconecte',
        'msg.updateError': 'Erro de atualização:',
        'st.updOnline': 'Atualização online',
        'st.updCheck': 'Procurar atualizações',
        'st.updInstall': 'Instalar atualização',
        'msg.updChecking': 'A procurar atualizações...',
        'msg.updAvailable': 'A versão {ver} está disponível',
        'msg.updUptodate': 'Está na última versão',
        'msg.updOffline': 'Sem ligação à internet',
        'msg.updInstalling': 'A instalar atualização... não desligue',
        'msg.updateMismatch': 'Este firmware não corresponde à versão de PCB registrada ({pcb}). Instale a correta pela página /update'
      },
      fr: {
        'gs.title': 'Contrôle PETALOT',
        'gs.ses': 'sess', 'gs.tot': 'tot',
        'gs.status': 'État', 'gs.speed': 'Vitesse', 'gs.sensor': 'Capteur', 'gs.temp': 'Temp',
        'gs.settings': 'Paramètres',
        'tab.general': 'Général',
        'tab.run': 'Fonctionnement',
        'tab.network': 'Réseau',
        'tab.advanced': 'Avancé',
        'st.startOnPower': 'Démarrer à la mise sous tension',
        'st.startOnPowerHelp': "Si désactivé, vous ne pourrez démarrer la machine qu'en appuyant sur le capteur",
        'st.motorOnTo': 'Moteur démarre à la température cible',
        'st.motorOnToHelp': "Si activé, le moteur ne tournera qu'une fois la température cible atteinte",
        'st.display': "Utiliser l'écran OLED",
        'st.language': 'Langue',
        'st.displayHelp': "Allumez l'écran si votre machine en a un",
        'st.control': 'Contrôle du chauffage',
        'st.controlHelp': "Le PID maintient la température stable sans calibrage ; Bang-bang est le contrôleur classique plus simple",
        'st.kp': 'Kp (proportionnel)',
        'st.kpHelp': "Vitesse de réponse : s'il oscille, diminuez-le",
        'st.ki': 'Ki (intégral)',
        'st.kiHelp': "Atteint la cible : s'il reste en dessous, augmentez-le",
        'st.kd': 'Kd (dérivée)',
        'st.kdHelp': "Amortissement : s'il oscille, augmentez-le",
        'st.hys': "Bande d'hystérésis",
        'st.hysHelp': "Degrés de chaque côté de la cible maintenus à la puissance de maintien avant coupe",
        'st.ramp': "Rampe d'approche",
        'st.rampHelp': "Degrés sur lesquels la puissance descend du maximum à la puissance de maintien",
        'st.hold': 'Puissance de maintien',
        'st.holdHelp': 'Puissance minimale (%) près de la cible pour garder la température stable',
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
        'st.ipHelp': "DHCP utilisé si vide. Vérifiez dans le routeur l'IP attribuée si aucune IP manuelle n'est définie",
        'st.subnet': 'Masque de sous-réseau',
        'st.subnetHelp': '255.255.255.0 si vide',
        'st.gateway': 'Passerelle',
        'st.gatewayHelp': "PETALOT ne nécessite pas de connexion Internet ; 0.0.0.0 si vide",
        'st.analog': 'Lecture analogique',
        'btn.save': 'Appliquer',
        'btn.factoryReset': 'Réinitialiser',
        'msg.minmax': 'min : {min}, max : {max}',
        't.running': 'En marche',
        't.connected': 'connecté',
        't.disconnected': 'déconnecté',

        't.stopped': 'Arrêté',
        't.checkThermistor': 'Vérifiez la thermistance',
        't.speedWarn': 'Des vitesses >25 cm/s peuvent causer des défauts. Testez avant la production',
        't.detected': 'détecté',
        't.notDetected': 'non détecté',
        't.sensorDisabled': 'Capteur désactivé',
        't.confirmSave': 'Êtes-vous sûr ?',
        't.restarting': 'Redémarrage...',
        't.confirmReset': "Réinitialiser ? Les statistiques, la température, le décalage et le réglage du chauffage ne seront pas réinitialisés",
        't.done': 'Terminé',
        'gs.update': 'Mise à jour du firmware',
        'btn.update': 'Mettre à jour',
        'msg.updating': 'Mise à jour... ne déconnectez pas',
        'msg.updateError': 'Erreur de mise à jour :',
        'st.updOnline': 'Mise à jour en ligne',
        'st.updCheck': 'Rechercher des mises à jour',
        'st.updInstall': 'Installer la mise à jour',
        'msg.updChecking': 'Recherche de mises à jour...',
        'msg.updAvailable': 'La version {ver} est disponible',
        'msg.updUptodate': 'Vous êtes à jour',
        'msg.updOffline': 'Pas de connexion internet',
        'msg.updInstalling': 'Installation... ne débranchez pas',
        'msg.updateMismatch': "Ce firmware ne correspond pas à la version de PCB enregistrée ({pcb}). Installez la bonne version depuis la page /update"
      },
      de: {
        'gs.title': 'PETALOT Steuerung',
        'gs.ses': 'ses', 'gs.tot': 'ges',
        'gs.status': 'Status', 'gs.speed': 'Geschwindigkeit', 'gs.sensor': 'Sensor', 'gs.temp': 'Temp',
        'gs.settings': 'Einstellungen',
        'tab.general': 'Allgemein',
        'tab.run': 'Betrieb',
        'tab.network': 'Netzwerk',
        'tab.advanced': 'Erweitert',
        'st.startOnPower': 'Beim Einschalten starten',
        'st.startOnPowerHelp': 'Wenn deaktiviert, kann die Maschine nur durch Drücken des Sensors gestartet werden',
        'st.motorOnTo': 'Motor startet bei Zieltemperatur',
        'st.motorOnToHelp': 'Wenn aktiviert, läuft der Motor erst, sobald die Zieltemperatur erreicht ist',
        'st.display': 'OLED-Display verwenden',
        'st.language': 'Sprache',
        'st.displayHelp': 'Display einschalten, falls die Maschine eines hat',
        'st.control': 'Heizregelung',
        'st.controlHelp': 'PID hält die Temperatur stabil ohne Kalibrierung; Bang-bang ist der einfachere klassische Regler',
        'st.kp': 'Kp (proportional)',
        'st.kpHelp': 'Ansprechgeschwindigkeit: bei Schwingen verringern',
        'st.ki': 'Ki (integral)',
        'st.kiHelp': 'Erreicht das Ziel: wenn es darunter bleibt, erhöhen',
        'st.kd': 'Kd (Differenzial)',
        'st.kdHelp': 'Dämpfung: bei Schwingen erhöhen',
        'st.hys': 'Hysterese-Band',
        'st.hysHelp': 'Grade pro Seite der Zieltemperatur, die auf Halteleistung gehalten werden, bevor abgeschaltet wird',
        'st.ramp': 'Annäherungsrampe',
        'st.rampHelp': 'Grade, in denen die Leistung vom Maximum auf die Halteleistung abnimmt',
        'st.hold': 'Halteleistung',
        'st.holdHelp': 'Mindestleistung (%) nahe dem Ziel für eine stabile Temperatur',
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
        'st.ipHelp': 'DHCP wird verwendet, wenn leer. Prüfe im Router die zugewiesene IP, wenn keine manuelle IP gesetzt ist',
        'st.subnet': 'Subnetzmaske',
        'st.subnetHelp': '255.255.255.0 wenn leer',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT benötigt keine Internetverbindung; 0.0.0.0 wenn leer',
        'st.analog': 'Analoger Wert',
        'btn.save': 'Anwenden',
        'btn.factoryReset': 'Zurücksetzen',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'Läuft',
        't.connected': 'verbunden',
        't.disconnected': 'getrennt',

        't.stopped': 'Gestoppt',
        't.checkThermistor': 'Thermistor prüfen',
        't.speedWarn': 'Geschwindigkeiten >25 cm/s können Druckfehler verursachen. Vor der Produktion testen',
        't.detected': 'erkannt',
        't.notDetected': 'nicht erkannt',
        't.sensorDisabled': 'Sensor deaktiviert',
        't.confirmSave': 'Sicher?',
        't.restarting': 'Neustart...',
        't.confirmReset': 'Zurücksetzen? Statistiken, Temperatur, Offset und Heizabstimmung werden nicht zurückgesetzt',
        't.done': 'Fertig',
        'gs.update': 'Firmware-Update',
        'btn.update': 'Aktualisieren',
        'msg.updating': 'Aktualisiere... nicht trennen',
        'msg.updateError': 'Update-Fehler:',
        'st.updOnline': 'Online-Update',
        'st.updCheck': 'Nach Updates suchen',
        'st.updInstall': 'Update installieren',
        'msg.updChecking': 'Suche nach Updates...',
        'msg.updAvailable': 'Version {ver} ist verfügbar',
        'msg.updUptodate': 'Sie sind auf dem neuesten Stand',
        'msg.updOffline': 'Keine Internetverbindung',
        'msg.updInstalling': 'Update wird installiert... nicht trennen',
        'msg.updateMismatch': 'Diese Firmware passt nicht zur registrierten PCB-Version ({pcb}). Installiere die richtige über die /update-Seite'
      },
      it: {
        'gs.title': 'Controllo PETALOT',
        'gs.ses': 'sess', 'gs.tot': 'tot',
        'gs.status': 'Stato', 'gs.speed': 'Velocità', 'gs.sensor': 'Sensore', 'gs.temp': 'Temp',
        'gs.settings': 'Impostazioni',
        'tab.general': 'Generale',
        'tab.run': 'Funzionamento',
        'tab.network': 'Rete',
        'tab.advanced': 'Avanzato',
        'st.startOnPower': "Avvio all'accensione",
        'st.startOnPowerHelp': "Se disattivato, puoi avviare la macchina solo premendo il sensore",
        'st.motorOnTo': 'Motore avvia alla temperatura target',
        'st.motorOnToHelp': 'Se attivato, il motore gira solo al raggiungimento della temperatura target',
        'st.display': 'Usa display OLED',
        'st.language': 'Lingua',
        'st.displayHelp': 'Accendi il display se la macchina ne ha uno',
        'st.control': 'Controllo riscaldamento',
        'st.controlHelp': 'Il PID mantiene la temperatura stabile senza calibrazione; il Bang-bang è il controllore classico più semplice',
        'st.kp': 'Kp (proporzionale)',
        'st.kpHelp': 'Velocità di risposta: se oscilla, riducilo',
        'st.ki': 'Ki (integrale)',
        'st.kiHelp': 'Raggiunge il target: se resta sotto, aumentalo',
        'st.kd': 'Kd (derivata)',
        'st.kdHelp': 'Smorzamento: se oscilla, aumentalo',
        'st.hys': 'Banda di isteresi',
        'st.hysHelp': 'Gradi per lato dal target mantenuti alla potenza di tenuta prima del taglio',
        'st.ramp': 'Rampa di avvicinamento',
        'st.rampHelp': 'Gradi in cui la potenza scende dal massimo alla tenuta',
        'st.hold': 'Potenza di tenuta',
        'st.holdHelp': 'Potenza minima (%) vicino al target per mantenere stabile la temperatura',
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
        'st.ipHelp': 'DHCP usato se vuoto. Controlla l\'IP assegnata nel router se non è impostata un\'IP manuale',
        'st.subnet': 'Subnet mask',
        'st.subnetHelp': '255.255.255.0 se vuoto',
        'st.gateway': 'Gateway',
        'st.gatewayHelp': 'PETALOT non richiede connessione Internet; 0.0.0.0 se vuoto',
        'st.analog': 'Lettura analogica',
        'btn.save': 'Applica',
        'btn.factoryReset': 'Ripristino',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'In funzione',
        't.connected': 'connesso',
        't.disconnected': 'disconnesso',

        't.stopped': 'Fermata',
        't.checkThermistor': 'Controlla il termistore',
        't.speedWarn': 'Velocità >25 cm/s possono causare difetti. Test prima della produzione',
        't.detected': 'rilevato',
        't.notDetected': 'non rilevato',
        't.sensorDisabled': 'Sensore disattivato',
        't.confirmSave': 'Sicuro?',
        't.restarting': 'Riavvio...',
        't.confirmReset': 'Ripristino? Statistiche, temperatura, offset e regolazione riscaldamento non verranno azzerati',
        't.done': 'Fatto',
        'gs.update': 'Aggiornamento firmware',
        'btn.update': 'Aggiorna',
        'msg.updating': 'Aggiornamento... non scollegare',
        'msg.updateError': 'Errore di aggiornamento:',
        'st.updOnline': 'Aggiornamento online',
        'st.updCheck': 'Verifica aggiornamenti',
        'st.updInstall': 'Installa aggiornamento',
        'msg.updChecking': 'Verifica aggiornamenti...',
        'msg.updAvailable': 'La versione {ver} è disponibile',
        'msg.updUptodate': 'Hai la versione più recente',
        'msg.updOffline': 'Nessuna connessione internet',
        'msg.updInstalling': 'Installazione aggiornamento... non scollegare',
        'msg.updateMismatch': 'Questo firmware non corrisponde alla versione PCB registrata ({pcb}). Installa la versione corretta dalla pagina /update'
      },
      zh: {
        'gs.title': 'PETALOT 控制',
        'gs.ses': '本次', 'gs.tot': '累计',
        'gs.status': '状态', 'gs.speed': '速度', 'gs.sensor': '传感器', 'gs.temp': '温度',
        'gs.settings': '设置',
        'tab.general': '常规',
        'tab.run': '运行',
        'tab.network': '网络',
        'tab.advanced': '高级',
        'st.startOnPower': '开机启动',
        'st.startOnPowerHelp': '如果禁用，只能通过按下传感器启动机器',
        'st.motorOnTo': '到达目标温度后启动电机',
        'st.motorOnToHelp': '启用后，电机只有在达到目标温度后才会运行',
        'st.display': '使用 OLED 显示屏',
        'st.language': '语言',
        'st.displayHelp': '如果机器有显示屏则开启',
        'st.control': '加热控制',
        'st.controlHelp': 'PID 无需校准即可保持温度稳定；Bang-bang 是更简单的经典控制器',
        'st.kp': 'Kp（比例）',
        'st.kpHelp': '响应速度：如果振荡，请调低',
        'st.ki': 'Ki（积分）',
        'st.kiHelp': '达到目标：如果达不到，请调高',
        'st.kd': 'Kd（微分）',
        'st.kdHelp': '阻尼：如果振荡，请调高',
        'st.hys': '迟滞带',
        'st.hysHelp': '目标两侧在关断前仍以保持功率驱动的度数',
        'st.ramp': '接近斜坡',
        'st.rampHelp': '功率在这些度数内从最大降到保持功率',
        'st.hold': '保持功率',
        'st.holdHelp': '目标附近维持温度稳定的最小功率 (%)',
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
        'st.ipHelp': '留空则使用 DHCP。若未设置手动 IP，请在路由器中查看分配的 IP',
        'st.subnet': '子网掩码',
        'st.subnetHelp': '留空则使用 255.255.255.0',
        'st.gateway': '网关',
        'st.gatewayHelp': 'PETALOT 不需要互联网连接；留空则为 0.0.0.0',
        'st.analog': '模拟读取',
        'btn.save': '应用',
        'btn.factoryReset': '恢复出厂设置',
        'msg.minmax': '最小：{min}，最大：{max}',
        't.running': '运行中',
        't.connected': '已连接',
        't.disconnected': '已断开',

        't.stopped': '已停止',
        't.checkThermistor': '检查热敏电阻',
        't.speedWarn': '超过 25 cm/s 的速度可能导致打印失败。量产前请测试',
        't.detected': '已检测',
        't.notDetected': '未检测到',
        't.sensorDisabled': '传感器已禁用',
        't.confirmSave': '确定吗？',
        't.restarting': '正在重启...',
        't.confirmReset': '恢复出厂设置？统计数据、温度、偏移和加热调校 不会被重置',
        't.done': '完成',
        'gs.update': '固件更新',
        'btn.update': '更新',
        'msg.updating': '正在更新……请勿断开',
        'msg.updateError': '更新错误：',
        'st.updOnline': '在线更新',
        'st.updCheck': '检查更新',
        'st.updInstall': '安装更新',
        'msg.updChecking': '正在检查更新……',
        'msg.updAvailable': '版本 {ver} 可用',
        'msg.updUptodate': '已是最新版本',
        'msg.updOffline': '无互联网连接',
        'msg.updInstalling': '正在安装更新……请勿断开',
        'msg.updateMismatch': '此固件与已注册的 PCB 版本（{pcb}）不匹配。请从 /update 页面安装正确的版本'
      },
      cs: {
        'gs.title': 'Ovládání PETALOT',
        'gs.ses': 'ses', 'gs.tot': 'cel',
        'gs.status': 'Stav', 'gs.speed': 'Rychlost', 'gs.sensor': 'Čidlo', 'gs.temp': 'Teplota',
        'gs.settings': 'Nastavení',
        'tab.general': 'Obecné',
        'tab.run': 'Provoz',
        'tab.network': 'Síť',
        'tab.advanced': 'Pokročilé',
        'st.startOnPower': 'Spustit po zapnutí',
        'st.startOnPowerHelp': 'Pokud zakážete, stroj spustíte pouze stisknutím čidla',
        'st.motorOnTo': 'Motor startuje při cílové teplotě',
        'st.motorOnToHelp': 'Pokud je povoleno, motor se spustí až po dosažení cílové teploty',
        'st.display': 'Použít OLED displej',
        'st.language': 'Jazyk',
        'st.displayHelp': 'Zapněte displej, pokud ho váš stroj má',
        'st.control': 'Regulace ohřevu',
        'st.controlHelp': 'PID drží teplotu stabilní bez kalibrace; Bang-bang je jednodušší klasický regulátor',
        'st.kp': 'Kp (proporcionální)',
        'st.kpHelp': 'Rychlost odezvy: pokud kmitá, snižte',
        'st.ki': 'Ki (integrální)',
        'st.kiHelp': 'Dosahuje cíle: pokud zůstává pod, zvyšte',
        'st.kd': 'Kd (derivační)',
        'st.kdHelp': 'Tlumení: pokud kmitá, zvyšte',
        'st.hys': 'Hysterezní pásmo',
        'st.hysHelp': 'Stupně na obě strany cíle držené na udržovacím výkonu před vypnutím',
        'st.ramp': 'Náběhová rampa',
        'st.rampHelp': 'Stupně, na kterých výkon klesá z maxima na udržovací úroveň',
        'st.hold': 'Udržovací výkon',
        'st.holdHelp': 'Minimální výkon (%) poblíž cíle pro stabilní teplotu',
        'st.toffset': 'Korekce teploty',
        'st.toffsetHelp': 'Upravte teplotu, pokud přesně nesedí',
        'st.stopDelay': 'Zpoždění zastavení (s)',
        'st.stopDelayHelp': 'Sekundy na dokončení procesu po projetí konce proužku čidlem',
        'st.maxTime': 'Maximální čas (min)',
        'st.maxTimeHelp': 'Maximální doba provozu stroje',
        'st.sensorTimeout': 'Časový limit čidla (min)',
        'st.sensorTimeoutHelp': 'Minuty provozu bez aktivity čidla. Pokud je zakázáno, platí pouze Maximální čas',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Název vaší Wi-Fi (doma / v práci)',
        'st.password': 'Heslo SSID',
        'st.passwordHelp': 'Vaše heslo Wi-Fi',
        'st.ip': 'IP adresa',
        'st.ipHelp': 'DHCP, pokud prázdné. Pokud není nastavena ruční IP, zkontrolujte přidělenou IP v routeru',
        'st.subnet': 'Maska sítě',
        'st.subnetHelp': '255.255.255.0, pokud prázdné',
        'st.gateway': 'Brána',
        'st.gatewayHelp': 'PETALOT nepotřebuje připojení k internetu; 0.0.0.0, pokud prázdné',
        'st.analog': 'Analogový vstup',
        'btn.save': 'Použít',
        'btn.factoryReset': 'Tovární nastavení',
        'msg.minmax': 'min: {min}, max: {max}',
        't.running': 'Běží',
        't.connected': 'připojeno',
        't.disconnected': 'odpojeno',
        't.stopped': 'Zastaveno',
        't.checkThermistor': 'Zkontrolujte termistor',
        't.speedWarn': 'Rychlosti >25 cm/s mohou způsobit tiskové vady. Otestujte před sériovou výrobou',
        't.detected': 'detekováno',
        't.notDetected': 'nedetekováno',
        't.sensorDisabled': 'Čidlo zakázáno',
        't.confirmSave': 'Jste si jistí?',
        't.restarting': 'Restartování...',
        't.confirmReset': 'Tovární reset? Statistiky, teplota, korekce teploty a nastavení ohřevu nebudou resetovány',
        't.done': 'Hotovo',
        'gs.update': 'Aktualizace firmwaru',
        'btn.update': 'Aktualizovat',
        'msg.updating': 'Aktualizace... neodpojujte',
        'msg.updateError': 'Chyba aktualizace:',
        'st.updOnline': 'Online aktualizace',
        'st.updCheck': 'Zkontrolovat aktualizace',
        'st.updInstall': 'Nainstalovat aktualizaci',
        'msg.updChecking': 'Kontrola aktualizací...',
        'msg.updAvailable': 'Verze {ver} je k dispozici',
        'msg.updUptodate': 'Máte nejnovější verzi',
        'msg.updOffline': 'Bez připojení k internetu',
        'msg.updInstalling': 'Instalace aktualizace... neodpojujte',
        'msg.updateMismatch': 'Tento firmware neodpovídá registrované verzi PCB ({pcb}). Nainstalujte správnou verzi ze stránky /update'
      },
      ru: {
        'gs.title': 'Управление PETALOT',
        'gs.ses': 'сес', 'gs.tot': 'итог',
        'gs.status': 'Статус', 'gs.speed': 'Скорость', 'gs.sensor': 'Датчик', 'gs.temp': 'Темп.',
        'gs.settings': 'Настройки',
        'tab.general': 'Общие',
        'tab.run': 'Работа',
        'tab.network': 'Сеть',
        'tab.advanced': 'Дополнительно',
        'st.startOnPower': 'Запуск при включении',
        'st.startOnPowerHelp': 'Если выключено, запускать машину можно только нажатием на датчик',
        'st.motorOnTo': 'Мотор запускается при целевой температуре',
        'st.motorOnToHelp': 'Если включено, мотор работает только после достижения целевой температуры',
        'st.display': 'Использовать OLED-экран',
        'st.language': 'Язык',
        'st.displayHelp': 'Включите экран, если он есть на машине',
        'st.control': 'Управление нагревом',
        'st.controlHelp': 'PID держит температуру стабильной без калибровки; Bang-bang — более простой классический регулятор',
        'st.kp': 'Kp (пропорциональный)',
        'st.kpHelp': 'Скорость реакции: если колеблется, уменьшите',
        'st.ki': 'Ki (интегральный)',
        'st.kiHelp': 'Достигает цели: если не достигает, увеличьте',
        'st.kd': 'Kd (дифференциальный)',
        'st.kdHelp': 'Демпфирование: если колеблется, увеличьте',
        'st.hys': 'Полоса гистерезиса',
        'st.hysHelp': 'Градусы по обе стороны от цели, удерживаемые на поддерживающей мощности до отключения',
        'st.ramp': 'Рампа приближения',
        'st.rampHelp': 'Градусы, на которых мощность снижается от максимума до поддерживающей',
        'st.hold': 'Поддерживающая мощность',
        'st.holdHelp': 'Минимальная мощность (%) вблизи цели для стабильной температуры',
        'st.toffset': 'Коррекция температуры',
        'st.toffsetHelp': 'Отрегулируйте температуру, если она не точная',
        'st.stopDelay': 'Задержка остановки (с)',
        'st.stopDelayHelp': 'Секунды на завершение процесса после прохода конца ленты через датчик',
        'st.maxTime': 'Макс. время (мин)',
        'st.maxTimeHelp': 'Максимальное время работы машины',
        'st.sensorTimeout': 'Таймаут датчика (мин)',
        'st.sensorTimeoutHelp': 'Минуты работы без активности датчика. Если отключено, действует только Макс. время',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Название вашей Wi-Fi (дом / работа)',
        'st.password': 'Пароль SSID',
        'st.passwordHelp': 'Ваш пароль Wi-Fi',
        'st.ip': 'IP-адрес',
        'st.ipHelp': 'DHCP, если пусто. Если ручной IP не задан, проверьте назначенный IP в роутере',
        'st.subnet': 'Маска подсети',
        'st.subnetHelp': '255.255.255.0, если пусто',
        'st.gateway': 'Шлюз',
        'st.gatewayHelp': 'PETALOT не требует подключения к интернету; 0.0.0.0, если пусто',
        'st.analog': 'Аналоговый вход',
        'btn.save': 'Применить',
        'btn.factoryReset': 'Сброс к заводским',
        'msg.minmax': 'мин: {min}, макс: {max}',
        't.running': 'Работает',
        't.connected': 'подключено',
        't.disconnected': 'отключено',
        't.stopped': 'Остановлено',
        't.checkThermistor': 'Проверьте термистор',
        't.speedWarn': 'Скорости >25 см/с могут вызывать дефекты печати. Протестируйте перед серийным производством',
        't.detected': 'обнаружен',
        't.notDetected': 'не обнаружен',
        't.sensorDisabled': 'Датчик отключен',
        't.confirmSave': 'Вы уверены?',
        't.restarting': 'Перезагрузка...',
        't.confirmReset': 'Сброс? Статистика, температура, коррекция температуры и настройка нагрева не будут сброшены',
        't.done': 'Готово',
        'gs.update': 'Обновление прошивки',
        'btn.update': 'Обновить',
        'msg.updating': 'Обновление... не отключайтесь',
        'msg.updateError': 'Ошибка обновления:',
        'st.updOnline': 'Обновление онлайн',
        'st.updCheck': 'Проверить обновления',
        'st.updInstall': 'Установить обновление',
        'msg.updChecking': 'Проверка обновлений...',
        'msg.updAvailable': 'Доступна версия {ver}',
        'msg.updUptodate': 'У вас последняя версия',
        'msg.updOffline': 'Нет подключения к интернету',
        'msg.updInstalling': 'Установка обновления... не отключайте',
        'msg.updateMismatch': 'Эта прошивка не соответствует зарегистрированной версии платы ({pcb}). Установите правильную со страницы /update'
      },
      tr: {
        'gs.title': 'PETALOT Kontrol',
        'gs.ses': 'oturum', 'gs.tot': 'toplam',
        'gs.status': 'Durum', 'gs.speed': 'Hız', 'gs.sensor': 'Sensör', 'gs.temp': 'Sıcaklık',
        'gs.settings': 'Ayarlar',
        'tab.general': 'Genel',
        'tab.run': 'Çalışma',
        'tab.network': 'Ağ',
        'tab.advanced': 'Gelişmiş',
        'st.startOnPower': 'Açılışta çalıştır',
        'st.startOnPowerHelp': 'Kapatırsanız makineyi yalnızca sensöre basarak başlatabilirsiniz',
        'st.motorOnTo': 'Motor hedef sıcaklıkta çalışır',
        'st.motorOnToHelp': 'Etkinse motor yalnızca hedef sıcaklığa ulaşıldığında çalışır',
        'st.display': 'OLED ekran kullan',
        'st.language': 'Dil',
        'st.displayHelp': 'Makinenizde varsa ekranı açın',
        'st.control': 'Isıtma kontrolü',
        'st.controlHelp': 'PID sıcaklığı ayar gerektirmeden sabit tutar; Bang-bang daha basit klasik denetleyicidir',
        'st.kp': 'Kp (orantısal)',
        'st.kpHelp': 'Yanıt hızı: salınım varsa azaltın',
        'st.ki': 'Ki (integral)',
        'st.kiHelp': 'Hedefe ulaşır: altında kalırsa yükseltin',
        'st.kd': 'Kd (türev)',
        'st.kdHelp': 'Sönümleme: salınım varsa yükseltin',
        'st.hys': 'Histerezis bandı',
        'st.hysHelp': 'Hedefin iki yanında kesmeden önce tutma gücü ile sürülen dereceler',
        'st.ramp': 'Yaklaşma rampası',
        'st.rampHelp': 'Gücün maksimumdan tutma seviyesine indiği dereceler',
        'st.hold': 'Tutma gücü',
        'st.holdHelp': 'Hedef yakınında sıcaklığı sabit tutan minimum güç (%)',
        'st.toffset': 'Sıcaklık ofseti',
        'st.toffsetHelp': 'Sıcaklık tam gelmiyorsa ayarlayın',
        'st.stopDelay': 'Durdurma gecikmesi (sn)',
        'st.stopDelayHelp': 'Şerit ucu sensörü geçtikten sonra işlemi bitirmek için geçen saniye',
        'st.maxTime': 'Maks. süre (dk)',
        'st.maxTimeHelp': 'Makinenin maksimum çalışma süresi',
        'st.sensorTimeout': 'Sensör zaman aşımı (dk)',
        'st.sensorTimeoutHelp': 'Sensör etkinliği olmadan çalışma dakikası. Devre dışıysa yalnızca Maks. süre geçerlidir',
        'st.ssid': 'SSID',
        'st.ssidHelp': 'Wi-Fi ağınızın adı (ev / iş)',
        'st.password': 'SSID şifresi',
        'st.passwordHelp': 'Wi-Fi şifreniz',
        'st.ip': 'IP adresi',
        'st.ipHelp': 'Boşsa DHCP kullanılır. Manuel IP ayarlanmadıysa yönlendiricide atanan IP adresini kontrol edin',
        'st.subnet': 'Alt ağ maskesi',
        'st.subnetHelp': 'Boşsa 255.255.255.0',
        'st.gateway': 'Ağ geçidi',
        'st.gatewayHelp': 'PETALOT internet bağlantısı gerektirmez; boşsa 0.0.0.0',
        'st.analog': 'Analog okuma',
        'btn.save': 'Uygula',
        'btn.factoryReset': 'Fabrika ayarları',
        'msg.minmax': 'min: {min}, maks: {max}',
        't.running': 'Çalışıyor',
        't.connected': 'bağlı',
        't.disconnected': 'bağlantı kesildi',
        't.stopped': 'Durduruldu',
        't.checkThermistor': 'Termistörü kontrol edin',
        't.speedWarn': '>25 cm/s hızlar baskı hatalarına neden olabilir. Seri üretimden önce test edin',
        't.detected': 'algılandı',
        't.notDetected': 'algılanmadı',
        't.sensorDisabled': 'Sensör devre dışı',
        't.confirmSave': 'Emin misiniz?',
        't.restarting': 'Yeniden başlatılıyor...',
        't.confirmReset': 'Fabrika ayarlarına sıfırlansın mı? İstatistikler, sıcaklık, sıcaklık ofseti ve ısıtma ayarı sıfırlanmaz',
        't.done': 'Tamam',
        'gs.update': 'Bellenim güncellemesi',
        'btn.update': 'Güncelle',
        'msg.updating': 'Güncelleniyor... bağlantıyı kesme',
        'msg.updateError': 'Güncelleme hatası:',
        'st.updOnline': 'Çevrimiçi güncelleme',
        'st.updCheck': 'Güncellemeleri kontrol et',
        'st.updInstall': 'Güncellemeyi yükle',
        'msg.updChecking': 'Güncellemeler kontrol ediliyor...',
        'msg.updAvailable': '{ver} sürümü mevcut',
        'msg.updUptodate': 'En son sürümdesiniz',
        'msg.updOffline': 'İnternet bağlantısı yok',
        'msg.updInstalling': 'Güncelleme yükleniyor... bağlantıyı kesme',
        'msg.updateMismatch': 'Bu bellenim kayıtlı PCB sürümüyle ({pcb}) eşleşmiyor. Doğru sürümü /update sayfasından yükleyin'
      },
      ja: {
        'gs.title': 'PETALOT コントロール',
        'gs.ses': '今回', 'gs.tot': '累計',
        'gs.status': '状態', 'gs.speed': '速度', 'gs.sensor': 'センサー', 'gs.temp': '温度',
        'gs.settings': '設定',
        'tab.general': '一般',
        'tab.run': '動作',
        'tab.network': 'ネットワーク',
        'tab.advanced': '詳細',
        'st.startOnPower': '電源投入時に起動',
        'st.startOnPowerHelp': '無効にすると、センサーを押すことでのみ機械を起動できます',
        'st.motorOnTo': '目標温度でモーター起動',
        'st.motorOnToHelp': '有効にすると、目標温度に達した時のみモーターが回転します',
        'st.display': 'OLEDディスプレイを使用',
        'st.language': '言語',
        'st.displayHelp': '機械にディスプレイがあればオンにします',
        'st.control': '加熱制御',
        'st.controlHelp': 'PID は調整なしで温度を安定に保ちます; バンバンはより単純な古典的制御器です',
        'st.kp': 'Kp（比例）',
        'st.kpHelp': '応答速度：振動する場合は下げる',
        'st.ki': 'Ki（積分）',
        'st.kiHelp': '目標に届く：下回ったままなら上げる',
        'st.kd': 'Kd（微分）',
        'st.kdHelp': '減衰：振動する場合は上げる',
        'st.hys': 'ヒステリシス帯',
        'st.hysHelp': '目標の両側でカット前に保持出力を継続する度数',
        'st.ramp': '接近ランプ',
        'st.rampHelp': '出力が最大から保持出力まで下がる度数',
        'st.hold': '保持出力',
        'st.holdHelp': '目標付近で温度を安定に保つ最小出力 (%)',
        'st.toffset': '温度オフセット',
        'st.toffsetHelp': '温度がずれている場合に調整します',
        'st.stopDelay': '停止遅延（秒）',
        'st.stopDelayHelp': 'テープの先端がセンサーを通過した後の処理終了までの秒数',
        'st.maxTime': '最大時間（分）',
        'st.maxTimeHelp': '機械の最大運転時間',
        'st.sensorTimeout': 'センサータイムアウト（分）',
        'st.sensorTimeoutHelp': 'センサー動作なしで運転する分数。無効の場合は最大時間のみ適用',
        'st.ssid': 'SSID',
        'st.ssidHelp': '自宅・職場のWi-Fi名',
        'st.password': 'SSIDパスワード',
        'st.passwordHelp': 'Wi-Fiパスワード',
        'st.ip': 'IPアドレス',
        'st.ipHelp': '空欄ならDHCP。手動IP未設定の場合はルーターで割り当て済みのIPを確認してください',
        'st.subnet': 'サブネットマスク',
        'st.subnetHelp': '空欄なら255.255.255.0',
        'st.gateway': 'ゲートウェイ',
        'st.gatewayHelp': 'PETALOTはインターネット接続を必要としません。空欄なら0.0.0.0',
        'st.analog': 'アナログ読取',
        'btn.save': '適用',
        'btn.factoryReset': '工場出荷時リセット',
        'msg.minmax': '最小：{min}、最大：{max}',
        't.running': '運転中',
        't.connected': '接続済み',
        't.disconnected': '切断中',
        't.stopped': '停止中',
        't.checkThermistor': 'サーミスタを確認してください',
        't.speedWarn': '速度が25cm/sを超えると印刷不良になる可能性があります。量産前にテストしてください',
        't.detected': '検出',
        't.notDetected': '未検出',
        't.sensorDisabled': 'センサー無効',
        't.confirmSave': 'よろしいですか？',
        't.restarting': '再起動中...',
        't.confirmReset': '工場出荷時リセットしますか？統計、温度、温度オフセット、加熱調整 はリセットされません',
        't.done': '完了',
        'gs.update': 'ファームウェア更新',
        'btn.update': '更新',
        'msg.updating': '更新中……接続を切らないでください',
        'msg.updateError': '更新エラー：',
        'st.updOnline': 'オンライン更新',
        'st.updCheck': '更新を確認',
        'st.updInstall': '更新をインストール',
        'msg.updChecking': '更新を確認中……',
        'msg.updAvailable': 'バージョン {ver} が利用可能です',
        'msg.updUptodate': '最新バージョンです',
        'msg.updOffline': 'インターネット接続がありません',
        'msg.updInstalling': '更新をインストール中……接続を切らないでください',
        'msg.updateMismatch': 'このファームウェアは登録されたPCBバージョン（{pcb}）と一致しません。/updateページから正しいバージョンをインストールしてください'
      },
      ko: {
        'gs.title': 'PETALOT 제어',
        'gs.ses': '세션', 'gs.tot': '합계',
        'gs.status': '상태', 'gs.speed': '속도', 'gs.sensor': '센서', 'gs.temp': '온도',
        'gs.settings': '설정',
        'tab.general': '일반',
        'tab.run': '작동',
        'tab.network': '네트워크',
        'tab.advanced': '고급',
        'st.startOnPower': '전원 켜짐 시 시작',
        'st.startOnPowerHelp': '비활성화하면 센서를 눌러서만 기기를 시작할 수 있습니다',
        'st.motorOnTo': '목표 온도에서 모터 시작',
        'st.motorOnToHelp': '활성화하면 목표 온도에 도달했을 때만 모터가 회전합니다',
        'st.display': 'OLED 디스플레이 사용',
        'st.language': '언어',
        'st.displayHelp': '기기에 디스플레이가 있으면 켭니다',
        'st.control': '가열 제어',
        'st.controlHelp': 'PID는 보정 없이 온도를 안정적으로 유지합니다; Bang-bang은 더 단순한 고전적 제어기입니다',
        'st.kp': 'Kp(비례)',
        'st.kpHelp': '반응 속도: 진동하면 낮추세요',
        'st.ki': 'Ki(적분)',
        'st.kiHelp': '목표 도달: 부족하면 높이세요',
        'st.kd': 'Kd(미분)',
        'st.kdHelp': '감쇠: 진동하면 높이세요',
        'st.hys': '히스테리시스 대역',
        'st.hysHelp': '목표 온도 양쪽에서 차단 전까지 유지 출력으로 유지되는 각도',
        'st.ramp': '접근 램프',
        'st.rampHelp': '출력이 최대에서 유지 출력으로 줄어드는 각도',
        'st.hold': '유지 출력',
        'st.holdHelp': '목표 근처에서 온도를 안정적으로 유지하는 최소 출력 (%)',
        'st.toffset': '온도 오프셋',
        'st.toffsetHelp': '온도가 맞지 않으면 조정합니다',
        'st.stopDelay': '정지 지연（초）',
        'st.stopDelayHelp': '테이프 끝이 센서를 통과한 후 공정을 마치는 시간（초）',
        'st.maxTime': '최대 시간（분）',
        'st.maxTimeHelp': '기기의 최대 작동 시간',
        'st.sensorTimeout': '센서 시간 초과（분）',
        'st.sensorTimeoutHelp': '센서 활동 없이 작동할 시간（분）. 비활성이면 최대 시간만 적용됩니다',
        'st.ssid': 'SSID',
        'st.ssidHelp': '집·직장 Wi-Fi 이름',
        'st.password': 'SSID 비밀번호',
        'st.passwordHelp': 'Wi-Fi 비밀번호',
        'st.ip': 'IP 주소',
        'st.ipHelp': '비우면 DHCP를 사용합니다. 수동 IP가 설정되지 않은 경우 라우터에서 할당된 IP를 확인하세요',
        'st.subnet': '서브넷 마스크',
        'st.subnetHelp': '비우면 255.255.255.0',
        'st.gateway': '게이트웨이',
        'st.gatewayHelp': 'PETALOT은 인터넷 연결이 필요하지 않습니다. 비우면 0.0.0.0',
        'st.analog': '아날로그 판독',
        'btn.save': '적용',
        'btn.factoryReset': '공장 초기화',
        'msg.minmax': '최소：{min}、최대：{max}',
        't.running': '작동 중',
        't.connected': '연결됨',
        't.disconnected': '연결 끊김',
        't.stopped': '정지됨',
        't.checkThermistor': '서미스터를 확인하세요',
        't.speedWarn': '25cm/s를 초과하는 속도는 인쇄 불량을 유발할 수 있습니다. 양산 전에 테스트하세요',
        't.detected': '감지됨',
        't.notDetected': '감지 안 됨',
        't.sensorDisabled': '센서 비활성',
        't.confirmSave': '확실합니까？',
        't.restarting': '재시작 중...',
        't.confirmReset': '공장 초기화하시겠습니까？통계、온도、온도 오프셋、가열 튜닝 은 초기화되지 않습니다',
        't.done': '완료',
        'gs.update': '펌웨어 업데이트',
        'btn.update': '업데이트',
        'msg.updating': '업데이트 중……연결을 끊지 마세요',
        'msg.updateError': '업데이트 오류：',
        'st.updOnline': '온라인 업데이트',
        'st.updCheck': '업데이트 확인',
        'st.updInstall': '업데이트 설치',
        'msg.updChecking': '업데이트 확인 중……',
        'msg.updAvailable': '버전 {ver} 사용 가능',
        'msg.updUptodate': '최신 버전입니다',
        'msg.updOffline': '인터넷 연결 없음',
        'msg.updInstalling': '업데이트 설치 중……연결을 끊지 마세요',
        'msg.updateMismatch': '이 펌웨어는 등록된 PCB 버전（{pcb}）과 일치하지 않습니다. /update 페이지에서 올바른 버전을 설치하세요'
      }
    };

    const LANGS = { en: 'English', es: 'Español', pt: 'Português', fr: 'Français', de: 'Deutsch', it: 'Italiano', zh: '中文', cs: 'Čeština', ru: 'Русский', tr: 'Türkçe', ja: '日本語', ko: '한국어' };

    let lang = 'en';
    let connOnline = false;
    let teleInFlight = false;
    let teleTimer = null;

    function scheduleTele() {
      if (teleTimer) clearTimeout(teleTimer);
      teleTimer = setTimeout(() => { teleTimer = null; fetchTele(); }, 2000);
    }

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
      setConnText();
    }

    function setConnText() {
      const el = document.getElementById('conn-text');
      if (!el) return;
      el.textContent = connOnline ? t('t.connected') : t('t.disconnected');
      el.classList.toggle('conn-up', connOnline);
      el.classList.toggle('conn-down', !connOnline);
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
      if (teleInFlight) return;
      teleInFlight = true;

      const abortCtrl = new AbortController();
      const abortTimer = setTimeout(() => abortCtrl.abort(), 5000);

      fetch('/tele', { signal: abortCtrl.signal })
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
          document.getElementById('val-output').innerText = (data.Output !== undefined && data.Output !== '') ? '(' + data.Output + ')' : '';

          document.getElementById('val-speed').innerText = data.Vo;
          document.getElementById('warn-speed').innerText = (data.Vo>25) ? t('t.speedWarn') : '';

          document.getElementById('val-filament').innerText = data.F ? t('t.detected') : t('t.notDetected');
          document.getElementById('ctrl-filament').checked = data.Fenable;
          document.getElementById('warn-filament').innerText = (!data.Fenable) ? t('t.sensorDisabled') : '';

          document.getElementById('tele-AR').value = data.AR || 0;

          updateIcons(data);
        })
        .catch(err => {
          document.getElementById('conn-icon').classList.remove('conn-on');
          document.getElementById('conn-icon').classList.add('conn-off');
          connOnline = false;
          setConnText();
          const fireIcon = document.getElementById('fire-icon');
          const motorIcon = document.getElementById('motor-icon');
          fireIcon.classList.remove('fire-on');
          fireIcon.classList.add('fire-off');
          motorIcon.classList.remove('motor-on');
          motorIcon.classList.add('motor-off');
        })
        .finally(() => {
          clearTimeout(abortTimer);
          teleInFlight = false;
          scheduleTele();
        });
    }

    function sendAction(param, value) {
      fetch(`/set?${param}=${value}`).then(res => res.json()).then(() => fetchTele());
    }

    let conf = {};
    let updatePcbVer = '';

    function fetchConf() {
      fetch('/get')
        .then(res => res.json())
        .then(data => {
          conf = data;
          if (data.version) {
            let versionParts = data.version.split('.');
            let semanticVersion = parseInt(versionParts[0] + versionParts[1] + versionParts[2]);

            const fwVer = versionParts.slice(0, 3).join('.');
            const hwVer = data.pcbVer ? formatVersion(data.pcbVer) : '';
            document.getElementById('version').innerText = hwVer ? `v${fwVer} (PCB ${hwVer})` : `v${fwVer}`;

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

          if (data.pcbVer) {
            updatePcbVer = data.pcbVer;
          }

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
          updateControlFields();

        });
    }

    function showSettingsTab(tab) {
      const valid = ['general', 'run', 'network', 'advanced'].includes(tab) ? tab : 'general';
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.toggle('active', b.dataset.tab === valid));
      document.querySelectorAll('.tab-panel').forEach(p => p.classList.toggle('active', p.id === 'tab-' + valid));
      try { localStorage.setItem('petalot-tab', valid); } catch (e) {}
    }

    function updateControlFields() {
      const sel = document.getElementsByName('ControlMode')[0];
      if (!sel) return;
      const mode = (sel.value === '1') ? 'bang' : 'pid';
      document.querySelectorAll('[data-mode]').forEach(el => {
        el.style.display = (el.dataset.mode === mode) ? '' : 'none';
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

    function startUpdate() {
      const input = document.getElementById('up-firmware');
      const file = input.files[0];
      const msgEl = document.getElementById('update-msg');
      if (!file) return;

      msgEl.className = 'msg';
      msgEl.textContent = t('msg.updating');
      const fd = new FormData();
      fd.append('firmware', file);

      fetch('/updatecheck', { method: 'POST', body: fd })
        .then(r => {
          if (r.ok) {
            fetch('/set?status=0');
            doRealUpdate(file, msgEl);
          } else {
            msgEl.className = 'msg warn';
            msgEl.textContent = t('msg.updateMismatch', {pcb: updatePcbVer});
          }
        })
        .catch(() => waitForReboot());
    }

    function doRealUpdate(file, msgEl) {
      const fd = new FormData();
      fd.append('firmware', file);
      fetch('/update?name=firmware', { method: 'POST', body: fd })
        .then(r => r.text())
        .then(txt => {
          if (txt.indexOf('Success') !== -1 || txt.indexOf('Reboot') !== -1) {
            waitForReboot();
          } else {
            msgEl.className = 'msg warn';
            msgEl.textContent = (txt.trim() !== '') ? t('msg.updateError') + ' ' + txt : t('msg.updateError');
          }
        })
        .catch(() => waitForReboot());
    }

    function waitForReboot(attempts = 20) {
      fetch('/tele')
        .then(() => {
          // The device answers, but its web server may still be initializing
          // (and other polls compete for sockets). Wait, then only reload once
          // the full page is available, to avoid rendering an unstyled page.
          setTimeout(() => loadWhenReady(), 2000);
        })
        .catch(() => {
          if (attempts > 0) setTimeout(() => waitForReboot(attempts - 1), 2000);
        });
    }

    function loadWhenReady(attempts = 20) {
      fetch('/', { cache: 'no-store' })
        .then(r => r.text())
        .then(html => {
          if (html.indexOf('</html>') !== -1) {
            window.location.reload();
          } else if (attempts > 0) {
            setTimeout(() => loadWhenReady(attempts - 1), 1500);
          }
        })
        .catch(() => {
          if (attempts > 0) setTimeout(() => loadWhenReady(attempts - 1), 1500);
        });
    }

    // --- Online update (GitHub manifest) -------------------------------------
    // --- Online update ------------------------------------------------------
    // The browser is the one with Internet: it reads the manifest and downloads
    // the .bin from GitHub, then uploads it to the device through the same
    // /updatecheck + /update path as the manual update. The device never does
    // any HTTPS request, so there is no TLS/watchdog strain on it.
    const UPDATE_BASE = 'https://raw.githubusercontent.com/function3d/petalot/master/Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/';
    let onlineUpdateUrl = '';

    function formatVersion(v) {
      if (!v) return '';
      const major = Math.floor(v / 1000);
      const minor = Math.floor((v % 1000) / 100);
      const patch = v % 100;
      return major + '.' + minor + '.' + patch;
    }

    function setUpdateStatus(text, warn) {
      const status = document.getElementById('upd-status');
      if (!status) return;
      status.className = warn ? 'msg warn' : 'msg';
      status.textContent = text || '';
    }

    function checkOnlineUpdate() {
      const checkBtn = document.getElementById('btn-check-upd');
      const installBtn = document.getElementById('btn-install-upd');
      onlineUpdateUrl = '';
      if (checkBtn) checkBtn.style.display = 'none';
      if (installBtn) installBtn.style.display = 'none';
      setUpdateStatus(t('msg.updChecking'), false);

      fetch(UPDATE_BASE + 'latest.json', { cache: 'no-store' })
        .then(r => r.json())
        .then(manifest => {
          const entry = manifest[updatePcbVer];
          if (!entry) {
            setUpdateStatus(t('msg.updError') + ' ' + updatePcbVer, true);
            if (checkBtn) checkBtn.style.display = '';
            return;
          }
          const vp = conf.version.split('.');
          const current = parseInt(vp[0]) * 1000 + parseInt(vp[1]) * 100 + parseInt(vp[2]);
          if (entry.version > current) {
            onlineUpdateUrl = UPDATE_BASE + entry.file;
            setUpdateStatus(t('msg.updAvailable', {ver: formatVersion(entry.version)}), false);
            if (installBtn) installBtn.style.display = '';
          } else {
            setUpdateStatus(t('msg.updUptodate'), false);
            if (checkBtn) checkBtn.style.display = '';
          }
        })
        .catch(() => {
          setUpdateStatus(t('msg.updOffline'), true);
          if (checkBtn) checkBtn.style.display = '';
        });
    }

    function installOnlineUpdate() {
      const installBtn = document.getElementById('btn-install-upd');
      if (installBtn) installBtn.style.display = 'none';
      setUpdateStatus(t('msg.updInstalling'), false);
      // 1) browser downloads the new firmware from GitHub
      // 2) browser uploads it to the device, exactly like a manual update
      fetch(onlineUpdateUrl, { cache: 'no-store' })
        .then(r => r.blob())
        .then(blob => {
          const file = new File([blob], 'petalot.bin', { type: 'application/octet-stream' });
          return uploadFirmware(file);
        })
        .catch(() => setUpdateStatus(t('msg.updError'), true));
    }

    function uploadFirmware(file) {
      const msgEl = document.getElementById('upd-status');
      const fd = new FormData();
      fd.append('firmware', file);
      return fetch('/updatecheck', { method: 'POST', body: fd })
        .then(r => {
          if (!r.ok) {
            msgEl.className = 'msg warn';
            msgEl.textContent = t('msg.updateMismatch', {pcb: updatePcbVer});
            return;
          }
          const fd2 = new FormData();
          fd2.append('firmware', file);
          fetch('/set?status=0');
          return fetch('/update?name=firmware', { method: 'POST', body: fd2 })
            .then(res => res.text())
            .then(txt => {
              if (txt.indexOf('Success') !== -1 || txt.indexOf('Reboot') !== -1) {
                waitForReboot(60);
              } else {
                msgEl.className = 'msg warn';
                msgEl.textContent = (txt.trim() !== '') ? t('msg.updateError') + ' ' + txt : t('msg.updateError');
              }
            });
        })
        .catch(() => waitForReboot(60));
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
      connOnline = true;
      setConnText();

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
      showSettingsTab((() => { try { return localStorage.getItem('petalot-tab'); } catch (e) { return null; } })());
      updateControlFields();
      fetchConf();
      fetchTele();
      updateNetworkFields();

      inputSSID.addEventListener('input', updateNetworkFields);
          };
  </script>
</body>
</html>
)rawliteral";
