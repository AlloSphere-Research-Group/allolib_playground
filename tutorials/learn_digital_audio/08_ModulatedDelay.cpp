// Joel A. Jaffe 2024-07-29

/* Modulated Delay
This app demonstrates how a number of popular digital audio effects
can be realized using modulated delay.

The effects can all be created through the parameterization 
of a single object, who's design is proposed by Jon Dattorro in
Effect Design, Part 2: Delay Line Modulation and Chorus.
Journal of the Audio Engineering Society 45.10 (1997) pgs. 764-788
*/

#ifndef MAIN
#define MAIN 8
#endif
#include "07_Delay.cpp"

/**
 * @brief ModDelay class that implements Fig. 36 of Dattorro's paper,
 * sometimes referred to as the "Dattorro Industry Scheme"
 */
template <typename T>
class ModDelay {
public: 
  enum EffectType { // enum for effect types
    VIBRATO,        
    FLANGE,
    CHORUS,
    DOUBLING,
  };
private:
  int sampleRate;
  T feedForward = 1.0, feedBack = 0.707, blend = 0.707; // coefficients (init to chorus setting)
  T delayCenter = 0.005 * sampleRate, delayWidth = 0.009 * sampleRate; // center and range of delay modulation
  DelayLine<T> buffer; // delay line
  SwissArmyOsc<T> osc; // osc for modulation

public:
  explicit ModDelay(int sampleRate) : sampleRate(sampleRate), 
  buffer(sampleRate), osc(sampleRate) {osc.setFrequency(0.15);}

  /**
   * @brief Sets all coefficients with a single function.
   * 
   * Inputs are NOT clipped to `[0,1]` but should be supplied as such.
   */
  void setCoefs(T ff, T fb, T b) {
    feedForward = ff;
    feedBack = fb;
    blend = b;
  }

  /**
   * @brief Sets the delay range in terms of a center and width
   * supplied in milliseconds.
   * 
   * The oscillator will modulate the delay between 
   * `center - width/2` and `center + width/2`
   * 
   * NOT clipped to `[0, DelayLine Length]` but should be supplied as such.
   */
  void setDelayRange(T centerMillis, T widthMillis) {
    delayCenter = (centerMillis / 1000.f) * sampleRate;
    delayWidth = (widthMillis / 1000.f) * sampleRate;;
  }

  /**
   * @brief Sets the frequency of the oscillator in Hz
   */
  void setRate(T freqHz) {osc.setFrequency(freqHz);}

  /**
   * @brief Function for setting the effect type.
   * 
   * Presets are taken from Table 7 of Dattorro's article.
   */
  void setEffectType(ModDelay::EffectType type) {
    switch (type) {
      case EffectType::VIBRATO:
        this->setCoefs(1.0, 0.0,0.0);
        this->setDelayRange(1.0, 1.0);
        this->setRate(5.0);
        break;

      case EffectType::FLANGE:
        this->setCoefs(0.707, -0.707, 0.707);
        this->setDelayRange(1.0, 1.0);
        this->setRate(0.5);
        break;

      case EffectType::CHORUS:
        this->setCoefs(1.0, 0.707, 0.707);
        this->setDelayRange(5.0, 9.0);
        this->setRate(0.15);
        break;

      case EffectType::DOUBLING:
        this->setCoefs(0.707, 0.0, 0.707);
        this->setDelayRange(20.0, 20.0);
        this->setRate(0.15);
        break;

      default: // set to chorus by default 
        this->setEffectType(ModDelay::EffectType::CHORUS);
        break;
    }
  }

  /**
   * @brief Applies the equations from Dattorro's design
   * @param input current sample
   * @return 
   */
  T processSample(T input) {
    T x_n = input - feedBack * buffer.readInterpSample(delayCenter);
    buffer.writeSample(x_n);
    return x_n * blend + feedForward * buffer.readInterpSample(delayCenter + osc.processSample() * (delayWidth/2));
  }
};

/**
 * @brief App that demonstrates ModDelay.
 */
struct ModulatedDelay : public App {
  int sampleRate = (int)(this->audioIO().framesPerSecond());
  Oscilloscope<float> oScope{sampleRate};
  MonoSynth<float> synth{sampleRate}; // synth for testing the effect
  ModDelay<float> mDelay{sampleRate};

  Parameter delayCenter{"delayCenter", "", 9.f, 0.f, 50.f};
  Parameter delayWidth{"delayWidth", "", 16.f, 0.f, 50.f}; 
  Parameter modFreq{"modFreq", "", 0.f, 0.f, 1.f};
  ParameterMenu effectType{"effectType"}; // al::ParameterMenu to choose between effect types

  // copied from 05_Synthesizer.cpp
  Parameter noteVal{"noteVal", "", 46.f, 22.f, 69.f}; // limit noteVal to a range
  Parameter volume{"volume", "", 0.f, 0.f, 1.f};
  OnePole<float> envelope; // envelope filter
  
  void onInit() override {
		// Set up the GUI
		auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(delayCenter);
    gui.add(delayWidth);
    gui.add(modFreq);

    // set effectType options
    effectType.setElements({"Vibrato", "Flange", "Chorus", "Doubling"});
    gui.add(effectType); // and add to gui

    // copied from 05_Synthesizer.cpp 
    gui.add(noteVal);
    gui.add(volume);
    envelope.setCutoff(0.5f, sampleRate);  
	}

  /**
   * @brief this override of `onSound` processes the synth through the ModDelay
   */
  void onSound(AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = mDelay.processSample(synth.processSample() * volume);
      oScope.writeSample(output);
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
    }
  }

  /**
   * @brief in this app we'll override `al::App::onKeyDown`
   * to trigger a 'noteOn' event (copied from 05_Synthesizer.cpp)
   */
  bool onKeyDown(const Keyboard& k) override {
    volume = 0.5f;
    noteVal = noteVal + rnd::uniformS() * 12.f; // random MIDI note within +/- octave of current
    synth.setFrequency(mToF<float>(noteVal));
    return true;
  }

  /**
   * @brief in this app we'll override `al::App::onKeyUp`
   * to trigger a 'noteOff' event (copied from 05_Synthesizer.cpp)
   */
  bool onKeyUp(const Keyboard& k) override {
    volume = 0.f;
    return true;
  }

  void onAnimate(double dt) override {
    oScope.update();
    mDelay.setEffectType(ModDelay<float>::EffectType(effectType.get())); // set effect type
  }

  void onDraw(Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }
  
};

#if (MAIN == 8)
int main() {
  ModulatedDelay app;
  app.start();
  return 0;
}
#endif