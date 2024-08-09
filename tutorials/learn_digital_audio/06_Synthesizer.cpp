// Joel A. Jaffe 2024-07-22

/* Synthesizer 
This app demonstrates how to make a basic monophonic synth
that operates on MIDI frequency values and is enveloped by
a filter.

Now that we've created a versatile bipolar oscillator class
and filter, we can create a synthesizer class 
that combines them.
*/

#ifndef MAIN
#define MAIN 6
#endif
#include "05_Filter.cpp"
#include "al/math/al_Random.hpp"

/**
 * @brief mToF function for converting MIDI note values 
 * to frequencies in Hz
 */
template <typename T>
T mToF(int midiNote) {
  return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

/**
 * @brief Monophonic additive synthesizer with variable number of harmonics. 
 * Each harmonic rendered with equal volume.
 * Each osc is band-limited with a one-pole low-pass filter
 */
template <typename T>
class MonoSynth {
private:
  int sampleRate;
  int numOscs = 4; // set number of oscs
  T fundamental = 1.f; // fundamental frequency variable
  std::vector<AAOsc<T>> oscBank; // create an array of oscillators
  OnePole<T> filter; // add a filter
  

public:
  explicit MonoSynth(int sampleRate) : sampleRate(sampleRate) { // constructor populates our osc and filter banks
    for (int i = 0; i < numOscs; i++) {
      AAOsc<T> osc(sampleRate); 
      osc.setWaveform(SwissArmyOsc<float>::Waveform(i%4)); // mod 4 to cycle through osc types
      osc.setFrequency(fundamental * (i+1)); // each successive osc is a harmonic
      oscBank.push_back(osc); // append to oscBank
    }
  }

  virtual void setFrequency(T freq) {
    this->fundamental = freq;
    for (int i = 0; i < numOscs; i++) {
      oscBank[i].setFrequency(fundamental * (i+1));
    }
  }

  virtual void setCutoff(T cutoffHz) {
    this->filter.setCutoff(cutoffHz, sampleRate);
  }

  virtual T processSample() {
    T output = 0.0;
    for (int i = 0; i < numOscs; i++) { // for every osc
      output += filter.lpf(oscBank[i].processSample()); // process
    }
    return output / numOscs; // divide by numOscs to give each harmonic equal weight
  }
};

/**
 * @brief App that demonstrates a monophonic synthesizer 
 */
struct Synthesizer : public al::App {
  int sampleRate = (int)(this->audioIO().framesPerSecond()); // convenient variable to hold sampleRate
  Oscilloscope<float> oScope{sampleRate}; // instance of our Oscilloscope class
  MonoSynth<float> synth{sampleRate}; // instance of our synth
  OnePole<float> envelope; // envelope filter 

  // Parameters for volume and frequency
  Parameter noteVal{"noteVal", "", 46.f, 22.f, 69.f}; // limit noteVal to a range
  Parameter volume{"volume", "", 0.f, 0.f, 1.f};
  Parameter cutoffFreq{"cutoffFreq", "", sampleRate/2.f, 0.f, sampleRate/2.f};

  /**
   * @brief `onInit()` that creates a GUI
   */
  void onInit() override {
		// Set up the GUI
		auto GUIdomain = al::GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(noteVal);
    gui.add(volume);
    gui.add(cutoffFreq);
    envelope.setCutoff(15.f, sampleRate); // set envelope 
	}

  /**
   * @brief in this app we'll override `al::App::onKeyDown`
   * to trigger a 'noteOn' event
   */
  bool onKeyDown(const Keyboard& k) override {
    volume = 1.f;
    noteVal = noteVal + rnd::uniformS() * 12.f; // random MIDI note within +/- octave of current
    synth.setFrequency(mToF<float>(noteVal));
    return true;
  }

  /**
   * @brief in this app we'll override `al::App::onKeyUp`
   * to trigger a 'noteOff' event
   */
  bool onKeyUp(const Keyboard& k) override {
    volume = 0.f;
    return true;
  }

  /**
   * @brief this override of `onSound` 
   * gets a sample from the synth and envelopes it with a filter 
   */
  void onSound(AudioIOData &io) override {
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float output = synth.processSample() * envelope.lpf(volume);
      oScope.writeSample(output);
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
    }
  }

  void onAnimate(double dt) override {
    oScope.update();
    synth.setCutoff(cutoffFreq);                                               
  }

  void onDraw(Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }
  
};

#if (MAIN == 6)
int main() {
  Synthesizer app;
  app.start();
  return 0;
}
#endif