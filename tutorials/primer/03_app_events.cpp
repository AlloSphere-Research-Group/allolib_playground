/*
This example shows the App event callbakcs
*/

#include "al/app/al_App.hpp"

#include "al/math/al_Random.hpp"

using namespace al;

struct MyApp : App {
  float value = 0.0f;

  void onInit() override { std::cout << "onInit() called" << std::endl; }

  void onCreate() override { std::cout << "onCreate() called" << std::endl; }

  void onAnimate(double dt) override {
    value += 0.005f;
    if (value >= 1.0f) {
      value -= 1.0f;
    }
  }

  void onDraw(Graphics &g) override { g.clear(0, value, 0); }

  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = value * 0.1 * rnd::uniformS();
    }
  }

  // App event callbacks
  bool onKeyDown(Keyboard const &k) override {
    std::cout << "onKeyDown " << k.key() << std::endl;
    return true;
  }
  bool onKeyUp(Keyboard const &k) override {
    std::cout << "onKeyUp " << k.key() << std::endl;
    return true;
  }
  bool onMouseDown(Mouse const &m) override {
    std::cout << "onMouseDown " << m.button() << " : " << m.x() << "," << m.y()
              << std::endl;
    return true;
  }
  bool onMouseUp(Mouse const &m) override {
    std::cout << "onMouseUp " << m.button() << " : " << m.x() << "," << m.y()
              << std::endl;
    return true;
  }
  bool onMouseDrag(Mouse const &m) override {
    std::cout << "onMouseDrag " << m.button() << " : " << m.x() << "," << m.y()
              << std::endl;
    return true;
  }
  bool onMouseMove(Mouse const &m) override {
    std::cout << "onMouseMove " << m.x() << "," << m.y() << std::endl;
    return true;
  }
  bool onMouseScroll(Mouse const &m) override {
    std::cout << "onMouseScroll " << m.scrollX() << " " << m.scrollY()
              << std::endl;
    return true;
  }
  void onResize(int w, int h) override {
    std::cout << "onResize  " << w << "," << h << std::endl;
  }
};

int main() {
  MyApp app;

  app.start();

  return 0;
}
