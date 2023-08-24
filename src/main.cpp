// 2023.05.21  重新开始调试的代码
// matrix: 7 columns * 8 rows

#include <Arduino.h>
#include <arduinoFFT.h>
#include <FastLED_NeoMatrix.h>

#define LED 2
#define AUDIO_IN_PIN A0
#define SAMPLES         256
#define SAMPLING_FREQ   40000

#define MATRIX_WIDTH    7
#define MATRIX_HEIGHT   8
#define NUM_LEDS       (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_TYPE    WS2812B
#define LED_PIN         2
#define TOP             8

unsigned int value = 0;
unsigned int sampling_period_us;

double vReal[SAMPLES];
double vImag[SAMPLES];
double vFreq[SAMPLES];
unsigned long newTime;
int bandValues[] = {0,0,0,0,0,0,0};
int pixelHeight[] = {0,0,0,0,0,0,0};
int oldBarHeights[] = {0,0,0,0,0,0,0};
byte peak[] = {0,0,0,0,0,0,0};
long maxValue = 0;
long minValue = 1e9;

CRGB leds[NUM_LEDS];

arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT,
  NEO_MATRIX_TOP    + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS    + NEO_MATRIX_ZIGZAG);


void initMode() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
}

void readData() {
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
    // Serial.println(vReal[i]);
    vImag[i] = 0;
    vFreq[i] = i * SAMPLING_FREQ / SAMPLES;
    while ((micros() - newTime) < sampling_period_us) { /* chill */ }
  }
  // delay(1000);
}

void setup() {
  // put your setup code here, to run once:
  initMode();
}

void show(int x, int height) {
  for (int y = 0; y < height; y++) {
    matrix->drawPixel(x,y, CRGB(255,0,0));
  }
}

/**
 * @brief draw peak
 * 
 * @param band 
 */
void whitePeak(int band) {
  matrix->drawPixel(band, peak[band], CHSV(0,0,255));
}

void show(int height[]) {
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    int barHeight = height[x];
    // draw light
    for (int y = 0; y < barHeight; y++) {
      matrix->drawPixel(x, y, CHSV((x) * (255 / 7), 255, 255));
    }
    if (barHeight > peak[x]) {
      peak[x] = min(TOP, barHeight);
    }
    whitePeak(x);
  }
}



void loop() {

  FastLED.clear();

  for (int i = 0; i< MATRIX_WIDTH; i++){
    bandValues[i] = 0;
  }
  // put your main code here, to run repeatedly:
  readData();
  // Compute FFT
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  for (int i = 2; i < (SAMPLES/2); i++){
      if (vReal[i] > maxValue) {
          maxValue = vReal[i];
      }
      if (vReal[i] < minValue) {
          minValue = vReal[i];
      }

      if (i<=3 )                 bandValues[0]  += (int)vReal[i];
      if (i>10  && i<=15  )      bandValues[1]  += (int)vReal[i];
      if (i>25  && i<=35  )      bandValues[2]  += (int)vReal[i];
      if (i>35  && i<=45  )      bandValues[3]  += (int)vReal[i];
      if (i>45  && i<=68  )      bandValues[4]  += (int)vReal[i];
      if (i>68  && i<=90  )      bandValues[5]  += (int)vReal[i];
      if (i>90  && i<=128 )      bandValues[6]  += (int)vReal[i];
  }
  Serial.println(String("max: ") + maxValue + " , min: " +minValue);
  for (int i = 0; i < MATRIX_WIDTH; i++) {
      int height = map(bandValues[i], minValue, 4000, 0, MATRIX_HEIGHT); //将频率幅值映射到0-LED_COUNT的范围
      if (height > MATRIX_HEIGHT) {
        height = MATRIX_HEIGHT;
      }
      height = ((oldBarHeights[i] * 1) + height) / 2;
      pixelHeight[i] = height;
      oldBarHeights[i] = height;
  }
  Serial.println();
  show(pixelHeight);

  // Decay peak
  EVERY_N_MILLISECONDS(60) {
    for (byte band = 0; band < MATRIX_WIDTH; band++)
      if (peak[band] > 0) peak[band] -= 1;
  }

  FastLED.show();
  delay(100);
}

