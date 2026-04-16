#include <MozziGuts.h>

#include <Oscil.h>

#include <ADSR.h>

#include <tables/sin2048_int8.h>

#define CONTROL_RATE 128
#define NUM_OSC 15
#define PIN_GREEN 13
#define PIN_RED 12
#define PIN_YELLOW 14
#define PIN_BLUE 27
#define PIN_ORANGE 26
#define PIN_STRUM_D 32
#define PIN_STRUM_U 33
#define PIN_SELECT 15
#define PIN_START 16
#define PIN_DPAD 17
#define PIN_WHAMMY 34
const int FRET_PINS[5] = {
   PIN_GREEN,
   PIN_RED,
   PIN_YELLOW,
   PIN_BLUE,
   PIN_ORANGE
};
const float CHORDS[3][5][3] = {
   {
      {
         196.00,
         246.94,
         293.66
      }, {
         130.81,
         164.81,
         196.00
      }, {
         146.83,
         185.00,
         220.00
      }, {
         220.00,
         261.63,
         329.63
      }, {
         164.81,
         196.00,
         246.94
      },
   },
   {
      {
         164.81,
         207.65,
         246.94
      },
      {
         110.00,
         138.59,
         164.81
      },
      {
         146.83,
         185.00,
         220.00
      },
      {
         196.00,
         246.94,
         293.66
      },
      {
         123.47,
         155.56,
         185.00
      },
   },
   {
      {
         130.81,
         164.81,
         196.00
      },
      {
         196.00,
         246.94,
         293.66
      },
      {
         220.00,
         261.63,
         329.63
      },
      {
         174.61,
         220.00,
         261.63
      },
      {
         164.81,
         196.00,
         246.94
      },
   },
};
const float NOTES[3][5] = {
   {
      65.41,
      73.42,
      82.41,
      98.00,
      110.00
   },
   {
      130.81,
      146.83,
      164.81,
      196.00,
      220.00
   },
   {
      261.63,
      293.66,
      329.63,
      392.00,
      440.00
   },
};
const char * SET_NAMES[3] = {
   "Pop",
   "Rock",
   "Balladi"
};
const char * OCT_NAMES[3] = {
   "Basso",
   "Normaali",
   "Melodia"
};
const char * CHORD_NAMES[3][5] = {
   {
      "G",
      "C",
      "D",
      "Am",
      "Em"
   },
   {
      "E",
      "A",
      "D",
      "G",
      "B"
   },
   {
      "C",
      "G",
      "Am",
      "F",
      "Em"
   },
};
bool chordMode = true;
int currentSet = 0;
int currentOct = 1;
bool vibratoOn = false;
int vibratoPhase = 0;
Oscil < SIN2048_NUM_CELLS, AUDIO_RATE > osc[NUM_OSC] = {
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
   Oscil < SIN2048_NUM_CELLS,
   AUDIO_RATE > (SIN2048_DATA),
};
ADSR < CONTROL_RATE, AUDIO_RATE > env[NUM_OSC];
bool fretHeld[5] = {
   false
};
bool strumDownLast = false;
bool strumUpLast = false;
bool selectLast = false;
bool startLast = false;
bool dpadLast = false;
void triggerChord(int fret) {
   for (int n = 0; n < 3; n++) {
      int idx = fret * 3 + n;
      osc[idx].setFreq(CHORDS[currentSet][fret][n]);
      env[idx].noteOn();
   }
   Serial.println("Sointu: " + String(CHORD_NAMES[currentSet][fret]));
}
void releaseChord(int fret) {
   for (int n = 0; n < 3; n++)
      env[fret * 3 + n].noteOff();
}
void triggerNote(int fret) {
   int idx = fret * 3;
   osc[idx].setFreq(NOTES[currentOct][fret]);
   env[idx].noteOn();
   Serial.print("Nuotti: ");
   Serial.print(NOTES[currentOct][fret]);
   Serial.println(" Hz");
}
void releaseNote(int fret) {
   env[fret * 3].noteOff();
}
void setup() {
   Serial.begin(115200);
   for (int i = 0; i < 5; i++)
      pinMode(FRET_PINS[i], INPUT);
   pinMode(PIN_STRUM_D, INPUT);
   pinMode(PIN_STRUM_U, INPUT);
   pinMode(PIN_SELECT, INPUT);
   pinMode(PIN_START, INPUT);
   pinMode(PIN_DPAD, INPUT);
   for (int i = 0; i < NUM_OSC; i++) {
      env[i].setADLevels(255, 180);
      env[i].setTimes(5, 80, 150, 400);
   }
   startMozzi(CONTROL_RATE);
   Serial.println("Kaynnistetty - Sointutila / Pop");
}
void updateControl() {
   for (int i = 0; i < 5; i++) {
      bool pressed = digitalRead(FRET_PINS[i]) == HIGH;
      if (!pressed && fretHeld[i])
         chordMode ? releaseChord(i) : releaseNote(i);
      fretHeld[i] = pressed;
   }
   bool sd = digitalRead(PIN_STRUM_D) == HIGH;
   bool su = digitalRead(PIN_STRUM_U) == HIGH;
   if ((sd && !strumDownLast) || (su && !strumUpLast)) {
      bool any = false;
      for (int i = 0; i < 5; i++) {
         if (fretHeld[i]) {
            chordMode ? triggerChord(i) : triggerNote(i);
            any = true;
         }
      }
      if (!any) chordMode ? triggerChord(0) : triggerNote(0);
   }
   strumDownLast = sd;
   strumUpLast = su;
   bool sel = digitalRead(PIN_SELECT) == HIGH;
   if (sel && !selectLast) {
      chordMode = !chordMode;
      Serial.println(chordMode ? "Tila: Sointutila" : "Tila: Nuottitila");
   }
   selectLast = sel;
   bool sta = digitalRead(PIN_START) == HIGH;
   if (sta && !startLast) {
      if (chordMode) {
         currentSet = (currentSet + 1) % 3;
         Serial.println("Setti: " + String(SET_NAMES[currentSet]));
      } else {
         currentOct = (currentOct + 1) % 3;
         Serial.println("Oktaavi: " + String(OCT_NAMES[currentOct]));
      }
   }
   startLast = sta;
   bool dp = digitalRead(PIN_DPAD) == HIGH;
   if (dp && !dpadLast) {
      vibratoOn = !vibratoOn;
      Serial.println(vibratoOn ? "Vibrato: ON" : "Vibrato: OFF");
   }
   dpadLast = dp;
   int whammy = mozziAnalogRead(PIN_WHAMMY);
   float bend = (float)(whammy - 2048) / 2048.0 f * 0.05 f;
   float vibrato = 0;
   if (vibratoOn) {
      vibratoPhase = (vibratoPhase + 1) % CONTROL_RATE;
      vibrato = 0.015 f * sin(2.0 f * 3.14159 f * vibratoPhase /
         (float) CONTROL_RATE * 5);
   }
   for (int i = 0; i < 5; i++) {
      for (int n = 0; n < 3; n++) {
         int idx = i * 3 + n;
         float base = chordMode ?
            CHORDS[currentSet][i][n] :
            (n == 0 ? NOTES[currentOct][i] : 0);
         if (base > 0)
            osc[idx].setFreq(base * (1.0 f + bend + vibrato));
         env[idx].update();
      }
   }
}
AudioOutput_t updateAudio() {
   int32_t out = 0;
   for (int i = 0; i < NUM_OSC; i++)
      out += (int32_t) env[i].next() * osc[i].next() >> 8;
   return MonoOutput::fromAlmostNBit(11, out / 5);
}
void loop() {
   audioHook();
}
