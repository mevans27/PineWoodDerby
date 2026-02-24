#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define APSSID "Pinewood_Timer"
#define APPSK  "thereisnospoon"

// --- PIN MAPPING ---
#define START_PIN D8  // Start Gate Sensor
#define L1_PIN    D0  // Lane 1 Sensor
#define L2_PIN    D5  // Lane 2 Sensor
#define L3_PIN    D6  // Lane 3 Sensor
#define L4_PIN    D7  // Lane 4 Sensor

#define LED1_PIN  D3  // Lane 1 LED
#define LED2_PIN  D4  // Lane 2 LED
#define LED3_PIN  3   // Lane 3 LED (TX Pin)
#define LED4_PIN  1   // Lane 4 LED (RX Pin)

#define X_OFF 32
#define Y_OFF 16

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);

unsigned long startTime;
unsigned long laneTimes[4] = {0, 0, 0, 0};
bool laneFinished[4] = {false, false, false, false};
int lanePins[4] = {L1_PIN, L2_PIN, L3_PIN, L4_PIN};
int ledPins[4] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};

bool isRacing = false, armed = true;
int lastStartState = HIGH;
int historyCount = 0;
unsigned long lastScrollTime = 0;
int scrollIndex = 0;
unsigned long firstFinishTime = 0;

struct RaceResult { float t[4]; };
RaceResult history[10];

void handleCSV() {
  String csv = "Race,L1,L2,L3,L4,Winner\n";
  for (int i = 0; i < historyCount; i++) {
    csv += String(i+1);
    float best = 99.9; int winIdx = -1;
    for(int j=0; j<4; j++) {
      csv += "," + String(history[i].t[j], 3);
      if(history[i].t[j] > 0 && history[i].t[j] < best) { best = history[i].t[j]; winIdx = j+1; }
    }
    csv += ",Lane " + String(winIdx) + "\n";
  }
  server.sendHeader("Content-Disposition", "attachment; filename=results.csv");
  server.send(200, "text/csv", csv);
}

void handleRoot() {
  int refresh = isRacing ? 1 : 5;
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv='refresh' content='" + String(refresh) + "'>";
  html += "<style>body{text-align:center; font-family:sans-serif; background:#1a1a1a; color:white;} .lane{padding:10px; border-bottom:1px solid #333;}</style></head><body>";
  html += "<h1>4-LANE RACE</h1>";
  for(int i=0; i<4; i++) {
    html += "<div class='lane'>Lane " + String(i+1) + ": " + (laneFinished[i] ? String(laneTimes[i]/1000.0, 3) + "s" : "---") + "</div>";
  }
  html += "<br><a href='/download' style='color:#0f0;'>Download CSV (" + String(historyCount) + ")</a></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  // Serial is disabled to allow LED3 (TX) and LED4 (RX) to work
  pinMode(START_PIN, INPUT_PULLUP);
  for(int i=0; i<4; i++) {
    pinMode(lanePins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  
  WiFi.softAP(APSSID, APPSK);
  server.on("/", handleRoot);
  server.on("/download", handleCSV);
  server.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  showScreen("READY", "IP: 192.168.4.1", "Drop to Start");
}

void loop() {
  server.handleClient();
  yield();

  unsigned long now = millis();
  int currentStartState = digitalRead(START_PIN);

  // --- START / RESET LOGIC ---
  if (currentStartState == LOW && lastStartState == HIGH) {
    if (armed && !isRacing) {
      startTime = now; isRacing = true; armed = false;
      firstFinishTime = 0;
      showScreen("GO!", "Racing...", "");
    } else if (!isRacing) {
      armed = true; 
      for(int i=0; i<4; i++) { laneFinished[i] = false; laneTimes[i] = 0; digitalWrite(ledPins[i], LOW); }
      showScreen("READY", "System Reset", "Waiting...");
    }
    delay(200); 
  }
  lastStartState = currentStartState;

  // --- HIGH SPEED FINISH POLLING ---
  if (isRacing) {
    bool stateChanged = false;
    int finishedCount = 0;

    for (int i = 0; i < 4; i++) {
      if (!laneFinished[i] && digitalRead(lanePins[i]) == LOW) {
        laneTimes[i] = now - startTime;
        laneFinished[i] = true;
        stateChanged = true;

        // Track first finish time for timeout
        if (firstFinishTime == 0) {
          firstFinishTime = now;
        }

        // Winner LED logic
        bool firstToFinish = true;
        for(int j=0; j<4; j++) { if(j != i && laneFinished[j]) firstToFinish = false; }
        if(firstToFinish) digitalWrite(ledPins[i], HIGH);
      }
      if (laneFinished[i]) finishedCount++;
    }

    if (stateChanged) {
      showScreen("FINISHING...", "L1:" + String(laneTimes[0]/1000.0,1), "L2:" + String(laneTimes[1]/1000.0,1));
    }

    // End race if all finished OR 5 seconds after first finish
    if (finishedCount == 4 || (firstFinishTime > 0 && (now - firstFinishTime > 20000))) {
      isRacing = false;
      
      // Fill unfinished lanes with 99.999
      for(int i=0; i<4; i++) {
        if (!laneFinished[i]) {
          laneTimes[i] = 99999;
          laneFinished[i] = true;
        }
      }
      
      if (historyCount < 10) {
        for(int i=0; i<4; i++) history[historyCount].t[i] = laneTimes[i]/1000.0;
        historyCount++;
      }
      scrollIndex = 0;
      lastScrollTime = now;
    }
  }

  // --- SCROLL RESULTS DISPLAY ---
  if (!isRacing && !armed) {
    if (now - lastScrollTime > 2000) {
      scrollIndex = (scrollIndex + 1) % 2;
      lastScrollTime = now;
      
      if (scrollIndex == 0) {
        showScreen("RESULTS 1/2", "L1:" + String(laneTimes[0]/1000.0,3) + "s", "L2:" + String(laneTimes[1]/1000.0,3) + "s");
      } else {
        showScreen("RESULTS 2/2", "L3:" + String(laneTimes[2]/1000.0,3) + "s", "L4:" + String(laneTimes[3]/1000.0,3) + "s");
      }
    }
  }
}

void showScreen(String l1, String l2, String l3) {
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(WHITE);
  display.setCursor(X_OFF, Y_OFF);      display.println(l1);
  display.setCursor(X_OFF, Y_OFF + 12); display.println(l2);
  display.setCursor(X_OFF, Y_OFF + 24); display.println(l3);
  display.display();
  yield();
}