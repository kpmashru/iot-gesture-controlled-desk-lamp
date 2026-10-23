/*************************************************************
 * Gesture Lamp + Wi-Fi Web Control  (Adafruit APDS9960, no INT pin, no battery)
 * ESP32 + APDS9960 (gesture, polled -- no interrupt wire needed) + WS2812B 16-LED ring
 *
 * - Gestures: LEFT/RIGHT cycles the 5 WHITE tones only.
 *             UP/DOWN changes brightness.
 *             HOLD hand close for 1-2s, then pull away = ON/OFF
 *             (detected from raw proximity readings, not the INT pin)
 * - Web page (open the IP shown in Serial Monitor):
 *     Slider 1: all 14 colours (9 RGB, web-only + 5 whites,
 *               also reachable by gesture)
 *     Slider 2: brightness
 * - OTA: after the first USB upload, you can upload future
 *        code changes over Wi-Fi (Tools > Port > network port)
 *
 * Libraries needed:
 *   - Adafruit APDS9960   (+ its dependencies: Adafruit BusIO, Adafruit Unified Sensor)
 *   - Adafruit NeoPixel
 *   (WiFi, WebServer, ArduinoOTA are built into the ESP32 core)
 *
 * Wiring for the sensor (4 wires only -- INT is NOT connected):
 *   VCC -> 3V3   GND -> GND   SDA -> GPIO21   SCL -> GPIO22
 *************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_NeoPixel.h>

// ===================== 1. FILL THESE IN =====================
const char* WIFI_SSID = "YourWifiNAme";
const char* WIFI_PASS = "YourWifiPassword";
// ==============================================================

// ---------- Pins ----------
#define LED_PIN   27      // ring's DIN, through a 330 ohm resistor
#define NUM_LEDS  16
// APDS9960: VCC=3V3, GND=GND, SDA=GPIO21, SCL=GPIO22 -- no INT wire used

// ---------- Settings ----------
#define MAX_BRIGHT    180
#define BRIGHT_STEP   25
#define HOLD_MIN_MS   1000     // hand held close at least this long = a "hold" (ON/OFF)
#define HOLD_MAX_MS   3000     // longer than this = ignored (something stuck in front of the sensor)
#define PROX_THRESHOLD 150     // raw proximity (0-255) above this = "hand is close". Tune if needed.

// ---------- 14 colours: 9 RGB (web slider only) + 5 whites (web + gesture) ----------
struct Colour { const char* name; uint8_t r, g, b; };
Colour colours[] = {
  {"Red",         255,   0,   0},
  {"Green",         0, 255,   0},
  {"Blue",          0,   0, 255},
  {"Yellow",      255, 200,   0},
  {"Cyan",          0, 255, 255},
  {"Magenta",     255,   0, 255},
  {"Orange",      255,  80,   0},
  {"Purple",      140,   0, 255},
  {"Pink",        255,  40, 100},
  {"Night Amber", 255, 100,  20},   // index 9  -- whites start here
  {"Warm White",  255, 170,  90},
  {"Neutral White",255,214, 170},
  {"Daylight",    255, 244, 229},
  {"Cool White",  255, 255, 255}
};
const int NUM_COLOURS = 14;
const int WHITE_START = 9;         // indexes 9-13 are the 5 whites

Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_APDS9960 apds;
WebServer server(80);

bool  lampOn      = true;
int   colourIndex = 10;     // start on Warm White
int   brightness  = 150;

// ---- hold-to-toggle state, built from plain proximity polling (no INT pin) ----
bool  handWasClose   = false;
unsigned long closeSince = 0;

// ================= Lamp control =================
void showLamp() {
  if (lampOn) {
    Colour c = colours[colourIndex];
    ring.setBrightness(min(brightness, MAX_BRIGHT));
    ring.fill(ring.Color(c.r, c.g, c.b));
  } else {
    ring.clear();
  }
  ring.show();
}

// ================= Web pages =================
void handleRoot() {
  String html =
    "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>body{font-family:sans-serif;max-width:420px;margin:30px auto;padding:0 16px}"
    "h2{color:#1F3A5F} input[type=range]{width:100%}</style></head><body>"
    "<h2>Gesture Lamp</h2>"
    "<p><b>Colour</b> (0-8 = RGB, 9-13 = whites)</p>"
    "<input type='range' min='0' max='13' value='" + String(colourIndex) + "' "
    "oninput=\"fetch('/color?value='+this.value)\"><p id='cname'>" + colours[colourIndex].name + "</p>"
    "<p><b>Brightness</b></p>"
    "<input type='range' min='10' max='255' value='" + String(brightness) + "' "
    "oninput=\"fetch('/bright?value='+this.value)\">"
    "</body></html>";
  server.send(200, "text/html", html);
}

void handleColor() {
  if (server.hasArg("value")) {
    colourIndex = constrain(server.arg("value").toInt(), 0, NUM_COLOURS - 1);
    lampOn = true;
    showLamp();
  }
  server.send(200, "text/plain", "ok");
}

void handleBright() {
  if (server.hasArg("value")) {
    brightness = constrain(server.arg("value").toInt(), 10, 255);
    lampOn = true;
    showLamp();
  }
  server.send(200, "text/plain", "ok");
}

void handleStatus() {
  String j = "{\"on\":" + String(lampOn ? "true" : "false") +
             ",\"colour\":" + String(colourIndex) +
             ",\"colour_name\":\"" + String(colours[colourIndex].name) + "\"" +
             ",\"brightness\":" + String(brightness) + "}";
  server.send(200, "application/json", j);
}

// ================= Gesture handling (polled, no INT wire) =================
void handleSwipe(uint8_t g) {
  if (!lampOn) return;
  if (g == APDS9960_LEFT || g == APDS9960_RIGHT) {
    if (colourIndex < WHITE_START) colourIndex = WHITE_START;   // jump into the whites first
    else {
      int w = colourIndex - WHITE_START;
      w = (g == APDS9960_RIGHT) ? (w + 1) % 5 : (w - 1 + 5) % 5;
      colourIndex = WHITE_START + w;
    }
    showLamp();
    Serial.println(colours[colourIndex].name);
  } else if (g == APDS9960_UP) {
    brightness = min(255, brightness + BRIGHT_STEP); showLamp();
  } else if (g == APDS9960_DOWN) {
    brightness = max(10, brightness - BRIGHT_STEP); showLamp();
  }
}

// Hold-to-toggle: watches raw proximity. If the hand stays close continuously
// for HOLD_MIN..HOLD_MAX ms and then moves away, that counts as a hold.
void handleHold() {
  uint8_t prox = apds.readProximity();
  bool isClose = (prox > PROX_THRESHOLD);

  if (isClose && !handWasClose) {          // hand just arrived
    handWasClose = true;
    closeSince = millis();
  } else if (!isClose && handWasClose) {   // hand just left -- check how long it was there
    unsigned long heldFor = millis() - closeSince;
    handWasClose = false;
    if (heldFor >= HOLD_MIN_MS && heldFor <= HOLD_MAX_MS) {
      lampOn = !lampOn;
      showLamp();
      Serial.println(lampOn ? "Lamp ON" : "Lamp OFF");
    }
    // else: too short (accidental) or too long (something was just passing by) -- ignored
  }
}

// ================= Setup / loop =================
void setup() {
  Serial.begin(115200);

  ring.begin();
  delay(300);      // let the boost converter's 5V fully stabilize first
  showLamp();
  delay(50);
  showLamp();      // send it twice -- the very first WS2812 frame is the one most likely to get dropped

  if (apds.begin()) {
    apds.enableProximity(true);
    apds.enableGesture(true);
    Serial.println("APDS9960 ready");
  } else {
    Serial.println("APDS9960 NOT found - check wiring (3.3V, SDA=21, SCL=22)");
  }

WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASS);
Serial.print("Connecting to Wi-Fi");

unsigned long startAttempt = millis();
while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
  delay(400);
  Serial.print(".");
}
WiFi.setSleep(false);
if (WiFi.status() == WL_CONNECTED) {
  String url = "http://" + WiFi.localIP().toString() + "/";
  Serial.println();
  Serial.println("****************************************************");
  Serial.println("*  Lamp web page:  " + url);
  Serial.println("****************************************************");
} else {
  Serial.println();
  Serial.print("Wi-Fi FAILED to connect. Status code: ");
  Serial.println(WiFi.status());
  Serial.println("1=NO_SSID_AVAIL 4=CONNECT_FAILED 6=WRONG_PASSWORD");
  Serial.println("Lamp will still work by gesture, web control disabled.");
}

  String url = "http://" + WiFi.localIP().toString() + "/";
  Serial.println();
  Serial.println("****************************************************");
  Serial.println("*  Lamp web page:  " + url);
  Serial.println("****************************************************");

  server.on("/", handleRoot);
  server.on("/color", handleColor);
  server.on("/bright", handleBright);
  server.on("/status", handleStatus);
  server.begin();

  ArduinoOTA.setHostname("gesture-lamp");
  ArduinoOTA.begin();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  handleHold();                              // non-blocking, checked every loop

  if (apds.gestureValid()) {
    uint8_t g = apds.readGesture();
    if (g == APDS9960_UP || g == APDS9960_DOWN || g == APDS9960_LEFT || g == APDS9960_RIGHT) {
      handleSwipe(g);
    }
  }
}
