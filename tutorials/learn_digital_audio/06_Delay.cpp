// Joel A. Jaffe 2024-07-25

/* Delay
This app demonstrates a basic delay with feedback effect, based on a design from 
Generating Sound & Organizing Time I - Wakefield and Taylor 2022.
*/

#ifndef MAIN
#define MAIN 6
#endif
#include "05_Synthesizer.cpp"

/**
 * @brief A basic delay line. 
 * You may notice that it shares many features with the Oscilloscope class. 
 * This is because both objects utilize a "circular buffer"
 */
template <typename T>
class DelayLine {
private:  
  int bufferSize, writeIndex, sampleRate;
  std::vector<T> buffer;
  T delayTime; // stored as a fractional amount of samples
  T feedback; // should be between 0 and 1
  OnePole<T> loPass; // digital delay effects often use a loPass  
  T damping = 0.f; // to emulate the infidelity of analog delays 

public:
  DelayLine(int sampleRate) : bufferSize(sampleRate * 2), sampleRate(sampleRate) {
    for (int i = 0; i < bufferSize; i++) {buffer.push_back(0);}
    loPass.setAlpha(damping);
  }

  /**
   * @brief Function for setting the delay time.
   * Set in terms of milliseconds, standard for delay effects.
   */
  void setDelay(T delayMillis) {
    delayTime = (delayMillis / 1000.0) * sampleRate; // (msVal/1000) * sampleRate = numSamples
  }

  /**
   * @brief Function for setting feedback.
   * Clipped to `[0, 1]`
   */
  void setFeedback(T fbGain) {
    fbGain = fbGain > 1.0 ? 1.0 : fbGain; // if statement to keep fb <= 1
    fbGain = fbGain < 0.0 ? 0.0 : fbGain; // if statement to keep fb >= 0
    feedback = fbGain;
  }

  /**
   * @brief Function for setting damping.
   * Clipped to `[0, 1]`
   */
  void setDamping(T alpha) {
    alpha = alpha > 1.0 ? 1.0 : alpha; // if statement to keep alpha <= 1
    alpha = alpha < 0.0 ? 0.0 : alpha; // if statement to keep alpha >= 0
    damping = alpha;
    loPass.setAlpha(damping);
  }

  /**
   * @brief Writes a new sample to the buffer. Copied from Oscilloscope. 
   * @param input sample value
   */
  void writeSample(T input) { // provide input sample 
    buffer[writeIndex] = input; // write sample to writeIndex
    writeIndex++; // increment writeIndex
    if (writeIndex >= bufferSize) { // if writeIndex > maxIndex...
      writeIndex = 0; // set to 0 (first index), overwriting the oldest value (circular logic)
    }
  }

  /**
   * @brief Reads a sample from the buffer. Copied from Oscilloscope.
   * @param delayInSamples access a sample this many samples ago
   * @return `buffer[writeIndex - delayInSamples]`
   */
  T readSample(int delayInSamples) const {
    if (delayInSamples >= bufferSize) {delayInSamples = bufferSize - 1;} // limit access to oldest sample
    int readIndex = writeIndex - delayInSamples; // calculate readIndex
    if (readIndex < 0) {readIndex += bufferSize;} // circular logic
    return buffer[readIndex];
  }

  /**
   * @brief Reads a fractional sample from the buffer using linear interpolation.
   * @param delayInSamples access a sample this many samples ago
   * @return `buffer[writeIndex - delayInSamples]`
   */
  T readInterpSample(T delayInSamples) const {
    int readIndex = delayInSamples; // sample 1
    int readIndex2 = readIndex + 1; // sample 2
    T frac = delayInSamples - readIndex; // proportion of sample 2 to blend in
    return (this->readSample(readIndex) * (1.0 - frac)) + (this->readSample(readIndex2) * frac); 
  }

  /**
   * @brief function that writes a sample to the buffer and returns an output
   */
  T processSample(T input) {
    T output = loPass.lpf(this->readInterpSample(delayTime)); // read from buffer and loPass
    this->writeSample(input + output * feedback); // write sample to delay buffer
    return (input + output) / 2.0; // output is 50/50 dry/wet 
  }
};

/**
 * @brief App that demonstrates a basic delay with feedback effect
 */
struct Delay : public App {
  int sampleRate = (int)(this->audioIO().framesPerSecond()); // convenient variable to hold sampleRate
  Oscilloscope<float> oScope{sampleRate}; // instance of our Oscilloscope class
  DelayLine<float> delayLine{sampleRate};
  bool impulse = false; // set impulse to false initially

  Parameter time{"time", "", 250.f, 0.f, 2000.f}; // delay time in milliseconds 
  Parameter feedback{"feedback", "", 0.f, 0.f, 1.f}; 
  Parameter damping{"damping", "", 0.f, 0.f, 1.f};

  /**
   * @brief we'll override `onInit()` to create a GUI
   */
  void onInit() override {
		// Set up the GUI
		auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(time);
    gui.add(feedback);
    gui.add(damping);
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
      float output = delayLine.processSample(impulse);
      oScope.writeSample(output);
      for (int channel = 0; channel < io.channelsOut(); channel++) {
        io.out(channel, sample) = output;
      }
      impulse = false; // set impulse false after writing each sample
    }
  }

  void onAnimate(double dt) override {
    oScope.update();     
    delayLine.setDelay(time);
    delayLine.setFeedback(feedback);
    delayLine.setDamping(damping);
  }

  void onDraw(Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }
  
};

#if (MAIN == 6)
int main() {
  Delay app;
  app.start();
  return 0;
}
#endif