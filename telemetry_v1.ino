#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define I2C_SDA 21
#define I2C_SCL 22


unsigned long lastUpdate = 0;
const int refreshRate = 100; // ms

int speed = 0;
int gear = 0;
float lapTime = 0.0;
bool pitMode = false;
bool pitFlash = false;
String carModel = "Unknown";

// For debugging
String lastRawString = "";
bool lastParseSuccess = false;

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  loadingScreen();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(15, 20);
  display.println("Assetto Corsa HUD");
  display.display();
  delay(1500);
}

void loop() {
  parseSerialTelemetry();

  unsigned long now = millis();

  if (now - lastUpdate > refreshRate) {
    lastUpdate = now;
    if (pitMode) {
      pitFlash = !pitFlash;
      if (pitFlash) {
        drawPitMode();
      } else {
        drawHUD();
      }
    } else {
      drawHUD();
    }
  }
}

void drawHUD() {
  display.clearDisplay();

  display.drawLine(0, 0, 127, 0, SSD1306_WHITE);
  display.drawLine(0, 0, 0, 63, SSD1306_WHITE);
  display.drawLine(127, 0, 127, 63, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print("CAR:");
  display.setCursor(30, 1);
  display.print(carModel);

  display.drawLine(0,9,127,9,SSD1306_WHITE);
  display.setCursor(2,11);
  display.print("TRACK:");
  display.setCursor(40,11);
  display.print("SILVERSTONE GP");
  display.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(2,19);
  display.print("  Gear:");
  display.setTextSize(2);
  display.drawLine(0,26,127,26,SSD1306_WHITE);
  display.setCursor(32, 28);
  int display_gear=gear-1;
  if(gear==0)
  {
    display.print("N");
  }else if(display_gear==-1)
  {
    display.print("R");
  }
  else
  {
    display.print(display_gear);
  }
  display.drawLine(64,18,64,44,SSD1306_WHITE);
  display.drawLine(0, 44, 127, 44, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(65,18);
  display.print("Speed-Km/h");
  display.setCursor(88, 28);
  display.setTextSize(2);
  display.print(speed);
/*
  display.setTextSize(1);
  display.setCursor(2, 52);
  display.printf("LAP: %.2f", lapTime);
  display.drawLine(0, 63, 127, 63, SSD1306_WHITE);
  */
display.setTextSize(1);
display.setCursor(2, 52);
display.print("LAP: ");

// Format lapTime as mm:ss.x (tenths)
lapTime=lapTime/1000;
unsigned int lap_mins = (unsigned int)(lapTime / 60);
unsigned int lap_secs = (unsigned int)(lapTime) % 60;
unsigned int lap_tenths = (unsigned int)((lapTime - (lap_mins * 60 + lap_secs)) * 100);
char lapTimeStr[11];
sprintf(lapTimeStr, "%02u:%02u.%01u", lap_mins, lap_secs, lap_tenths);
display.setTextSize(2);
display.setCursor(28, 48);
display.print(lapTimeStr);
display.drawLine(0, 63, 127, 63, SSD1306_WHITE);


  display.display();
}

void drawPitMode() {
  display.clearDisplay();
  for (int i = 0; i < 3; i++) {
    display.drawRect(i, i, SCREEN_WIDTH - 2*i, SCREEN_HEIGHT - 2*i, SSD1306_WHITE);
  }
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 26);
  display.println("PIT MODE");
  display.display();
}

void loadingScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(28, 20);
  display.print("Loading HUD...");
  for (int i = 0; i < SCREEN_WIDTH; i += 8) {
    display.fillRect(16 + i, 40, 6, 10, SSD1306_WHITE);
    display.display();
    delay(60);
  }
  delay(400);
}

// Robust parsing with serial debug output
void parseSerialTelemetry() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    lastRawString = line; // Save for debug print
    lastParseSuccess = false;

    // Debug: print the exact line received
    Serial.println("RAW IN: '" + line + "'");

    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);

    if (idx1 > 0 && idx2 > idx1 && idx3 > idx2 && idx4 > idx3) {
      String speedStr = line.substring(0, idx1);
      String gearStr = line.substring(idx1 + 1, idx2);
      String lapStr = line.substring(idx2 + 1, idx3);
      String pitStr = line.substring(idx3 + 1, idx4);
      String carStr = line.substring(idx4 + 1);
      speedStr.trim(); gearStr.trim(); lapStr.trim(); pitStr.trim(); carStr.trim();

      speed = speedStr.toInt();
      gear = gearStr.toInt();
      lapTime = lapStr.toFloat();
      int pitFlag = pitStr.toInt();
      pitMode = (pitFlag == 1);
      carModel = carStr;
      if (carModel.length() > 15) carModel = carModel.substring(0, 15); // Fit display

      lastParseSuccess = true;
      // Debug: print the parsed values
      Serial.print("Parsed -> SPEED: "); Serial.print(speed);
      Serial.print(", GEAR: "); Serial.print(gear);
      Serial.print(", LAP: "); Serial.print(lapTime);
      Serial.print(", PIT: "); Serial.print(pitMode);
      Serial.print(", CAR: "); Serial.println(carModel);

    } else {
      Serial.println("ERROR: Could not parse line (bad comma count or format).");
    }
  }
}
