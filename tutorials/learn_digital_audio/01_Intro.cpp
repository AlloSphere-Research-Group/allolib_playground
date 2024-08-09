// Joel A. Jaffe 2024-07-17
// With acknowledgement and gratitude to 
// Karl Yerkes, Graham Wakefield, Lance Putnam, and Mahdi Ayman

/* Intro
This series is intended as an introduction to 
digital audio programming using AlloLib.

This first app will demonstrate how to create a basic 'AlloApp'
and also introduce the oscilloscope, an invaluable tool for 
visualizing audio data.
*/

// #includes allow recursive inclusion of classes, functions, etc
// they allow us to "build on the shoulders of giants" and develop things incrementally 
#include "al/app/al_App.hpp"
using namespace al; // use the namespace, so that we don't have to prepend objects with `al::`

/**
 * @brief An oscilloscope class that inherits from `al::Mesh`.
 * Useful for visualizing audio data. 
 * Templated so that different types of audio data (float, double, etc) can be used.
 */
template <typename T>
class Oscilloscope : public al::Mesh { // inherit from `al::Mesh`, the al:: is optional when using namespace
private: // private 'member' variables unique to each instance 
  int bufferSize; // how many samples in the past oscilloscope will remember
  int writeIndex = 0; // variable that tracks what index of the buffer to write to
  std::vector<T> buffer; // a resizable array of variables representing past audio samples

public: // public 'member' functions that allow for interaction with member variables 
  // 'Constructor' that requires the oscilloscope to be instantiated with a sample rate- 
  // this makes the scope remember 1 second of audio
  explicit Oscilloscope(int sampleRate) : bufferSize(sampleRate) {
    this->primitive(al::Mesh::LINE_STRIP); // sets how mesh will be drawn
    for (int i = 0; i < sampleRate; i++) { // for loop that: 
      buffer.push_back(0); // sets size of buffer and inits all values to zero
      this->vertex((i / static_cast<float>(sampleRate)) * 2.f - 1.f, 0); // populates mesh vertices 
      this->color(al::RGB(1.f)); // populates mesh colors
		}
  }

  /**
   * @brief Writes a new sample to the buffer. Typically used in the audio callback.
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
   * @brief Reads a sample from the buffer. 
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
   * @brief Updates the mesh with values from the buffer
   */
  void update() {
		for (int i = 0; i < bufferSize; i++) {
			vertices()[i][1] = this->readSample(bufferSize - i);
		}
	}

};

/**
 * @brief Intro app that demonstrates how an oscilloscope may be used 
 * to visualize audio data. 
 */
struct Intro : public App {
  // app member objects and variables are best but at the top
  Oscilloscope<float> oScope{(int)(AudioIOData().framesPerSecond())}; // instance of our oscilloscope class

  // in this app, we'll override some member functions of `al::App`
  // to add the functionality we want

  // `onSound` is AlloLib's 'audio callback',
  // which allows us to read input from and write output to our audio device.
  // It processes a buffer of `framesPerBuffer` samples, 
  // `framesPerBuffer`/`framesPerSecond` times per second.
  // When `framesPerBuffer` represents an amount of time below 
  // the threshold of human perception for discrete events, it is 'perceptually real-time'
  void onSound(AudioIOData& io) override {
    // inside the callback, we can loop through every sample in the buffer in a 'sample loop'
    // this allows us to operate on each 'frame' of the audio individually 
    for (int sample = 0; sample < io.framesPerBuffer(); sample++) {
      float input = io.in(0, sample); // write input from channel 0 to a variable
      oScope.writeSample(input); // feed the input value to the oscilloscope
    }
  }

  // in onAnimate, we simply update the oscilloscope 
  void onAnimate(double dt) override {
    oScope.update();
  }

  // in onDraw, we determine what will be rendered on screen
  void onDraw(Graphics &g) override {
    g.clear(0); // set black background 
    g.camera(Viewpoint::IDENTITY); // lock camera to unit space 
    g.draw(oScope); // draw oScope
  }

};
  
#ifndef MAIN // <- ignore these for now, explained in next app.
int main() {
  Intro app;
  app.start();
  return 0;
}
#endif