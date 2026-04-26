#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>

// ── Pins ──────────────────────────────────────────────────────────────────
#define TFT_CS    10
#define TFT_RST   9
#define TFT_DC    46
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL    3
#define BOOT_BTN  0

// ── Display & frame config ─────────────────────────────────────────────────
#define FRAME_W       240
#define FRAME_H       280
#define FRAME_BYTES   (FRAME_W * FRAME_H * 2)
#define TOTAL_FRAMES  62

// ── Target FPS — increase until jitter returns, then back off one step ────
#define TARGET_FPS     24
#define FRAME_TIME_MS  (1000 / TARGET_FPS)

// ── Brightness ────────────────────────────────────────────────────────────
int brightness_stages[5]     = {25, 80, 140, 200, 255};
int current_stage            = 4;
unsigned long last_button_press = 0;

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

// ── PSRAM double-buffer ───────────────────────────────────────────────────
uint16_t* frameBuf = nullptr;
uint16_t* nextBuf  = nullptr;
File gifFile;
int current_frame = 0;

// ── Timing ────────────────────────────────────────────────────────────────
uint32_t lastFrameTime = 0;
uint32_t frameDrawMs   = 0;   // measured draw time (shown in Serial)
uint32_t frameLoadMs   = 0;   // measured load time

// ─────────────────────────────────────────────────────────────────────────
void loadFrame(int idx, uint16_t* buf) {
  gifFile.seek((uint32_t)idx * FRAME_BYTES);
  gifFile.read((uint8_t*)buf, FRAME_BYTES);

  // Swap bytes for correct colour on ST7789
  for (int i = 0; i < FRAME_W * FRAME_H; i++) {
    uint16_t px = buf[i];
    buf[i] = (px >> 8) | (px << 8);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BOOT_BTN, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, brightness_stages[current_stage]);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // ── Boost SPI clock for faster drawing ───────────────────────────────────
  tft.init(240, 280);
  tft.setSPISpeed(80000000);   // 80 MHz — reduces draw time significantly
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("Display OK");

  // ── PSRAM ─────────────────────────────────────────────────────────────────
  frameBuf = (uint16_t*) ps_malloc(FRAME_BYTES);
  nextBuf  = (uint16_t*) ps_malloc(FRAME_BYTES);
  if (!frameBuf || !nextBuf) {
    Serial.println("ERROR: ps_malloc failed!");
    while (1) delay(500);
  }
  Serial.printf("PSRAM free: %u bytes\n", ESP.getFreePsram());

  // ── LittleFS ──────────────────────────────────────────────────────────────
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS failed!");
    while (1) delay(500);
  }

  gifFile = LittleFS.open("/gif.bin", "r");
  if (!gifFile) {
    Serial.println("ERROR: gif.bin not found!");
    while (1) delay(500);
  }
  Serial.printf("gif.bin: %u bytes\n", gifFile.size());

  // ── Preload first two frames ──────────────────────────────────────────────
  loadFrame(0, frameBuf);
  loadFrame(1, nextBuf);
  current_frame = 0;

  // Print timing budget info
  Serial.printf("Target FPS    : %d\n", TARGET_FPS);
  Serial.printf("Frame budget  : %d ms per frame\n", FRAME_TIME_MS);
  Serial.println("Playing — watch Serial for timing stats...");

  lastFrameTime = millis();
}

void loop() {
  // ── Button ────────────────────────────────────────────────────────────────
  if (digitalRead(BOOT_BTN) == LOW && (millis() - last_button_press > 250)) {
    current_stage++;
    if (current_stage > 4) current_stage = 0;
    analogWrite(TFT_BL, brightness_stages[current_stage]);
    last_button_press = millis();
    Serial.printf("Brightness → stage %d\n", current_stage);
  }

  // ── Wait until frame time is due (non-blocking) ───────────────────────────
  uint32_t now = millis();
  int32_t remaining = FRAME_TIME_MS - (int32_t)(now - lastFrameTime);
  if (remaining > 0) {
    delay(remaining);
  }
  lastFrameTime = millis();

  // ── Draw current frame ────────────────────────────────────────────────────
  uint32_t t0 = millis();
  tft.drawRGBBitmap(0, 0, frameBuf, FRAME_W, FRAME_H);
  frameDrawMs = millis() - t0;

  // ── Swap buffers ──────────────────────────────────────────────────────────
  uint16_t* tmp = frameBuf;
  frameBuf = nextBuf;
  nextBuf  = tmp;

  // ── Advance frame ─────────────────────────────────────────────────────────
  current_frame++;
  if (current_frame >= TOTAL_FRAMES) current_frame = 0;

  // ── Prefetch next frame ───────────────────────────────────────────────────
  uint32_t t1 = millis();
  int prefetch = (current_frame + 1) % TOTAL_FRAMES;
  loadFrame(prefetch, nextBuf);
  frameLoadMs = millis() - t1;

  // ── Print timing every 30 frames so you can tune TARGET_FPS ──────────────
  if (current_frame % 30 == 0) {
    Serial.printf("draw=%2dms  load=%2dms  budget=%dms  fps=%d\n",
                  frameDrawMs, frameLoadMs, FRAME_TIME_MS, TARGET_FPS);
  }
}