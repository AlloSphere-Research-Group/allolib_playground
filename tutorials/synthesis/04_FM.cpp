
#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

// using namespace gam;
using namespace al;
using namespace std;

class FM : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::Sine<> car, mod;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.75, 0.1, 1.0);  // Unused

    // FM index
    createInternalTriggerParameter("idx1", 0.01, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 7, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 5, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 1, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 1.0007, 0.0, 20.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      car.freq(carBaseFreq + mod() * mModEnv() * modScale);
      float s1 = car() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    g.pushMatrix();
    g.translate(getInternalParameterValue("freq") / 300 - 2,
                getInternalParameterValue("modAmt") / 25 - 1, -4);
    float scaling = getInternalParameterValue("amplitude") * 1;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[1] = 0.001;
    mModEnv.lengths()[1] = 0.001;

    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));

    //        mModEnv.lengths()[1] = mAmpEnv.lengths()[1];

    mAmpEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
  }
};

class MyApp : public App {
 public:
  SynthGUIManager<FM> synthManager{"synth4"};

  ParameterMIDI parameterMIDI;
  int midiNote;

  void onInit() override {
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

  void onCreate() override {
    imguiInit();

    navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering

    // Play example sequence. Comment this line to start from scratch
    //    synthManager.synthSequencer().playSequence("synth4.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  void onSound(AudioIOData& io) override {
    synthManager.render(io);  // Render audio
  }

  void onAnimate(double dt) override {
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  void onDraw(Graphics& g) override {
    g.clear();
    synthManager.render(g);

    imguiDraw();
  }

  bool onKeyDown(Keyboard const& k) override {
    if (ParameterGUI::usingKeyboard()) {  // Ignore keys if GUI is using them
      return true;
    }
    if (k.shift()) {
      // If shift pressed then keyboard sets preset
      int presetNumber = asciiToIndex(k.key());
      synthManager.recallPreset(presetNumber);
    } else {
      // Otherwise trigger note for polyphonic synth
      int midiNote = asciiToMIDI(k.key());
      if (k.ctrl()) {
        midiNote -= 24;
      }
      if (midiNote > 0) {
        synthManager.voice()->setInternalParameterValue(
            "freq", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  bool onKeyUp(Keyboard const& k) override {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0) {
      synthManager.triggerOff(midiNote);
      synthManager.triggerOff(midiNote - 24);  // Trigger both off for safety
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }
};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}
