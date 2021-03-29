
#include "Gamma/Domain.h"
#include "Gamma/Oscillator.h"
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetSequencer.hpp"

using namespace al;

/*
 * This tutorial presents the PolySynth class that can be used to manage
 * a dynamic list of voices that can render audio and graphics.
 *
 * A SynthVoice is a self contained audio/visual process that has internal
 * parameters. In this example, we use the X, Y and Scale parameters
 * to set the internal parameters for a voice. Once internal parameters are
 * set on trigger, they can't be changed from the outside. See the next tutorial
 * for the application of global parameters.
 */

/*
 * Start by defining the behavior of a voice
 */
class MyVoice : public SynthVoice {
public:
  MyVoice() {
    addCone(mesh); // Prepare mesh to draw a cone
    mesh.primitive(Mesh::LINE_STRIP);
  }
  /*
   * define onProcess(AudioIOData &io) to determine the audio behavior of the
   * voice.
   */
  void onProcess(AudioIOData &io) override {
    while (io()) {
      // Always remember to add to the audio output!!
      // Otherwise you will be overwriting everyone else!!
      io.out(0) +=
          mSource() * 0.05; // Output on the first channel scaled by 0.05;
    }
  }

  /*
   * define onProcess(Graphics &g) to draw the object. Notice that the code
   * from the previous tutorial has been placed here.
   */
  void onProcess(Graphics &g) override {
    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(mX, mY, 0);
    g.scale(mSize);
    g.draw(mesh); // Draw the mesh
    g.popMatrix();
    if (--mDurCounter < 0) {
      free(); // if we have counted all the frames, we free this voice
              // You must always mark the voice as free either in the audio
      // or graphics onProcess function if you want it to become available
      // for reuse. If you don't do this, you will see unnecessarily high
      // memory and CPU use.
    }
  }

  void set(float x, float y, float size, float frequency, float numFrames) {
    mX = x;
    mY = y;
    mSize = size;
    mSource.freq(frequency);
    mDurCounter = numFrames;
  }

private:
  gam::Sine<> mSource; // Sine wave oscillator source

  Mesh mesh; // The mesh now belongs to the voice

  float mX{0}, mY{0}, mSize{1.0}; // This are the internal parameters

  int mDurCounter{0}; // The number of frames remaining for active
};

class MyApp : public App {
public:
  virtual void onCreate() override {
    nav().pos(Vec3d(0, 0, 8)); // Set the camera to view the scene

    gui << X << Y << Size;      // Register the parameters with the GUI
    gui.init();                 // Initialize GUI. Don't forget this!
    navControl().active(false); // Disable nav control (because we are using
                                // the control to drive the synth
  }
  //    virtual void onAnimate(double dt) override {
  //        navControl().active(!gui.usingInput());
  //    }

  virtual void onDraw(Graphics &g) override {
    g.clear();

    mPolySynth.render(g); // Call render for PolySynth to generate its output

    // Draw th GUI
    gui.draw(g);
  }

  virtual void onSound(AudioIOData &io) override {
    // You must also call render() for audio if you want to hear the voice's
    // audio output
    mPolySynth.render(io);
  }

  virtual bool onKeyDown(const Keyboard &k) override {
    /*
     * First we need to get a free voice from the PolySynth.
     * If there are no free voices, one is allocated.
     */
    MyVoice *voice = mPolySynth.getVoice<MyVoice>();
    /*
     * Next, we configure the voice. Notice, we capture the current
     * values of the X,Y and Size parameters and then these become
     * "frozen" as the internal parameters for the voice.
     * For the frequency of the oscillator, we use the key pressed
     */

    int midiNote = asciiToMIDI(k.key());
    float freq = 440.0f * powf(2, (midiNote - 69) / 12.0f);
    voice->set(X, Y, Size, freq, 120);
    /*
     * After the voice is configured, it needs to be triggered in the
     * PolySynth
     */
    mPolySynth.triggerOn(voice);
    // Make every voice appear slightly different
    X = X + 0.1f;
    if (X == 1.0f) {
      X = -1.0f;
    }
    Size = Size + 0.1f;

    return true;
  }

private:
  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};

  ControlGUI gui;

  PolySynth mPolySynth;
};

int main() {
  MyApp app;
  app.dimensions(800, 600);
  app.configureAudio(44100, 256, 2, 0);
  gam::sampleRate(44100);
  app.start();
  return 0;
}
