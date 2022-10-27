#include <ESP8266WebServer.h>


ESP8266WebServer server(80);


static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <script src="https://cdn.tailwindcss.com"></script>
    <style type="text/tailwindcss">
    @layer components {
      .card {
        @apply mx-2 px-4 relative flex flex-col min-w-0 grow break-words bg-white rounded mt-4 xl:mb-0 shadow-lg p-4;
      }
        .card .title{
        @apply text-slate-400 font-bold text-xs;
        }
        .card .value{
        @apply font-semibold text-xl text-slate-700 inline;
        }

    }
  </style>
  <script defer src="https://unpkg.com/alpinejs@3.x.x/dist/cdn.min.js"></script>
</head>
<body class="bg-slate-600" x-data="petalot()"
     x-init="
    fetchConf('get')
    fetchTele('tele')
    setInterval(function(){ fetchTele('tele') }, 1000)
    ">
    <div class="w-11/12 lg:w-1/2 mx-auto">
        <div class="card font-bold text-xl flex-row" href="#pablo">PETALOT <span class="text-slate-400 text-sm m-1.5">by @function.3d</span></div>
        <div class="flex flex-col sm:flex-row mx-auto items-stretch">
          
            <template x-for="el in ui">
                <div :class="el.type">
                    <div class="title" x-text="el.title"></div>

                    <div><div class="value" x-text="(el.value_type=='bool')?tele[el.value]?el.true:el.false:tele[el.value]"></div> <span x-text="el.unit"></span>
                        <label x-show="el.toggle" :for="'toggle-'+el.title" class="float-right inline-flex relative items-center cursor-pointer">
                      <input @click="fetchTele('set?'+el.value+'='+($event.target.checked?1:0))" x-model="tele[el.value]" type="checkbox" :id="'toggle-'+el.title" class="sr-only peer" >
                      <div class="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-slate-300 dark:peer-focus:ring-slate-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-slate-600"></div>
                    
                    </label>
                    </div>
                    
                </div>
            </template>
        </div>
        <div x-data="{show:false}" class="card">
            <div  class="cursor-pointer hover:opacity-75 font-bold text-xl mb-4" @click="show = !show">Settings</div>
            <div x-show="show">
            <template x-for="[entry,value] in Object.entries(conf)" >
                <div >
                    <div class="title" x-text="entry"></div>
                    <input class="shadow appearance-none border mb-4 rounded w-full py-2 px-3 text-gray-700 leading-tight focus:outline-none focus:shadow-outline" x-model="conf[entry]">
                    
                </div>        
            </template>
            <button x-text="save" class="bg-slate-500 hover:bg-slate-700 text-white font-bold py-2 px-4 rounded float-right" @click="fetchConf('set?'+ new URLSearchParams(conf));save='Saved!';setInterval(function(){ save='Save' }, 3000) ">
              Save
            </button>
            </div>
        </div>
    </div>
<script>
function petalot() {
    return {
        tele : {
            V: 0,
            T: 0,
            F: 0,
            status: 0,
        },
        save:'Save',
        conf:{},
        fetchConf(url){
            fetch(url)
                .then(response=> {
                    return response.json()
                })
                .then((data) => {
                    this.conf = data;
                    this.conf.Vo = this.conf.Vo/2;
                 })
        },
        fetchTele(url){
            fetch(url)
                .then(response=> {
                    return response.json()
                })
                .then((data) => {
                    this.tele.V = data.V/2,
                    this.tele.T= Math.round(data.T),
                    this.tele.F= data.F,
                    this.tele.status = data.status
                 })
        },
        ui : [
            { type: 'card', title: 'Status', value: 'status', unit:'', value_type:'bool', toggle:true, true:'working', false:'stopped' },
            { type: 'card', title: 'Temperature', value: 'T', unit:'°C' },
            { type: 'card', title: 'Speed', value: 'V', unit:'cm/min' },
            { type: 'card', title: 'Filament', value: 'F' , unit:'', value_type:'bool', true:'detected', false:'no detected' }



            
        ],
    }
}
</script>
</body>
</html>
)rawliteral";

static const char PROGMEM INDEX_HTML_APMODE[] = R"rawliteral(
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
    <form style="width: 50%; min-width:250px;margin:0 auto;display:flex; flex-direction:column" action="set">
<h2>Wifi parameters</h2>
    <div>ssid</div><input id="ssid" name="ssid">
    <br><div>password</div><input id="password" name="password">
    <br><div>ip</div><input id="LocalIP" name="LocalIP">
    <br><div>gateway</div><input id="gateway" name="gateway">
    <br><div>subnet</div><input id="Subnet" name="Subnet">
    <br><input type="submit" value="Save">
    </form>        
