#ifndef ADC_H
#define ADC_H

// Get level from ADC
int getLevelFan(int ADC) {
  if (ADC >= 0 && ADC < 100) return 0;
  else if (ADC >= 100 && ADC < 550) return 1;
  else if (ADC >= 550 && ADC < 1000) return 2;
  else if (ADC >= 1000 && ADC < 1450) return 3;
  else if (ADC >= 1450 && ADC < 1900) return 4;
  else if (ADC >= 1900 && ADC < 2350) return 5;
  else if (ADC >= 2350 && ADC < 2800) return 6;
  else if (ADC >= 2800 && ADC < 3250) return 7;
  else if (ADC >= 3250 && ADC < 3700) return 8;
  else if (ADC >= 3700) return 9;
  return -1;
}

// Update fan level
int updateFanLevel(int ADC_sample, int previousLevel, int lastADCValue) {
  int level = getLevelFan(ADC_sample);
  if (previousLevel != -1) {
    int border = previousLevel * 450 + 100;
    if (abs(ADC_sample - border) <= 40 && abs(lastADCValue - border) <= 40) {
      level = previousLevel;
    }
  }
  return level;
}

#endif