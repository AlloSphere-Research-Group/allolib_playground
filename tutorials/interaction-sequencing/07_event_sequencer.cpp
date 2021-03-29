
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
 * This tutorial shows how to use the SynthSequencer class
 *
 * This class handles the time sequencing of events for a PolySynth. An instance
 * of PolySynth is created inside the SynthSequencer and can be accessed if
 * needed.
 *
 * Events can be sequenced programatically (i.e. directly in C++ code) or in
 * realtime. This tutorial shows the first mechanism.
 */

/*
 * Start by defining the behavior of a voice.
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

  void onProcess(AudioIOData &io) override {
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
    mEnvelope.lengths()[2] = releaseTime;
  }

  /* If we depend on triggering envelopes we need to define on TriggerOn()
   */
  void onTriggerOn() override {
    // We want to reset the envelope:
    mEnvelope.reset();
  }

  /* If we rely on triggering a release portion of an envelope when triggered
   * off we need to override the onTriggerOff() function.
   * You can override one, both or none as needed.
   */
  void onTriggerOff() override {
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
    sequencer().playSequence();
  }

  void onDraw(Graphics &g) override {
    g.clear();

    // We call the render method for the sequencer. This renders its
    // internal PolySynth
    mSequencer.render(g);

    // We don't need the GUI in this case, it has no effect as we are not
    // connecting the parameters
    //        gui.draw(g);
  }

  void onSound(AudioIOData &io) override {
    // We call the render method for the sequencer to render audio
    mSequencer.render(io);
  }

  // A simple function to return a reference to the sequencer. This looks
  // nicer than just using the internal variable.
  SynthSequencer &sequencer() { return mSequencer; }

private:
  Light light;

  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};
  Parameter AttackTime{"AttackTime", "Sound", 0.1, 0.001f, 2.0f};
  Parameter ReleaseTime{"ReleaseTime", "Sound", 1.0, 0.001f, 5.0f};

  rnd::Random<> randomGenerator; // Random number generator

  ControlGUI gui;

  SynthSequencer mSequencer;
};

int main() {
  MyApp app;

  // Before starting the application we will schedule events in the sequencer

  app.sequencer().add<MyVoice>(0, 1).set(0, 0, 0.5, 440, 0.1, 0.5);
  app.sequencer().add<MyVoice>(0.5, 1).set(0, 0.5, 0.5, 880, 0.1, 0.5);
  app.sequencer().add<MyVoice>(1, 2).set(0.5, 0.5, 0.7, 660, 1.0, 0.05);

  app.sequencer().add<MyVoice>(1.1, 2).set(0.6, 0.5, 0.7, 650, 1.0, 2.0);
  app.sequencer().add<MyVoice>(1.2, 2).set(0.3, 0.4, 0.7, 640, 1.0, 2.0);
  app.sequencer().add<MyVoice>(1.3, 2).set(0.2, 0.3, 0.7, 630, 1.0, 2.0);
  app.sequencer().add<MyVoice>(1.4, 2).set(0.1, 0.2, 0.7, 620, 1.0, 2.0);
  app.sequencer().add<MyVoice>(1.5, 2).set(0.0, 0.2, 0.7, 610, 1.0, 2.0);
  app.sequencer().add<MyVoice>(1.6, 2).set(-0.1, 0.1, 0.7, 600, 1.0, 2.0);

  app.start();
  return 0;
}
