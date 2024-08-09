// Joel A. Jaffe 2024-07-18

/* Waveshaping
This app demonstrates how our Phasor object 
can be shaped into numerous useful waveforms in a derived class.

Our Phasor class has all the basic components of an oscillator,
but has a waveform better suited to control signals than audio. 
It's 'unipolar', meaning that it's output is limited to the range [0,1].
Audio signals are typically `bipolar`, meaning they range [-1,1].

Thankfully, we can `waveshape` the Phasor's output into nearly any
waveform we want by overriding the `proccessSample()` method
*/

#ifndef MAIN
#define MAIN 3
#endif
#include "02_BasicOsc.cpp"

/**
 * @brief Oscillator that inherits from Phasor 
 * and can generate a range of different waveforms
 * as stipulated by an enum.
 */
template <typename T>
class SwissArmyOsc : public Phasor<T> { // here we define an object that 'inherits' from phasor 
public:
  enum Waveform { // we can define a set of public 'enums'
    SINE,         // that control what waveform will be generated
    SAWTOOTH,
    SQUARE,
    TRIANGLE,
  };
protected:
  Waveform shape; // add a waveform variable to our osc
public:
  explicit SwissArmyOsc(int sampleRate) : Phasor<T>(sampleRate) {
    this->shape = SwissArmyOsc::SINE; // set default in constructor 
  }

  /**
   * @brief function for setting the waveform
   */
  virtual void setWaveform(SwissArmyOsc::Waveform shape) {
    this->shape = shape;
  }

  /**
   * @brief Waveshaping function. 
   * 
   * Takes a phase in the range `[0, 1]` and returns a bipolar waveform 
   * based on the value of `this->shape`
   */
  virtual T waveShape(T phase) {
    switch (this->shape) { // we can use a `switch` here on `shape`, similar to an `if/else`
      case Waveform::SINE:
        return std::sin(M_2PI * phase);
        break;
      case Waveform::SAWTOOTH:
        return 2 * phase - 1;
        break;
      case Waveform::SQUARE:
        return phase < 0.5 ? 1.0 : -1; // concise syntax for an if/else statement
        break;
      case Waveform::TRIANGLE:
        return 1 - 4 * std::abs(phase - 0.5);
        break;
    }
  } 

  /**
   * @brief overridden `processSample()` function 
   * that produces bipolar waveforms using `this->waveShape`
   */
  virtual T processSample() override {
    T output = Phasor<T>::processSample(); // call base class to increment & retrieve phase
    output = this->waveShape(output); // apply waveshaping
    return output; 
  }
};

/**
 * @brief App that demonstrates SwissArmyOsc.
 */
struct Waveshaping : public al::App {
  Oscilloscope<float> oScope{(int)(this->audioIO().framesPerSecond())}; // instance of our Oscilloscope class
  SwissArmyOsc<float> osc{(int)(this->audioIO().framesPerSecond())}; // instance of our SwissArmyOsc class
  float phase; // <- see `onAnimate`

  /**
   * @brief this override of `onSound`
   * is actually identical to the one from the previous app
   */
  void onSound(al::AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = osc.processSample() * 0.9; // scale by 0.9 to see waveform bounds
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
    osc.setWaveform(SwissArmyOsc<float>::Waveform((int)(phase))); // truncate phase to an int, pass as waveform
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

#if (MAIN == 3)
int main() {
  Waveshaping app;
  app.start();
  return 0;
}
#endif