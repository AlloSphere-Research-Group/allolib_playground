
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetHandler.hpp"
#include "al/ui/al_PresetSequencer.hpp"
#include "al/ui/al_SequenceServer.hpp"

using namespace al;

/*
 * This tutorial presents the facilities to sequence presets. This is useful
 * where you have a monolithic system where parameter changes over time
 * form the composition. This is done by connecting a PresetSequencer to a
 * PresetHandler.
 * In a later tutorial we will explore how to sequence event/voice based
 * systems. These systems offer polyphony and they will all have internal as
 * well as global parameters.
 */

class MyApp : public App {
public:
  void onCreate() override {
    nav().pos(Vec3d(0, 0, 8)); // Set the camera to view the scene
    addCone(mesh);             // Prepare mesh to draw a cone
    mesh.primitive(Mesh::LINE_STRIP);

    gui << X << Y << Size; // Register the parameters with the GUI
    gui << presetHandler;  // Register the preset handler with GUI
    /*
     * You can register the PresetSequencer to display a GUI to control it
     */
    //        gui << presetSequencer;
    gui.init(); // Initialize GUI. Don't forget this!

    presetHandler << X << Y << Size; // Register Parameters with Preset Handler
    presetHandler.setMorphTime(2.0); // Presets will take 2 seconds to "morph"

    /*
     * After you group parameters together in a preset handler, you need to
     * register it with the sequencer. The PresetSequencer object reads sequence
     * files and then commands the PresetHandler to load them at the right
     * times.
     */
    presetSequencer << presetHandler;

    /* Add the preset sequencer to the GUI */
    gui << presetSequencer;
    /*
        A preset sequencer is exposed via OSC through a SequenceServer object.
        By default it accepts a string with the name of the sequence to play on
       the OSC address /sequence. To play the "demo.sequence" sequence, send an
       OSC message like: /sequence demo

        The OSC address can be changed with the setAddress() function.
    */
    sequenceServer << presetSequencer;
    /*
        Show the details from the sequence server. In this case it prints:
        Sequence server listening on: 127.0.0.1:9012
        Communicating on path: /sequence
    */
    sequenceServer.print();
  }

  void onAnimate(double dt) override {
    // You will want to disable navigation and text if the mouse is within
    // the gui window. You need to do this within the onAnimate callback
    navControl().active(!gui.usingInput());
  }

  void onDraw(Graphics &g) override {
    g.clear();

    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(X.get(), Y.get(), 0);
    g.scale(Size.get());
    g.draw(mesh); // Draw the mesh
    g.popMatrix();

    // Draw th GUI
    gui.draw(g);
  }

  bool onKeyDown(const Keyboard &k) override {
    // The space key plays back the sequence we create in the main() function
    if (k.key() == ' ') {
      // Notice that you don't need to add the extension ".sequence" to the name
      presetSequencer.playSequence("demo");
    }
    return true;
  }

private:
  Light light;
  Mesh mesh;

  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};

  PresetHandler presetHandler;

  /*
      A PresetSequencer object can read sequences that trigger presets over
     time.
  */
  PresetSequencer presetSequencer;
  SequenceServer sequenceServer{"127.0.0.1", 9012};

  rnd::Random<> randomGenerator; // Random number generator

  ControlGUI gui;
};

int main(int argc, char *argv[]) {
  /*
          We'll create a sequence file. This assumes that you have created
          at least three presets called 1,2 and 3 (You can create them with the
          earlier programs in this tutorial).

          Sequence files are stored in a simple plain text format. Each line
          corresponds to a command lo load a preset, with the preset name first,
          followed by the morph time and the wait time, separated by colons
     (without any spaces).

          The morph time in this case determines the time it takes to get to the
          preset and the wait time determines the time to wait after the morph
     has completed.

          For example, the sequence below sets preset "1" immediately (no morph
     time) and keeps it for 3 seconds. Then it takes 4 seconds to morph to
     preset "2". After these 4 seconds, there is a 2 second wait, after which it
     will take 1 second to get to preset "3".

          The double colons ("::") mark the end of the sequence.
  */
  std::string presetFileText = R"(1:0.0:3.0
2:4.0:2.0
3:1.0:1.0
::
)";
  // The default path for presets is "presets/"
  std::ofstream ofs(std::string("presets/demo.sequence"), std::ofstream::out);
  ofs << presetFileText; // Write the text to file
  ofs.close();

  MyApp app;
  app.start();
  return 0;
}
