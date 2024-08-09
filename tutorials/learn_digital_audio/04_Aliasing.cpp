// Joel A. Jaffe 2024-07-22

/* Aliasing  
This app demonstrates how to create an "anti-aliased" oscillator.

We've created a bipolar oscillator object that can produce various elemental waveforms,
however have not sonified them at audio rates (frequencies > ~20Hz). 
This is because the square, sawtooth, and triangle waveforms 
contain frequencies above half the sample rate, and therefore will generate 
a type of digital distortion known as 'aliasing'.

To fix this, we can extend our SwissArmyOsc in a derived class whose
`waveShape()` and `processSample()` functions generate anti-aliased waveforms.
*/

#ifndef MAIN
#define MAIN 4
#endif
#include "03_Waveshaping.cpp"

template <typename T>
class AAOsc : public SwissArmyOsc<T> {
public:
  AAOsc(int sampleRate) : SwissArmyOsc<T>(sampleRate) {
    this->setFrequency(220);
  }

  /**
   * @brief Overridden waveshape function that returns 
   * anti-aliased waveforms...
   * 
   * @TODO: Mathematical explanation
   */
  virtual T waveShape(T phase) override {
    switch (this->shape) { 
      case SwissArmyOsc<T>::Waveform::SINE:
        return std::sin(M_2PI * phase);
        break;
      case SwissArmyOsc<T>::Waveform::SAWTOOTH:
        return pow(2 * phase - 1, 2);
        break;
      case SwissArmyOsc<T>::Waveform::SQUARE:
        return 2 * std::fabs(2 * phase - 1);
        break;
      case SwissArmyOsc<T>::Waveform::TRIANGLE:
        phase = phase * 2 - 1;
        return 2 * phase * (std::fabs(phase) - 1);
        break;
    }
  }

  /**
   * @brief Increments, waveshapes, and compares `phase` with its previous value
   * @return anti-aliased waveform
   */
  T processSample() override {
    T prevPhase = this->waveShape(this->phase);
    T nextPhase = this->waveShape(Phasor<T>::processSample());
    return (nextPhase - prevPhase) * (0.25 / std::fabs(this->frequency / this->sampleRate));
  }
};

/**
 * @brief App that demonstrates our anti-aliased oscillator
 */
struct Aliasing : public al::App {
  Oscilloscope<float> oScope{(int)(this->audioIO().framesPerSecond())}; // instance of our Oscilloscope class
  AAOsc<float> osc{(int)(this->audioIO().framesPerSecond())}; // instance of our SwissArmyOsc class
  float phase; // <- see `onAnimate`

  /**
   * @brief this override of `onSound`
   * is identical to the previous app
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
   * @brief as is this `onAnimate()`
   */
  void onAnimate(double dt) override {
    phase += dt/2; // dt counts seconds, so /2 gives us 2 seconds per waveform
    if (phase >= 4) {phase -= 4;} // wrap after numWaveforms is reached
    osc.setWaveform(SwissArmyOsc<float>::Waveform((int)(phase))); 
    oScope.update();                                              
  }

  /**
   * @brief and this `onDraw` 
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