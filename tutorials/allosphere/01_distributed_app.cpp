
/*
This set of examples demonstrate how to build a distributed application using
allolib, and will target the Allosphere in particular, but there will be
enough information if you want to run this distributed application on a
different system.

To build a distributed application using allolib, you only need to write a
single application, and at runtime determine its role to conditionally perform
different actions.

In the allosphere, there are three roles an application can take: simulator,
audio and renderer.

The simulator node is in charge of generating the common state for the
application and handling interaction.

The audio node receives the state and parameters from the simulator and renders
multichannel audio, but it is common that the simulator and audio are the same
application.

The renderer nodes take state and parameters from the simulator and render their
correct viewports from existing calibration files.

To build a distributed application, you want to start with the DistributedApp
class.
*/

#include "Gamma/Oscillator.h"
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Mesh.hpp"

using namespace al;

// Make a class that inherits from DistributedApp, and implement its virtual
// functions. The al::DistributedApp class has an identical API to al::App, so
// you can start prototyping using App, and then just change the inheritance to
// al::DistributedApp. See the other tutorials for more details about al::App.

// This class is ready, but noy yet a distributed app, as the shared state is
// not being distributed.
class MyApp : public DistributedApp {
public:
  Mesh m;
  float mod{0.5};
  bool rising{false};
  gam::Square<> osc;

  void onInit() override { osc.freq(220); }
  void onCreate() override { addIcosphere(m); }
  void onAnimate(double dt) override {
    // mod is a signal that goes up and down with a funky shape
    float factor = 0.02f;
    if (rising) {
      mod += factor;
    } else {
      mod -= factor;
    }
    if (mod > 1.0) {
      rising = false;
    } else if (mod < 0.0) {
      rising = true;
    }
  }
  void onDraw(Graphics &g) override {
    g.clear(0);
    g.translate(0, 0, -4);
    g.scale(mod);
    g.polygonLine();
    g.draw(m);
  }
  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = osc() * mod;
    }
  }
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
