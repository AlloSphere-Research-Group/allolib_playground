/*
The App class is the most common way to build allolib applications.

You need to create a new class or struct that inherits from App and then
onverride the virtual functions that are called to perform computation and
rendering for different contexts like graphics and audio, and callbacks that are
called when events like keyboard, mouse or network events are received.
*/

#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

struct MyApp : App {
  float value = 0.0f;

  // The onInit() function is called when the application has been initialized,
  // but before processing actually starts, so there is no graphics context and
  // the audio card is not yet receiveing audio.
  void onInit() override { std::cout << "onInit() called" << std::endl; }
  // onCreate() is called once all processing domains are initialized. The
  // OpenGL graphics context is now available, so you can create textures and
  // initialize the graphics engine.
  void onCreate() override { std::cout << "onCreate() called" << std::endl; }

  // onAnimate() is called before rendering a graphics frame, it can be used to
  // prepare graphic elements for render. The main difference between
  // onAnimate() and onDraw() is that onAnimate() is called once per frame,
  // while onDraw() might be called more than once, for example for stereo
  // rendering.
  void onAnimate(double dt) override {
    std::cout << "onAnimate() dt = " << dt << std::endl;
    value += 0.003f;
    if (value >= 1.0f) {
      value -= 1.0f;
    }
  }

  // onDraw() is called to render framebuffers.
  void onDraw(Graphics &g) override {
    std::cout << "onDraw()" << std::endl;
    g.clear(value);
  }

  // onSound() is called whenever the audio driver requires a new audio buffer
  // through io() you can construct the sample loop, access the input/output
  // buffers and get information about the sound stream like sampling rate and
  // channels.
  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = value * 0.1 * rnd::uniformS();
    }
  }
};

int main() {
  MyApp win;
  win.start();

  return 0;
}
