
/*
 * GUITAR HERO CONTROLLER SYNTHESIZER - ARDUINO IDE VERSIO
 * * --- PINNIKYTKENTÄ & JOHDOTUS ---
 * Nauhapainikkeet (Fretit) - Kytke maahan (GND):
 * - VIHREÄ: GPIO 32 | PUNAINEN: GPIO 33 | KELTAINEN: GPIO 25 | SININEN: GPIO 26 | ORANSSI: GPIO 27
 * * Plektra (Strum) ja Valinnat - Kytke maahan (GND):
 * - STRUM YLÖS: GPIO 14 | STRUM ALAS: GPIO 23 
 * - SELECT: GPIO 15 | START (RESET): GPIO 13
 * * Joystick (Efektit ja Oktaavi) - Kytke maahan (GND):
 * - YLÖS: GPIO 4 | ALAS: GPIO 5 | VASEN: GPIO 16 | OIKEA: GPIO 17 | PAINIKE: GPIO 18
 * * I2S DAC (esim. PCM5102A) - Äänen ulostulo:
 * - BCLK: GPIO 22 | LRC/WS: GPIO 21 | DIN/DATA: GPIO 19
 * --------------------------------
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_I2S_DAC
#define MOZZI_I2S_PIN_BCK 22
#define MOZZI_I2S_PIN_WS 21
#define MOZZI_I2S_PIN_DATA 19

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw2048_int8.h>
#include <tables/sin2048_int8.h>
#include <ADSR.h>
#include <LowPassFilter.h>

// Pinnimääritykset
const int PIN_GREEN = 32, PIN_RED = 33, PIN_YELLOW = 25, PIN_BLUE = 26, PIN_ORANGE = 27;
const int PIN_STRUM_UP = 14, PIN_STRUM_DOWN = 23; 
const int PIN_SELECT = 15, PIN_START = 13; 
const int PIN_JOY_UP = 4, PIN_JOY_DOWN = 5, PIN_JOY_LEFT = 16, PIN_JOY_RIGHT = 17, PIN_JOY_BTN = 18;

// Tilamuuttujat
enum EffectType { CLEAN, DISTORTION, VIBRATO, TREMOLO, LPF };
volatile EffectType currentEffect = CLEAN;
volatile bool chordMode = false;
volatile int currentOctaveIndex = 1; 
const float octaveMultipliers[4] = {0.5, 1.0, 2.0, 4.0};

// Mozzi objektit
Oscil <SAW2048_NUM_CELLS, AUDIO_RATE> osc1(SAW2048_DATA);
Oscil <SAW2048_NUM_CELLS, AUDIO_RATE> osc2(SAW2048_DATA);
Oscil <SAW2048_NUM_CELLS, AUDIO_RATE> osc3(SAW2048_DATA);
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
Oscil <SIN2048_NUM_CELLS, CONTROL_RATE> vibratoLfo(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> tremoloLfo(SIN2048_DATA);
LowPassFilter lpf;

// Kommunikaatiomuuttujat ytimien välillä
volatile int tFreq1 = 0, tFreq2 = 0, tFreq3 = 0;
volatile bool noteTriggered = false;
volatile bool forceStop = false; 

// Reunatunnistus
bool lastStrumUp = HIGH, lastStrumDown = HIGH, lastSelect = HIGH, lastStart = HIGH;
bool lastJoyUp = HIGH, lastJoyDown = HIGH, lastJoyLeft = HIGH, lastJoyRight = HIGH, lastJoyBtn = HIGH;

// TEHTÄVÄ: Ohjainten luku Core 0:lla
void buttonTask(void *pvParameters) {
  while (true) {
    bool cStrumUp = digitalRead(PIN_STRUM_UP);
    bool cStrumDown = digitalRead(PIN_STRUM_DOWN);
    bool cSelect = digitalRead(PIN_SELECT);
    bool cStart = digitalRead(PIN_START);
    bool cJoyBtn = digitalRead(PIN_JOY_BTN);
    bool cJoyUp = digitalRead(PIN_JOY_UP);
    bool cJoyDown = digitalRead(PIN_JOY_DOWN);
    bool cJoyLeft = digitalRead(PIN_JOY_LEFT);
    bool cJoyRight = digitalRead(PIN_JOY_RIGHT);

    // 1. START-painike (Reset)
    if (lastStart == HIGH && cStart == LOW) {
      currentEffect = CLEAN;      
      chordMode = false;          
      currentOctaveIndex = 1;     
      forceStop = true; 
    }
    lastStart = cStart;

    // 2. Tilanvaihdot
    if (lastSelect == HIGH && cSelect == LOW) chordMode = !chordMode;
    lastSelect = cSelect;

    if (lastJoyBtn == HIGH && cJoyBtn == LOW) currentOctaveIndex = (currentOctaveIndex + 1) % 4;
    lastJoyBtn = cJoyBtn;

    // 3. Efektit
    if (cJoyUp == LOW && lastJoyUp == HIGH) currentEffect = (currentEffect == DISTORTION) ? CLEAN : DISTORTION;
    else if (cJoyDown == LOW && lastJoyDown == HIGH) currentEffect = (currentEffect == VIBRATO) ? CLEAN : VIBRATO;
    else if (cJoyLeft == LOW && lastJoyLeft == HIGH) currentEffect = (currentEffect == TREMOLO) ? CLEAN : TREMOLO;
    else if (cJoyRight == LOW && lastJoyRight == HIGH) currentEffect = (currentEffect == LPF) ? CLEAN : LPF;
    
    lastJoyUp = cJoyUp; lastJoyDown = cJoyDown; lastJoyLeft = cJoyLeft; lastJoyRight = cJoyRight;

    // 4. Fretit
    int f1 = 0, f2 = 0, f3 = 0;
    if (digitalRead(PIN_ORANGE) == LOW) { f1 = 392; if (chordMode) { f2 = 494; f3 = 587; } }
    else if (digitalRead(PIN_BLUE) == LOW) { f1 = 220; if (chordMode) { f2 = 261; f3 = 330; } }
    else if (digitalRead(PIN_YELLOW) == LOW) { f1 = 330; if (chordMode) { f2 = 392; f3 = 494; } }
    else if (digitalRead(PIN_RED) == LOW) { f1 = 294; if (chordMode) { f2 = 370; f3 = 440; } }
    else if (digitalRead(PIN_GREEN) == LOW) { f1 = 261; if (chordMode) { f2 = 330; f3 = 392; } }

    // 5. Plektra
    if ((lastStrumUp == HIGH && cStrumUp == LOW) || (lastStrumDown == HIGH && cStrumDown == LOW)) {
      if (f1 > 0) {
        float m = octaveMultipliers[currentOctaveIndex];
        tFreq1 = (int)(f1 * m); tFreq2 = (int)(f2 * m); tFreq3 = (int)(f3 * m);
        noteTriggered = true;
      }
    }
    lastStrumUp = cStrumUp; lastStrumDown = cStrumDown;
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(PIN_GREEN, INPUT_PULLUP); pinMode(PIN_RED, INPUT_PULLUP);
  pinMode(PIN_YELLOW, INPUT_PULLUP); pinMode(PIN_BLUE, INPUT_PULLUP);
  pinMode(PIN_ORANGE, INPUT_PULLUP); pinMode(PIN_STRUM_UP, INPUT_PULLUP);
  pinMode(PIN_STRUM_DOWN, INPUT_PULLUP); pinMode(PIN_SELECT, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP); 
  pinMode(PIN_JOY_UP, INPUT_PULLUP); pinMode(PIN_JOY_DOWN, INPUT_PULLUP);
  pinMode(PIN_JOY_LEFT, INPUT_PULLUP); pinMode(PIN_JOY_RIGHT, INPUT_PULLUP);
  pinMode(PIN_JOY_BTN, INPUT_PULLUP);

  vibratoLfo.setFreq(6.0f); tremoloLfo.setFreq(4.0f);
  lpf.setCutoffFreqAndResonance(50, 0);
  envelope.setADLevels(255, 0); envelope.setTimes(10, 1500, 10000, 100);

  // Pinon koko kasvatettu 4096:een Arduino IDE -vakauden takaamiseksi
  xTaskCreatePinnedToCore(buttonTask, "ButtonTask", 4096, NULL, 1, NULL, 0);
  startMozzi();
}

void updateControl() {
  if (forceStop) {
    envelope.noteOff(); 
    tFreq1 = 0; tFreq2 = 0; tFreq3 = 0;
    forceStop = false;
  }

  if (noteTriggered) { envelope.noteOn(); noteTriggered = false; }
  
  int vOff = (currentEffect == VIBRATO) ? (vibratoLfo.next() >> 4) : 0;
  osc1.setFreq(tFreq1 + vOff); osc2.setFreq(tFreq2 + vOff); osc3.setFreq(tFreq3 + vOff);
  envelope.update();
}

int updateAudio() {
  long s = osc1.next();
  if (chordMode) s = (s + osc2.next() + osc3.next()) / 3;
  if (currentEffect == DISTORTION) { s *= 5; if (s > 127) s = 127; if (s < -128) s = -128; }
  if (currentEffect == LPF) s = lpf.next(s);
  s = (int)(envelope.next() * s) >> 8;
  if (currentEffect == TREMOLO) s = (s * (128 + tremoloLfo.next())) >> 8;
  return s;
}

void loop() { audioHook(); }