</body>
</html>
)rawliteral";


void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void tele() {
  String r;
  r = String("{") +
      "\"time\":" + String(millis()) +
      ",\"status\":" + (status=="working"?"true":"false") +
      ",\"T\":" + String(T) +
      //",\"To\":" + String(To) +
      ",\"V\":" + String(V) +
      ",\"F\":" + String(F) +
      //",\"Fc\":" + String(Fc) +
      //",\"Fi\":" + String(Fi) +
      //",\"tempLastStart\":" + String(tempLastStart) +
      //",\"tempLastFilament\":" + String(tempLastFilament) +
      ",\"Output\":" + Output +  //String(map(Output, 0, 255, 0, 100))
      //",\"msg\":\"" + msg + "\"" +
      //",\"config\":"+ printConf() +
      "}";
  server.send(200, "text/html", r);
}
void get(){
  server.send(200, "text/html", printConf());
}
void set() {
  bool save = false;
  bool reboot = false;
  
  String ToChange = server.arg(String("To"));
  if (ToChange != "" && ToChange.toFloat() <= Tm && ToChange.toFloat() >= 120) {
    save = true;
    To = ToChange.toInt();
    //if (To!=0) Tco = To;
  }
  String VoChange = server.arg(String("Vo"));
  if (VoChange != "" && VoChange.toFloat() <= 20 && VoChange.toFloat() >= 5) {
    save = true;
    Vo = VoChange.toInt()*2;
    //if (Vo!=0) Vco = Vo;
  }
  String KpChange = server.arg(String("Kp"));
  if (KpChange != "") {
    save = true;
    Kp = KpChange.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
  }
  String KiChange = server.arg(String("Ki"));
  if (KiChange != "") {
    save = true;
    Ki = KiChange.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
  }
  String KdChange = server.arg(String("Kd"));
  if (KdChange != "") {
    save = true;
    Kd = KdChange.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
  }
  String MaxChange = server.arg(String("Max"));
  if (MaxChange != "") {
    save = true;
    Max = MaxChange.toDouble();
    myPID.SetOutputLimits(0,Max);
  }
  String TmChange = server.arg(String("Tm"));
  if (TmChange != "" && TmChange.toFloat() <= 270 && TmChange.toFloat() >= 150) {
    save = true;
    Tm = TmChange.toFloat();
  }
  String statusChange = server.arg(String("status"));
  if (statusChange != "") {
    if (statusChange.toFloat())
      start();
     else
      stop();
  }
  String R1Change = server.arg(String("R1"));
  if (R1Change != "") {
    save = true;
    R1 = R1Change.toInt();
  }
  String ssidChange = server.arg(String("ssid"));
  if (ssidChange != "") {
    save = true;
    ssidChange.toCharArray(ssid, sizeof(ssid)); 
  }
  String passwordChange = server.arg(String("password"));
  if (passwordChange != "") {
    save = true;
    passwordChange.toCharArray(password, sizeof(password)); 
  }
  String _ifttt_event_name = server.arg(String("ifttt_event_name"));
  if (_ifttt_event_name!="") {
    save = true;
    ifttt_event_name = _ifttt_event_name;
  }
  String _ifttt_api_key = server.arg(String("ifttt_api_key"));
  if (_ifttt_api_key!="") {
    save = true;
    ifttt_api_key = _ifttt_api_key;
  }
  String LocalIPChange = server.arg(String("LocalIP"));
  if (LocalIPChange!="") {
    save = true;
    reboot = true;
    LocalIP = LocalIPChange;
  }
  String SubnetChange = server.arg(String("Subnet"));
  if (SubnetChange!="") {
    save = true;
    reboot = true;
    Subnet = SubnetChange;
  }
  String gatewayChange = server.arg(String("gateway"));
  if (gatewayChange!="") {
    save = true;
    reboot = true;
    gateway = gatewayChange;
  }
  
  
  if (save) {
     saveConfiguration();
     if (apmode || reboot)
      ESP.restart();
      else 
      get();
     return;
     
  }

  tele();
  
}

void handleRoot()
{
  if (apmode)
    server.send_P(200, "text/html", INDEX_HTML_APMODE);
  else
    server.send_P(200, "text/html", INDEX_HTML);
}



void InitServer()
{
  server.on("/", handleRoot);
  server.on("/get", get);
  server.on("/tele", set);
  server.on("/set", set);
  server.onNotFound([]() {
      handleNotFound();             
  });
  server.enableCORS(true);
  server.begin();
}
