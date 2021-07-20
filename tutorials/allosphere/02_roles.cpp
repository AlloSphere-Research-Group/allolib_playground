
/*
This file shows how to know at runtime the role of the current application.
The role determines a set of "capabilities". The list of capabilities are
defined in the Capability typedef. Currently the available capabilities are:

CAP_NONE = 0,
CAP_SIMULATOR = 1 << 1,
CAP_RENDERING = 1 << 2,
CAP_OMNIRENDERING = 1 << 3,
CAP_AUDIO_IO = 1 << 4,
CAP_OSC = 1 << 5,
CAP_CONSOLE_IO = 1 << 6,
CAP_2DGUI = 1 << 7,
CAP_STATE_SEND = 1 << 8,
CAP_USER = 1 << 10

You can request the current capabilities for an application by calling
hasCapability().

The capabilities are set using a configuration file called distributed_app.toml
that must be located next to the application binary (more strictly on the
working directory at application startup).

If no config file is provided or it is empty, it will default to desktop
prototyping, where the first instance is a "desktop" node and any other
instances are "renderers". If it is run the allosphere, it will automatically
configure to the allosphere system.

Try running multiple instances of this application on a single machine and you
will see how they both take different roles.
The first instance will report:
SIMULATOR
RENDERING
AUDIOIO
OSC
2DGUI
2DGUI

The second instance will report:
SIMULATOR
OMNIRENDERING
OSC

The second instance will shown an unfolded cube representing the full surround
rendering.

The SIMULATOR capability indicates that the onAnimate() callback is called. For
this reason, you can use the onAnimate() function to send state as well as
to apply state.

Notice that the spheres are not synchronized! This is because each application
instance is running onAnimate independently and generating its own value for
"mod". The next files in this tutorial show state and parameter synchronization.
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
  float mod{0.5};
  bool rising{false};
  gam::Square<> osc;

  void onInit() override {
    // Display capabilities on the command line
    if (hasCapability(CAP_SIMULATOR)) {
      std::cout << "SIMULATOR" << std::endl;
    }
    if (hasCapability(CAP_RENDERING)) {
      std::cout << "RENDERING" << std::endl;
    }
    if (hasCapability(CAP_OMNIRENDERING)) {
      std::cout << "OMNIRENDERING" << std::endl;
    }
    if (hasCapability(CAP_AUDIO_IO)) {
      std::cout << "AUDIOIO" << std::endl;
    }
    if (hasCapability(CAP_OSC)) {
      std::cout << "OSC" << std::endl;
    }
    if (hasCapability(CAP_CONSOLE_IO)) {
      std::cout << "CONSOLEIO" << std::endl;
    }
    if (hasCapability(CAP_2DGUI)) {
      std::cout << "2DGUI" << std::endl;
    }
    if (hasCapability(CAP_STATE_SEND)) {
      std::cout << "STATE_SEND" << std::endl;
    }
    if (hasCapability(CAP_STATE_RECEIVE)) {
      std::cout << "STATE_RECEIVE" << std::endl;
    }
  }
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
    // For distributed applications, it is very important to push a
    // transformation matrix if you do transformations!
    // Try commenting out the push and pop functions and you will get only a
    // grey screen
    g.pushMatrix();
    g.translate(0, 0, -4);
    g.polygonLine();
    g.scale(mod);
    g.draw(m);

    g.popMatrix();
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
