
/*
The simplest way to share information between application nodes are the
parameter classes.

This file shows how to add parameters to your distributed application. We will
use this parameter in all contexts to test that it is being propagated
correctly.

See the interaction-sequencing set of tutorials for more details on how to use
parameters.

The first instance on the application on a machine will take all the roles,
The second instance you run will take on a renderer role. We can test if we are
on the primary application by calling isPrimary(). You can also test for
specific capabilities, for example to avoid drawing the GUI on the renderers.

Run a second instance of this application and you will see the spheres are
synchronized because the primary application (the first one being run)
is writing to the "mod" parameter, while the secondary renderer is receiving
it through the parameter server.

This is a quick and easy way to synchronize data, but for more complex
situations, and especially if you want to move large state on every frame, you
should use the DistributedAppWithState class. This is shown in other files in
this tutorial.

*/

#include "Gamma/Oscillator.h"
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Mesh.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

class MyApp : public DistributedApp {
public:
  Mesh m;
  bool rising{false};
  gam::Square<> osc;

  // We will make the modulation "factor" a parameter
  Parameter factor{"factor", "", 0.03f, 0.001f, 0.08f};
  // The "mod" parameter will only be used to move data
  // it will not be visible in the controls.
  Parameter mod{"mod", "", 0.5};
  ControlGUI gui;

  void onInit() override {}
  void onCreate() override {
    addIcosphere(m);
    gui.init();
    gui << factor; // display factor parameter in gui
    // We don't want to show the "mod" parameter, but we do want to
    // synchronize it over the network.
    parameterServer() << factor << mod;
  }

  void onAnimate(double dt) override {
    // We only want to update the mod parameter if we are the main machine

    if (isPrimary()) {
      if (rising) {
        // We can no longer use *= operators with Parameter classes...
        mod = mod + factor;
      } else {
        mod = mod - factor;
      }
      if (mod > 1.0) {
        rising = false;
      } else if (mod < 0.0) {
        rising = true;
      }
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0);
    g.pushMatrix();
    g.translate(factor * 10, 0, -4);
    g.scale(mod);
    g.polygonLine();
    g.draw(m);
    if (hasCapability(Capability::CAP_2DGUI)) {
      gui.draw(g);
    }
    g.popMatrix();
  }

  void onSound(AudioIOData &io) override {
    // factor affects frequency
    osc.freq(220 * factor * 10);
    while (io()) {
      io.out(0) = osc() * mod;
    }
  }

  void onExit() override { gui.cleanup(); }
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
