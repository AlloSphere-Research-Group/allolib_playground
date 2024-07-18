// Joel A. Jaffe 2024-07-18

/* Waveshaping
This app demonstrates how our basic unipolar ramp oscillator 
can be shaped into numerous useful waveforms in derived classes
*/

#ifndef MAIN
#define MAIN 3
#endif
#include "02_BasicOsc.cpp"

struct Waveshaping : public App {};

#if (MAIN == 3)
int main() {
  Waveshaping app;
  app.start();
  return 0;
}
#endif