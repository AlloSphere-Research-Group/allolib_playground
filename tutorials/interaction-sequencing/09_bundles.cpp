
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_ParameterBundle.hpp"
#include "al/ui/al_PresetSequencer.hpp"

#include "al/scene/al_SynthSequencer.hpp"

#include "Gamma/Domain.h"
#include "Gamma/Oscillator.h"

using namespace al;

/*
 * Bundles allow easy grouping and control of multiple instances of
 * voices.
 *
 * This method is adequate for a static array of agents (i.e. not using
 * PolySynth). Hopefully this will be extended in the future to support
 * dynamic chains too.
 */

#define NUM_VOICES 5

class MyVoice {
public:
  // We move the parameters inside the voice itself
  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};

  // And we make a bundle to group them

  ParameterBundle bundle{"myvoice"};

  MyVoice() {
    addCone(mesh); // Prepare mesh to draw a cone
    mesh.primitive(Mesh::LINE_STRIP);

    // We need to register the parameters with the bundle
    bundle << X << Y << Size;
  }

  void draw(Graphics &g) {
    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(X, Y, 0);
    g.scale(Size);
    g.draw(mesh); // Draw the mesh
    g.popMatrix();
  }

private:
  Mesh mesh;
};

class MyApp : public App {
public:
  void onInit() override { gam::sampleRate(audioIO().framesPerSecond()); }

  void onCreate() override {
    nav().pos(Vec3d(0, 0, 8)); // Set the camera to view the scene

    // Register the bundles with the gui and the preset handler
    for (int i = 0; i < NUM_VOICES; i++) {
      gui << voices[i].bundle;
      presets << voices[i].bundle;
    }
    gui << presets;
    gui.init();                 // Initialize GUI. Don't forget this!
    navControl().active(false); // Disable nav control (because we are using the
                                // control to drive the synth
  }

  void onDraw(Graphics &g) override {
    g.clear();

    // Draw all the voices
    for (int i = 0; i < NUM_VOICES; i++) {
      voices[i].draw(g);
    }

    // Draw th GUI
    gui.draw(g);
  }

private:
  ControlGUI gui;

  MyVoice voices[NUM_VOICES]; // Static array of fice voices.
  PresetHandler presets;
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
