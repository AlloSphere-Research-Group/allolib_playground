// Joel A. Jaffe 2024-07-18

/* Waveshaping
This app demonstrates how our basic unipolar ramp oscillator 
can be shaped into numerous useful waveforms in derived classes
*/

#ifndef MAIN
#define MAIN 3
#endif
#include "02_BasicOsc.cpp"


// Our Phasor class has all the basic components of an oscillator,
// but has a waveform better suited to control signals than audio. 
// We can `waveshape` the Phasor's output however into nearly any
// waveform we want by overriding the `proccessSample()` method
template <typename T>
class SwissArmyOsc : public Phasor<T> {
public:
  enum Waveform {
    SINE,
    SAWTOOTH,
    SQUARE,
    TRIANGLE,
  };
private:
  Waveform shape;
public:
  SwissArmyOsc(int sampleRate) : Phasor<T>(sampleRate) {
    this->shape = SwissArmyOsc::TRIANGLE;
  }

  virtual T processSample() override {
    T output = Phasor<T>::processSample();
    switch (this->shape) {
      case Waveform::SINE:
        output = std::sin(M_2PI * output);
        break;
      case Waveform::SAWTOOTH:
        output = 2 * output - 1;
        break;
      case Waveform::SQUARE:
        output = output < 0.5 ? 1.0 : -1;
        break;
      case Waveform::TRIANGLE:
        output = 1 - 4 * std::abs(output - 0.5);
        break;
      // default:
      }
    return output;
  }
};

/**
 * @brief App that demonstrates SwissArmyOsc.
 */
struct Waveshaping : public al::App {
  Oscilloscope<float> oScope{(int)(this->audioIO().framesPerSecond())}; // instance of our Oscilloscope class
  SwissArmyOsc<float> osc{(int)(this->audioIO().framesPerSecond())}; // instance of our SwissArmyOsc class

  /**
   * @brief this override of `onSound`
   * is actually identical to the one from the previous app
   */
  void onSound(al::AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = osc.processSample(); 
      oScope.writeSample(output); 
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
    }
  }

  /**
   * @brief as is this `onAnimate`
   */
  void onAnimate(double dt) override {
    oScope.update();
  }

  /**
   * @brief and `onDraw`
   */
  void onDraw(al::Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(al::Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }

};

#if (MAIN == 3)
int main() {
  Waveshaping app;
  app.start();
  return 0;
}
#endif