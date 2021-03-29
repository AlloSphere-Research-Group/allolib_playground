
#include "Gamma/Domain.h"
#include "Gamma/Envelope.h"
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
 * This tutorial shows how to define and act on events' triggering.
 *
 * When building a synthesizer you may want specific actions to be taken when
 * an event is triggered and when an event is terminated. Note that the
 * actual duration of the event may be longer than the difference between
 * trigger and termination, to account for release envelopes for example.
 */

/*
 * Start by defining the behavior of a voice
 */
class MyVoice : public SynthVoice {
public:
  MyVoice() {
    addCone(mesh); // Prepare mesh to draw a cone
    mesh.primitive(Mesh::LINE_STRIP);

    mEnvelope.lengths(0.1f, 0.5f);
    mEnvelope.levels(0, 1, 0);
    mEnvelope.sustainPoint(1);
  }
  virtual void onProcess(AudioIOData &io) override {
    while (io()) {
      // We multiply the envelope by the generator
      io.out(0) += mEnvelope() * mSource() *
                   0.05; // Output on the first channel scaled by 0.05;
    }
    // it's very important to mark a voice as done to allow the polysynth
    // to reuse it. If you don't do this, each new voice will allocate new
    // memory and you will fill up your system's memory after a while!
    if (mEnvelope.done()) {
      free();
    }
  }

  void onProcess(Graphics &g) override {
    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(mX, mY, 0);
    // We can use the audio envelope to affect the graphics.
    // We will use the value() function to get its current value without
    // making it "tick" i.e. calculate the next value
    g.scale(mSize * mEnvelope.value());
    g.draw(mesh); // Draw the mesh
    g.popMatrix();
  }

  void set(float x, float y, float size, float frequency, float attackTime,
           float releaseTime) {
    mX = x;
    mY = y;
    mSize = size;
    mSource.freq(frequency);
    mEnvelope.lengths()[0] = attackTime;
    mEnvelope.lengths()[1] = releaseTime;
  }

  /* If we depend on triggering envelopes we need to define on TriggerOn()
   */
  virtual void onTriggerOn() override {
    // We want to reset the envelope:
    mEnvelope.reset();
  }

  /* If we rely on triggering a release portion of an envelope when triggered
   * off we need to override the onTriggerOff() function.
   * You can override one, both or none as needed.
   */
  virtual void onTriggerOff() override {
    // We want to force the envelope to release:
    mEnvelope.release();
  }

private:
  gam::Sine<> mSource; // Sine wave oscillator source
  gam::AD<> mEnvelope;

  Mesh mesh; // The mesh now belongs to the voice

  float mX{0}, mY{0}, mSize{1.0}; // This are the internal parameters
};

class MyApp : public App {
public:
  void onInit() override { gam::sampleRate(audioIO().framesPerSecond()); }

  void onCreate() override {
    nav().pos(Vec3d(0, 0, 8)); // Set the camera to view the scene

    gui << X << Y << Size << AttackTime
        << ReleaseTime;         // Register the parameters with the GUI
    gui.init();                 // Initialize GUI. Don't forget this!
    navControl().active(false); // Disable nav control (because we are using
                                // the control to drive the synth
  }

  void onDraw(Graphics &g) override {
    g.clear();

    mPolySynth.render(g); // Call render for PolySynth to generate its output

    // Draw th GUI
    gui.draw(g);
  }

  void onSound(AudioIOData &io) override {
    // You must also call render() for audio if you want to hear the voice's
    // audio output
    mPolySynth.render(io);
  }

  bool onKeyDown(const Keyboard &k) override {
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
    voice->set(X.get(), Y.get(), Size.get(), freq, AttackTime.get(),
               ReleaseTime.get());
    /*
     * After the voice is configured, it needs to be triggered in the
     * PolySynth. The second parameter is a time offset and the third
     * parameter is the note id. We need to pass this value to be able to
     * turn it off later.
     */
    mPolySynth.triggerOn(voice, 0, midiNote);

    // Make every voice appear slightly different
    X = X + 0.1f;
    if (X == 1.0f) {
      X = -1.0f;
    }
    Size = Size + 0.1f;

    return true;
  }

  bool onKeyUp(const Keyboard &k) override {
    int midiNote = asciiToMIDI(k.key());
    mPolySynth.triggerOff(midiNote);
    return true;
  }

private:
  Light light;

  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};
  Parameter AttackTime{"AttackTime", "Sound", 0.1, 0.001f, 2.0f};
  Parameter ReleaseTime{"ReleaseTime", "Sound", 1.0, 0.001f, 5.0f};

  ControlGUI gui;

  PolySynth mPolySynth;
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
