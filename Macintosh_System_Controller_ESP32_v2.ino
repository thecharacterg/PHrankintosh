
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 48
#define NUM_PIXELS 1

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* hostname = "macintosh-controller";

WebServer server(80);
Adafruit_NeoPixel pixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

bool hardwareMode = true;
uint8_t ledR = 0, ledG = 50, ledB = 0;

struct FanState {
  bool enabled;
  bool automatic;
  uint8_t speed;
  uint16_t rpm;
};

FanState fans[4] = {
  {true,false,30,1100},
  {true,false,30,1080},
  {true,false,30,1120},
  {false,false,0,0}
};

float controllerTemp = 31.2;
float buckTemp = 35.4;
float caseTemp = 27.8;
float humidity = 42.0;
float piTemp = 47.3;

char heartbeat = 'A';
bool piAlive = true;
bool piShuttingDown = false;
unsigned long lastPiUpdate = 0;

void applyPixel() {
  if (!hardwareMode) return;
  pixel.setPixelColor(0, pixel.Color(ledR, ledG, ledB));
  pixel.show();
}

String b(bool v) { return v ? "true" : "false"; }

String statusJson() {
  String s = "{";
  s += "\"mode\":\"" + String(hardwareMode ? "hardware" : "test") + "\",";
  s += "\"led\":{\"r\":" + String(ledR) + ",\"g\":" + String(ledG) + ",\"b\":" + String(ledB) + "},";
  s += "\"pi\":{\"alive\":" + b(piAlive) + ",\"shutdown\":" + b(piShuttingDown) +
       ",\"heartbeat\":\"" + String(heartbeat) + "\",\"temperature\":" + String(piTemp,1) + "},";
  s += "\"temps\":{\"controller\":" + String(controllerTemp,1) +
       ",\"buck\":" + String(buckTemp,1) +
       ",\"case\":" + String(caseTemp,1) +
       ",\"humidity\":" + String(humidity,1) + "},";
  s += "\"fans\":[";
  for (int i=0;i<4;i++) {
    if (i) s += ",";
    s += "{\"enabled\":" + b(fans[i].enabled) +
         ",\"automatic\":" + b(fans[i].automatic) +
         ",\"speed\":" + String(fans[i].speed) +
         ",\"rpm\":" + String(fans[i].rpm) + "}";
  }
  s += "]}";
  return s;
}

