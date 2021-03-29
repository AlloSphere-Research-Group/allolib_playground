/*
This example shows how to configure the App's graphics, audio and network
domains.
*/

#include "al/app/al_App.hpp"

#include "al/math/al_Random.hpp"

using namespace al;

struct MyApp : App {
  float value = 0.0f;

  void onInit() override { std::cout << "onInit() called" << std::endl; }

  void onCreate() override { std::cout << "onCreate() called" << std::endl; }

  void onAnimate(double dt) override {
    value += 0.05f;
    if (value >= 1.0f) {
      value -= 1.0f;
    }
  }

  void onDraw(Graphics &g) override { g.clear(value, 0, 0); }

  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = value * 0.1 * rnd::uniformS();
    }
  }
};

int main() {
  MyApp app;

  //  void App::configureAudio(AudioDevice &dev, double audioRate, int
  //  audioBlockSize,
  //                           int audioOutputs, int audioInputs) {
  app.configureAudio(44100, 512, 2, 0);

  // Window config
  app.fps(3);
  app.cursorHide(true);
  app.dimensions(400, 400, 300, 300);
  app.title("Hello");
  //  app.decorated(false);

  // Network config
  app.parameterServer().configure(45100, "127.0.0.1");

  app.start();

  return 0;
}
