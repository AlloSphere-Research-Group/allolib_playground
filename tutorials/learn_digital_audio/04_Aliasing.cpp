// Joel A. Jaffe 2024-07-22

/* Aliasing  
This app demonstrates how to create an "anti-aliased" oscillator.

We've created a bipolar oscillator object that can produce various elemental waveforms,
however have not sonified them at audio rates (frequencies > ~20Hz). 
This is because the square, sawtooth, and triangle waveforms 
contain frequencies above half the sample rate, and therefore will generate 
a type of digital distortion known as 'aliasing'.

To fix this, we can extend our SwissArmyOsc in a derived class whose
`processSample()` function generates an anti-aliased waveform.
*/

#ifndef MAIN
#define MAIN 4
#endif
#include "03_Waveshaping.cpp"

template <typename T>
class AntiAliasedOsc : public SwissArmyOsc<T> {
private:
  T prevOutput = 0;
  T sr, freq;

public:
  AntiAliasedOsc(int sampleRate, T freqHz = 2862) : SwissArmyOsc<T>(sampleRate) {
    sr = sampleRate;
    freq = freqHz;
    this->setFrequency(freqHz);
    this->setWaveform(SwissArmyOsc<float>::Waveform::SAWTOOTH);
    }

  /**
   * @brief Increments and returns `phase` 
   * @return `phase` (after increment)
   */
  virtual T processSample() override {
    T prevPhase = this->getPhase();
    T nextPhase = Phasor<T>::processSample();
    
    // Lance's method
    T i1 = pow(this->waveShape(prevPhase), 2);
    T i2 = pow(this->waveShape(nextPhase), 2);
    T dpw_inspired = (i2 - i1) * (0.25 / std::fabs(freq / sr));

    // Graham's method
    auto raw = this->waveShape(nextPhase);
    auto b = nextPhase - 0.5f;
    auto blep = 0.5f - std::abs(b); // upside-down triangle wave from 0.0 to 0.5
    blep /= std::abs((freq/sr)); // subsample time since ramp step
    blep = std::min(1.f, std::max(0.f, blep)); // we only care if the step happens this sample
    blep = (blep - 1.f) * (1.f - blep); // the blep polynomial correction
    blep = (b >= 0.f) ? blep : -blep; // flip the correction for 2nd half of transition

    return dpw_inspired;
    //return raw + blep;
    //return raw;
  }
};

/**
 * @brief App that demonstrates our anti-aliased oscillator
 */
struct Aliasing : public al::App {
  Oscilloscope<float> oScope{(int)(this->audioIO().framesPerSecond())}; // instance of our Oscilloscope class
  AntiAliasedOsc<float> osc{(int)(this->audioIO().framesPerSecond())}; // instance of our SwissArmyOsc class
  float phase; // <- see `onAnimate`

  /**
   * @brief this override of `onSound`
   * is actually identical to the one from the previous app
   */
  void onSound(al::AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = osc.processSample() * 0.5; 
      oScope.writeSample(output); 
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
    }
  }

  /**
   * @brief this `onAnimate` increments a phase at the frame rate (~60fps).
   * We use the phase to cycle through the waveform options of our osc
   */
  void onAnimate(double dt) override {
    phase += dt/2; // dt counts seconds, so /2 gives us 2 seconds per waveform
    if (phase >= 4) {phase -= 4;} // wrap after numWaveforms is reached
    //osc.setWaveform(SwissArmyOsc<float>::Waveform((int)(phase))); 
    oScope.update();                                              
  }

  /**
   * @brief this `onDraw` is identical to the previous app
   */
  void onDraw(al::Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(al::Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }

};

#if (MAIN == 4)
int main() {
  Aliasing app;
  app.start();
  return 0;
}
#endif