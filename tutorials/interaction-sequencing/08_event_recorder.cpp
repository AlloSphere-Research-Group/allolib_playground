
#include "Gamma/Domain.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_SynthRecorder.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetSequencer.hpp"

using namespace al;

/*
 * This tutorial shows how to use the SynthRecorder class
 *
 * This class allows recording and playback of a PolySynth to a text file.
 * To allow the SynthVoice to read and write you must implement the
 * setParamFields and getParamFields function and register the SynthVoice to
 * allow instantiation from a text file.
 *
 * The sequences are stored as text files in the running directory (usually
 * bin/) with the extension ".synthSequence".
 *
 * You can also write or generate these text files manually. The following
 * commands are accepted:
 *
 * Event
 * @ absTime duration synthName pFields....
 *
 * e.g. @ 0.981379 0.116669 MyVoice 0 0 1 698.456 0.1 1
 *
 * Turnon
 * + absTime eventId synthName pFields....
 *
 * e.g. + 0.981379 25 MyVoice 0 0 1 698.456 0.1 1
 *
 * Turnoff
 * - absTime eventId
 *
 * eventId looks of the oldest id match adn turns it off
 * e.g. - 1.3 25
 *
 * Tempo
 * t absTime tempoBpm
 *
 * e.g. t 4.5 120
 *
 */

class MyVoice : public SynthVoice {
public:
  MyVoice() {
    addCone(mesh); // Prepare mesh to draw a cone
    mesh.primitive(Mesh::LINES);

    mEnvelope.lengths(0.1f, 0.5f);
    mEnvelope.levels(0, 1, 0);
    mEnvelope.sustainPoint(1);

    // We will be using the internal "frequency" parameter, so we need to
    // connect it to the oscillators frequency. Whenever the mFrequency
    // parameter changes, this will force a change in the oscillator. It is
    // necessary to use a Parameter here in order to simplify the sequencing, as
    // if parameters are used, the sequence recording and playback is automatic
    // after registering the paramters as "fields"
    mFrequency.registerChangeCallback(
        [this](float value) { mSource.freq(value); });
    // We need to do the same fo the attack and the release
    mAttack.registerChangeCallback(
        [this](float value) { mEnvelope.lengths()[0] = value; });
    mRelease.registerChangeCallback(
        [this](float value) { mEnvelope.lengths()[2] = value; });

    // Register the parameters as fields. This sets the order of the parameters
    // into the fields for sequencing. The order in which the parameters are
    // registered determines the order in which the are stored and read from
    // sequences. Be careful as changes here will most likely break your
    // existing sequences!
    *this << mX << mY << mSize << mFrequency << mAttack << mRelease;
  }

  void onProcess(AudioIOData &io) override {
    while (io()) {
      io.out(0) += mEnvelope() * mSource() *
                   0.05; // Output on the first channel scaled by 0.05;
    }
    if (mEnvelope.done()) {
      free();
    }
  }

  void onProcess(Graphics &g) override {
    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(mX, mY, 0);
    g.color(1 - (mFrequency / 1000.0f), (mFrequency / 1000.0f), 0.0);
    g.scale(mSize * mEnvelope.value());
    g.draw(mesh); // Draw the mesh
    g.popMatrix();
  }

  void set(float x, float y, float size, float frequency, float attackTime,
           float releaseTime) {
    mX = x;
    mY = y;
    mSize = size;
    mFrequency = frequency;
    mAttack = attackTime;
    mRelease = releaseTime;
  }

  void onTriggerOn() override {
    // We want to reset the envelope:
    mEnvelope.reset();
  }

  void onTriggerOff() override {
    // We want to force the envelope to release:
    mEnvelope.release();
  }

private:
  gam::Sine<> mSource; // Sine wave oscillator source
  gam::AD<> mEnvelope;

  Mesh mesh; // The mesh now belongs to the voice

  // These are the internal parameters. They "freeze" or copy the external
  // parameters. You can this way separate "per instance" parameters
  // from "global" parameters.
  Parameter mX{"X", "", 0};
  Parameter mY{"Y", "", 0};
  Parameter mSize{"Size", "", 1.0};
  Parameter mFrequency{"Frequency", "", 0.0};
  Parameter mAttack{"Attack", "", 0.0};
  Parameter mRelease{"Release", "", 0.0};
};

class MyApp : public App {
public:
  void onInit() override { gam::sampleRate(audioIO().framesPerSecond()); }

  void onCreate() override {
    nav().pos(Vec3d(0, 0, 8)); // Set the camera to view the scene

    gui << X << Y << Size << AttackTime
        << ReleaseTime; // Register the parameters with the GUI

    /*
     * The SynthRecorder object can be passed to a ControlGUI object to
     * generate a GUI interface that can be controlled via the mouse
     */

    gui << mRecorder;
    gui << mSequencer;

    gui.init(); // Initialize GUI. Don't forget this!

    navControl().active(false); // Disable nav control (because we are using
                                // the control to drive the synth

    // We need to register a PolySynth with the recorder
    // We could use PolySynth directly and we can also use the PolySynth
    // contained within the sequencer accesing it through the synth()
    // function. Using the PolySynth from the sequencer allows both
    // text file based and programmatic (C++ based) sequencing.
    mRecorder << mSequencer.synth();
  }

  void onDraw(Graphics &g) override {
    g.clear();

    // We call the render method for the sequencer. This renders its
    // internal PolySynth
    mSequencer.render(g);

    gui.draw(g);
  }

  void onSound(AudioIOData &io) override {
    // We call the render method for the sequencer to render audio
    mSequencer.render(io);
  }

  /*
   * Put back the functions to trigger the PolySynth in real time.
   * Notice that we are using the PolySynth found within the
   * sequencer instead of directly
   */
  bool onKeyDown(const Keyboard &k) override {
    MyVoice *voice = sequencer().synth().getVoice<MyVoice>();
    int midiNote = asciiToMIDI(k.key());
    float freq = 440.0f * powf(2, (midiNote - 69) / 12.0f);
    voice->set(-3.0f + X.get() + midiNote / 24.0, Y.get(), Size.get(), freq,
               AttackTime.get(), ReleaseTime.get());
    sequencer().synth().triggerOn(voice, 0, midiNote);
    return true;
  }

  bool onKeyUp(const Keyboard &k) override {
    int midiNote = asciiToMIDI(k.key());
    sequencer().synth().triggerOff(midiNote);
    return true;
  }

  SynthSequencer &sequencer() { return mSequencer; }

  SynthRecorder &recorder() { return mRecorder; }

private:
  Light light;

  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};
  Parameter AttackTime{"AttackTime", "Sound", 0.1, 0.001f, 2.0f};
  Parameter ReleaseTime{"ReleaseTime", "Sound", 1.0, 0.001f, 5.0f};

  ControlGUI gui;

  SynthRecorder mRecorder;
  SynthSequencer mSequencer;
};

int main() {
  MyApp app;

  // Before starting the application we need to register our voice in
  // the PolySynth (that is inside the sequencer). This allows
  // triggering the class from a text file.
  app.sequencer().synth().registerSynthClass<MyVoice>("MyVoice");

  app.start();
  return 0;
}
