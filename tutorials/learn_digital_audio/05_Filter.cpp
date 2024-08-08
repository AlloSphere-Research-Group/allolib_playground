// Joel A. Jaffe 2024-07-18

/* Filter
This app demonstrates how to enable user input, and also how to generate 
an impulse response for testing audio objects like filters.

@TODO: Intro paragraph

To fix this, we can use a digital filter to remove the high frequency content.
In this app we'll use a one-pole low-pass filter, which is simple to implement
and demonstrates the basics of digital filter design.
*/

#ifndef MAIN
#define MAIN 5
#endif
#include "04_Aliasing.cpp"
#include "al/app/al_GUIDomain.hpp" // GUI for controlling filter

/**
 * @brief one of the simplest types of filters.
 * A one-pole filter compares an input sample
 * with the filter's previous output, `y_1`
 */
template <typename T>
class OnePole {
private:
  T y_1 = 0; // this variable stores past output samples
  T alpha = 0; // 'filter coefficient' sets the blend of past and current values

public:
  /**
   * @brief applies the one pole equation for a low-pass filter.
   * By averaging the current input with the last output, 
   * large jumps in amplitude between sample frames (high frequencies)
   * are attenuated
   * @param input input sample at current timestep 
   */
  T lpf(T x_0) {
    this->y_1 = (y_1 * alpha) + (x_0 * (1-alpha));
    return y_1;
  }

  /**
   * @brief method for setting alpha directly.
   * Clipped to the range `[0,1]`
   */
  void setAlpha(T a) {
    if (a > 1.0) {a = 1.0;}
    if (a < 0.0) {a = 0.0;}
    this->alpha = a;
  }

  /**
   * @brief Set filter coefficient by specifying a cutoff frequency and sample rate.
   * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 166
   * @param Hz cutoff frequency in Hz
   * @param sampleRate project sample rate
   */
  void setCutoff(T Hz, T sampleRate) {
    if (Hz < 0.0) {Hz = 0.0;}
    if (Hz > sampleRate/2.0) {Hz = sampleRate/2.0;}
    Hz *= -M_2PI / sampleRate;
    this->alpha = std::pow(M_E, Hz);
  }
};

/**
 * @brief App that allows for the generation and testing of impulse responses
 */
struct Filter : public al::App {
  int sampleRate = (int)(this->audioIO().framesPerSecond()); // convenient variable to hold sampleRate
  Oscilloscope<float> oScope{sampleRate}; // instance of our Oscilloscope class
  OnePole<float> filter; // instance of our filter
  bool impulse = false; // set impulse to false initially

  // `al::Parameter` type, with args {"OSCname", "OSCgroup", initVal, minVal, maxVal}
  // ignore the first two args for now
  Parameter cutoffFreq{"cutoffFreq", "", sampleRate/2.f, 0.f, sampleRate/2.f};

  /**
   * @brief we'll override `onInit()` to create a GUI
   */
  void onInit() override {
		// Set up the GUI
		auto GUIdomain = al::GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(cutoffFreq); // add our parameter 
	}
  

  /**
   * @brief in this app we'll override `al::App::onKeyDown`,
   * allowing our app to respond to key commands
   */
  bool onKeyDown(const Keyboard& k) override {
    impulse = true; // hitting any key will fire a single-sample impulse
    return true;
  }

  /**
   * @brief this override of `onSound` processes an impulse 
   * through a filter so we can see the effect
   */
  void onSound(AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = filter.lpf(impulse);
      oScope.writeSample(output);
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
      impulse = false; // set impulse false after writing each sample
    }
  }

  void onAnimate(double dt) override {
    oScope.update();     
    filter.setCutoff(cutoffFreq, sampleRate); // we'll check for parameter updates here.                                              
  }

  void onDraw(Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }
  
};

#if (MAIN == 5)
int main() {
  Filter app;
  app.start();
  return 0;
}
#endif