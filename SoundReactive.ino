#include <FastLED.h>
#include "reactive.h"

#define READ_PIN 0
#define LED_PIN 0
#define NUM_LEDS 60

#define MIC_LOW 0
#define MIC_HIGH 644

#define SAMPLE_SIZE 20
#define LONG_TERM_SAMPLES 250
#define BUFFER_DEVIATION 400
#define BUFFER_SIZE 3

CRGB leds[NUM_LEDS];

struct averageCounter *samples;
struct averageCounter *longTermSamples;
struct averageCounter* sanityBuffer;

float globalHue;
float globalBrightness = 255;
int hueOffset = 120;
float fadeScale = 1.3;
float hueIncrement = 0.7;
bool fade = false;

void setup() {
  pinMode(READ_PIN, INPUT);
  globalHue = 0;
  samples = new averageCounter(SAMPLE_SIZE);
  longTermSamples = new averageCounter(LONG_TERM_SAMPLES);
  sanityBuffer    = new averageCounter(BUFFER_SIZE);
  while (sanityBuffer->setSample(250) == true) {}
  while (longTermSamples->setSample(200) == true) {}
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  delay(10);
}

void loop() {
  uint32_t analogRaw;
  analogRaw = analogRead(READ_PIN);
  fade = false;
  soundReactive(analogRaw);
}

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve)
{
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  if (curve > 10)
    curve = 10;
  if (curve < -10)
    curve = -10;

  curve = (curve * -.1);
  curve = pow(10, curve);

  if (inputValue < originalMin)
  {
    inputValue = originalMin;
  }
  if (inputValue > originalMax)
  {
    inputValue = originalMax;
  }

  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin)
  {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal = zeroRefCurVal / OriginalRange;

  if (originalMin > originalMax)
  {
    return 0;
  }

  if (invFlag == 0)
  {
    rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else
  {
    rangedValue = newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }
  return rangedValue;
}

void soundReactive(int analogRaw) {
  int sanityValue = sanityBuffer->computeAverage();
  if (!(abs(analogRaw - sanityValue) > BUFFER_DEVIATION)) {
    sanityBuffer->setSample(analogRaw);
  }
  analogRaw = fscale(MIC_LOW, MIC_HIGH, MIC_LOW, MIC_HIGH, analogRaw, 0.4);

  if (samples->setSample(analogRaw))
  {
    return;
  }

  uint16_t longTermAverage = longTermSamples->computeAverage();
  uint16_t useVal = samples->computeAverage();
  longTermSamples->setSample(useVal);

  int diff = (useVal - longTermAverage);
  if (diff > 5)
  {
    if (globalHue < 235)
    {
      globalHue += hueIncrement;
    }
  }
  else if (diff < -5)
  {
    if (globalHue > 2)
    {
      globalHue -= hueIncrement;
    }
  }
  int curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)NUM_LEDS, (float)useVal, 0);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (i < curshow)
    {
      leds[i] = CHSV(globalHue + hueOffset + (i * 2), 255, 255);
    }
    else
    {
      leds[i] = CRGB(leds[i].r / fadeScale, leds[i].g / fadeScale, leds[i].b / fadeScale);
    }
  }
  delay(5);
  FastLED.show();
}
