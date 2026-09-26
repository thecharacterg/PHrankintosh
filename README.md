Phrankintosh System Controller
The Phrankintosh System Controller is an ESP32-S3-based hardware management controller built for a Raspberry Pi-powered Macintosh Classic II retrofit.
What started as a simple fan controller has turned into a small independent system-management platform with Wi-Fi, a browser-based control interface, NeoPixel status control, planned fan PWM and RPM monitoring, temperature monitoring, Raspberry Pi heartbeat/shutdown supervision, serial communication, SD-card support, and future power-management features.
The basic idea is simple: the ESP32 stays available independently of the Raspberry Pi and handles low-level monitoring and control even when the Pi is booting, shut down, or misbehaving.
Current Hardware
The initial development platform is a FORIOT ESP32-S3 N16R8 DevKitC-style board.
Current confirmed hardware details include:
- ESP32-S3
- 16 MB flash
- 8 MB PSRAM
- onboard NeoPixel on GPIO48
- Wi-Fi
- USB programming/serial connection
- separate native USB/OTG capability
- planned microSD storage
- four planned 5 V PWM fan channels
- tachometer/RPM feedback
- onboard temperature sensing
- Grove-style external sensor connections
- Raspberry Pi serial supervision
- future switched 5 V Pi power control
Current Software Status
The current firmware is already serving a working web interface over Wi-Fi.
The browser interface includes:
- Overview tab
- Fan control tab
- LED control tab
- System/supervisor tab
- Test Mode / Hardware Mode switching
- NeoPixel color selection
- simulated fan speeds and RPM values
- simulated temperature telemetry
- simulated Raspberry Pi heartbeat
- simulated shutdown state
- simulated lost-Pi condition
The interface is intentionally styled after an older Macintosh control panel rather than a modern dashboard.
Wi-Fi Setup
Edit the Wi-Fi credentials in the sketch before uploading:
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

The controller also uses a hostname:
const char* hostname = "macintosh-controller";

After connecting to Wi-Fi, the ESP32 prints its IP address to the serial monitor.
The controller also starts mDNS, so on compatible networks it can be reached at:
http://macintosh-controller.local

If mDNS is unavailable, use the IP address shown in the serial monitor.
NeoPixel Test
The onboard NeoPixel has been confirmed on:
GPIO48

The web interface can currently change the NeoPixel color directly from the browser.
This is the first proven end-to-end hardware control path:
Browser
   ↓
Wi-Fi
   ↓
ESP32 web server
   ↓
GPIO48
   ↓
NeoPixel

That same control model will be expanded to the fan channels, temperature sensors, and Raspberry Pi supervision.
Raspberry Pi Supervision
The planned Raspberry Pi companion service will communicate with the ESP32 over the USB programming/serial connection.
The Pi will periodically send:
- CPU temperature
- heartbeat sequence
- system state
- shutdown state
- future telemetry and control data
The current heartbeat concept uses letters A through O during normal operation.
Example:
T,4B,A,47.3
T,4B,B,47.4
T,4B,C,47.4

The sequence advances once per second and wraps after O.
P is reserved for shutdown:
T,4B,P,47.1

This lets the ESP32 distinguish between:
- normal operation
- intentional shutdown
- lost serial communication
- a crashed or unresponsive Raspberry Pi
Future serial commands may use a separate command prefix:
C,FAN,1,50
C,FAN,2,AUTO
C,LED,255,0,0
C,GET,STATUS

Fan Control Plans
The board is being designed around four 5 V PWM fan channels.
Each channel will eventually support:
- enable/disable
- manual speed percentage
- automatic fan curve
- tach/RPM monitoring
- fault detection
- temperature-based speed control
The current web interface already contains placeholder controls for all four fans so that the software can be tested before the final PCB is manufactured.
Development Philosophy
A major goal of this project is to test as much of the system as possible before ordering the PCB.
The web interface, state machine, fan controls, temperature reporting, serial protocol, heartbeat logic, and GPIO behavior can all be exercised on prototype hardware first.
That means the final PCB should primarily be a clean implementation of a system that has already been electrically and logically tested.
Future Features
Planned or possible additions include:
- real fan PWM control
- real RPM/tach monitoring
- onboard temperature sensor
- external buck-converter temperature sensor
- case temperature/humidity sensor
- SD-card logging
- configurable fan curves
- configurable NeoPixel status colors
- Raspberry Pi temperature reporting
- Pi heartbeat monitoring
- graceful shutdown handling
- remote Pi power control
- Wi-Fi scanning/configuration
- local DNS/mDNS access
- sound effects through I²S audio
- SD-hosted web assets
- optional browser-based games because mission creep is apparently a design requirement
Yes, Doom has already been discussed.
Why “Phrankintosh”?
Because at some point this stopped being merely a Macintosh Classic II restoration and became a collection of Raspberry Pi hardware, ESP32 control electronics, custom PCB work, 3D-printed parts, fan management, web interfaces, serial supervision, and increasingly questionable feature ideas living inside a classic Macintosh shell.
At that point, “Phrankintosh” seemed appropriate.
Status
Early prototype / active development.
The current proof of concept successfully demonstrates:
browser → Wi-Fi → ESP32 → hardware control
The next major step is moving from simulated fan data to one real PWM fan channel with live RPM feedback.