const char PAGE[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Macintosh System Controller</title>
<style>
body{margin:0;background:#d8d8d8;font-family:Geneva,Arial,sans-serif;padding:20px}
.window{max-width:760px;margin:auto;background:#fff;border:2px solid #000;box-shadow:5px 5px 0 #000}
.title{padding:12px;text-align:center;font-size:22px;font-weight:bold;border-bottom:2px solid #000}
.tabs{display:flex;gap:6px;flex-wrap:wrap;padding:10px;background:#eee;border-bottom:1px solid #000}
button,select,input{font:inherit}button{padding:8px 14px;border:1px solid #000;background:#eee}
button.active{background:#000;color:#fff}
.panel{display:none;padding:18px}.panel.active{display:block}
.box{border:1px solid #000;padding:12px;margin-bottom:14px}
.row{display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin:8px 0}
.label{min-width:145px;font-weight:bold}
input[type=range]{width:240px;max-width:100%}
input[type=color]{width:80px;height:48px}
.mode{border:2px solid #000;padding:10px;margin-bottom:14px;font-weight:bold}
.small{font-size:13px}
</style></head><body>
<div class="window">
<div class="title">Macintosh System Controller</div>
<div class="tabs">
<button class="tab active" data-tab="overview">Overview</button>
<button class="tab" data-tab="fans">Fans</button>
<button class="tab" data-tab="led">LED</button>
<button class="tab" data-tab="system">System</button>
</div>

<div id="overview" class="panel active">
<div class="mode">Mode: <span id="mode">...</span> <button id="toggleMode">Toggle Test / Hardware</button></div>
<div class="box"><h3>Pi Status</h3>
<div class="row"><span class="label">State</span><span id="piState"></span></div>
<div class="row"><span class="label">Heartbeat</span><span id="hb"></span></div>
<div class="row"><span class="label">Pi Temperature</span><span id="piTemp"></span></div></div>
<div class="box"><h3>Temperatures</h3>
<div class="row"><span class="label">Controller</span><span id="ct"></span></div>
<div class="row"><span class="label">Buck</span><span id="bt"></span></div>
<div class="row"><span class="label">Case</span><span id="caseT"></span></div>
<div class="row"><span class="label">Humidity</span><span id="hum"></span></div></div>
<div class="box"><h3>Fan Overview</h3><div id="fanOverview"></div></div>
</div>

<div id="fans" class="panel"><h3>Fan Control</h3><div id="fanControls"></div></div>

<div id="led" class="panel"><h3>Status LED</h3><div class="box">
<div class="row">
<button onclick="setColor(255,0,0)">Red</button>
<button onclick="setColor(0,255,0)">Green</button>
<button onclick="setColor(0,0,255)">Blue</button>
<button onclick="setColor(255,255,255)">White</button>
<button onclick="setColor(0,0,0)">Off</button></div>
<div class="row"><input type="color" id="picker" value="#00ff00"><button onclick="pickerColor()">Set Color</button></div>
<div class="row"><span class="label">Current RGB</span><span id="rgb"></span></div>
</div></div>

<div id="system" class="panel"><h3>Pi Supervisor Test</h3><div class="box">
<div class="row">
<button onclick="hit('/api/sim/heartbeat')">Advance A-O Heartbeat</button>
<button onclick="hit('/api/sim/shutdown')">Send P / Shutdown</button>
<button onclick="hit('/api/sim/lost')">Simulate Lost Pi</button>
</div><p class="small">These are bench-test controls. Later the Pi USB serial service supplies the real heartbeat, temperature and shutdown state.</p>
</div></div>
</div>

<script>
document.querySelectorAll('.tab').forEach(btn=>btn.addEventListener('click',()=>{
 document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
 document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));
 btn.classList.add('active'); document.getElementById(btn.dataset.tab).classList.add('active');
}));

async function hit(url){await fetch(url);refresh();}
async function setColor(r,g,b){await hit(`/api/led?r=${r}&g=${g}&b=${b}`)}
function pickerColor(){
 const c=document.getElementById('picker').value;
 setColor(parseInt(c.slice(1,3),16),parseInt(c.slice(3,5),16),parseInt(c.slice(5,7),16));
}
document.getElementById('toggleMode').onclick=()=>hit('/api/mode/toggle');

async function setFan(n,key,val){await hit(`/api/fan?fan=${n}&${key}=${val}`)}

function drawFans(fans){
 let o='',c='';
 fans.forEach((f,i)=>{
  o+=`<div class="row"><span class="label">Fan ${i+1}</span><span>${f.enabled?f.speed+'% / '+f.rpm+' RPM':'OFF'}</span></div>`;
  c+=`<div class="box"><h3>Fan ${i+1}</h3>
  <div class="row"><span class="label">Enabled</span><input type="checkbox" ${f.enabled?'checked':''} onchange="setFan(${i+1},'enabled',this.checked?1:0)"></div>
  <div class="row"><span class="label">Mode</span><select onchange="setFan(${i+1},'auto',this.value)">
  <option value="0" ${!f.automatic?'selected':''}>Manual</option><option value="1" ${f.automatic?'selected':''}>Automatic</option></select></div>
  <div class="row"><span class="label">Speed</span><input type="range" min="0" max="100" value="${f.speed}" onchange="setFan(${i+1},'speed',this.value)"><span>${f.speed}%</span></div>
  <div class="row"><span class="label">RPM</span><span>${f.rpm}</span></div></div>`;
 });
 document.getElementById('fanOverview').innerHTML=o;
 document.getElementById('fanControls').innerHTML=c;
}

async function refresh(){
 try{
  const s=await (await fetch('/api/status')).json();
  document.getElementById('mode').textContent=s.mode.toUpperCase();
  document.getElementById('rgb').textContent=`${s.led.r}, ${s.led.g}, ${s.led.b}`;
  document.getElementById('hb').textContent=s.pi.heartbeat;
  document.getElementById('piTemp').textContent=s.pi.temperature.toFixed(1)+' °C';
  document.getElementById('piState').textContent=s.pi.shutdown?'SHUTTING DOWN':(!s.pi.alive?'COMMUNICATION LOST':'RUNNING');
  document.getElementById('ct').textContent=s.temps.controller.toFixed(1)+' °C';
  document.getElementById('bt').textContent=s.temps.buck.toFixed(1)+' °C';
  document.getElementById('caseT').textContent=s.temps.case.toFixed(1)+' °C';
  document.getElementById('hum').textContent=s.temps.humidity.toFixed(1)+' %';
  drawFans(s.fans);
 }catch(e){}
}
setInterval(refresh,1000);refresh();
</script></body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  pixel.begin();
  pixel.clear();
  pixel.show();

  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(hostname)) {
    Serial.print("Open: http://");
    Serial.print(hostname);
    Serial.println(".local");
  }

  applyPixel();

  server.on("/", [](){ server.send(200,"text/html",PAGE); });
  server.on("/api/status", [](){ server.send(200,"application/json",statusJson()); });

  server.on("/api/mode/toggle", [](){
    hardwareMode = !hardwareMode;
    if (hardwareMode) applyPixel();
    else { pixel.clear(); pixel.show(); }
    server.send(200,"text/plain","OK");
  });

  server.on("/api/led", [](){
    if (!server.hasArg("r") || !server.hasArg("g") || !server.hasArg("b")) {
      server.send(400,"text/plain","Missing RGB");
      return;
    }
    ledR = constrain(server.arg("r").toInt(),0,255);
    ledG = constrain(server.arg("g").toInt(),0,255);
    ledB = constrain(server.arg("b").toInt(),0,255);
    applyPixel();
    server.send(200,"text/plain","OK");
  });

  server.on("/api/fan", [](){
    if (!server.hasArg("fan")) { server.send(400,"text/plain","Missing fan"); return; }
    int i = server.arg("fan").toInt()-1;
    if (i<0 || i>3) { server.send(400,"text/plain","Bad fan"); return; }

    if (server.hasArg("enabled")) fans[i].enabled = server.arg("enabled").toInt()!=0;
    if (server.hasArg("auto")) fans[i].automatic = server.arg("auto").toInt()!=0;
    if (server.hasArg("speed")) fans[i].speed = constrain(server.arg("speed").toInt(),0,100);

    if (!hardwareMode) {
      fans[i].rpm = (fans[i].enabled && fans[i].speed>0) ? 500 + fans[i].speed*30 : 0;
    }

    server.send(200,"text/plain","OK");
  });

  server.on("/api/sim/heartbeat", [](){
    piAlive = true;
    piShuttingDown = false;
    heartbeat++;
    if (heartbeat>'O') heartbeat='A';
    lastPiUpdate = millis();
    server.send(200,"text/plain","OK");
  });

  server.on("/api/sim/shutdown", [](){
    piAlive = true;
    piShuttingDown = true;
    heartbeat='P';
    lastPiUpdate = millis();
    server.send(200,"text/plain","OK");
  });

  server.on("/api/sim/lost", [](){
    piAlive = false;
    piShuttingDown = false;
    server.send(200,"text/plain","OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Next steps:
  // - real fan PWM
  // - real tach RPM
  // - real temperature sensors
  // - USB serial from Pi: A-O heartbeat + temp, P shutdown
  // - automatic fan curves
}
