// Joel A. Jaffe 2024-07-17

/* Basic Oscillator
This app demonstrates a basic phase accumulator class 
*/

#ifndef MAIN // because main() cannot be overridden,
#define MAIN 2  // we use 'preprocessor macros' 
#endif // to conditionally compile main() 
#include "01_Intro.cpp" // in apps that #include others

/**
 * @brief Basic phase accumulator. 
 * Produces an ideal unipolar sawtooth wave
 * given a sample rate and frequency. 
 */
template <typename T>
class Phasor {
private:
int sampleRate;
  T phase = 0.f; // the value an oscillator tracks over time
  T frequency = 1.f; // determines how often the waveform repeats
  T phaseIncrement = frequency / sampleRate; // determines how much to increment phase by every frame

public:
  Phasor(int sampleRate) : sampleRate(sampleRate) {
    phaseIncrement = frequency / sampleRate;
  }

  /**
   * @brief Sets the oscillator's frequency
   * @param freqHz frequency in hertz (cycles per second)
   */
  virtual void setFrequency(T freqHz) {
    frequency = freqHz;
    phaseIncrement = ::abs(this->frequency) / static_cast<T>(this->sampleRate);
  }

  /**
   * @brief Increments and returns `phase` 
   * @return `phase` (after increment)
   */
  virtual T processSample() {
    phase += phaseIncrement; // increment phase
    if (phase >= 1) {phase -= 1;} // if waveform zenith, wrap phase
    if (frequency < 0) { // if negative frequency...
      return 1.f - phase; // return reverse phasor
    } else {return phase;} // return phasor
  }

  /**
   * @brief Sets `phase` manually 
   * @param ph User-defined phase. 
   * Will be wrapped to the range `[0,1]` by `processSample()` 
   */
  virtual void setPhase(T ph) { // set phase manually 
    phase = ph;
  }
  
  /**
   * @brief Returns `phase` without incrementing. 
   * If `frequency` is negative, returns `1 - phase`
   * @return `phase` 
   */
  virtual T getPhase() {
    if (frequency < 0) {return 1 - phase;} // if negative frequency return reverse phasor 
    else {return phase;} // else return phasor
  }

};

/**
 * @brief App that demonstrates Phasor.
 */
struct BasicOsc : public al::App {
  // app member objects and variables
  Oscilloscope<float> oScope{(int)(this->audioIO().framesPerSecond())}; // instance of our oscilloscope class
  Phasor<float> osc{(int)(this->audioIO().framesPerSecond())};


  /**
   * @brief in this override of `onSound`, 
   * we'll actually write audio to our speakers
   */
  void onSound(al::AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) { // for each sample frame...
      float output = osc.processSample(); // produce an output sample with our oscillator
      oScope.writeSample(output); // update oScope

      // When writing an identical output to all channels (multi-mono), 
      // we can write the output to all channels with a 'channel loop',
      // which in this case is best put inside the sample loop. 
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output; // this is where we write output to speakers.
      }
    }
  }

  void onAnimate(double dt) override {
    oScope.update();
  }

  void onDraw(al::Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(al::Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }

};

#if (MAIN == 2)
int main() {
  BasicOsc app;
  app.start();
  return 0;
}
#endif