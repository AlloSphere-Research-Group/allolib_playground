#include <cstdio>  // for printing to stdout

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

using namespace gam;
using namespace al;
using namespace std;

// tables for oscillator
gam::ArrayPow2<float>
tbSin(2048), tbSqr(2048), tbPls(2048), tbDin(2048);


class OscAM : public SynthVoice {
public:

  gam::Osc<> mAM;
  gam::ADSR<> mAMEnv;
  gam::Sine<> mOsc;
  gam::ADSR<> mAmpEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::Pan<> mPan;

  Mesh mMesh;

  // Initialize voice. This function will nly be called once per voice
  virtual void init( ) {
    mAmpEnv.levels(0,1,1,0);
//    mAmpEnv.sustainPoint(1);

    mAMEnv.curve(0);
    mAMEnv.levels(0,1,1,0);
//    mAMEnv.sustainPoint(1);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amplitude", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 10, 4000.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.75, 0.1, 1.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("amFunc", 0.0, 0.0, 3.0);
    createInternalTriggerParameter("am1", 0.75, 0.1, 1.0);
    createInternalTriggerParameter("am2", 0.75, 0.1, 1.0);
    createInternalTriggerParameter("amRise", 0.75, 0.1, 1.0);
    createInternalTriggerParameter("amRatio", 0.75, 0.1, 2.0);
  }

  virtual void onProcess(AudioIOData& io) override {
    mOsc.freq(getInternalParameterValue("frequency"));

    float amp = getInternalParameterValue("amplitude");
    float amRatio = getInternalParameterValue("amRatio");
    while(io()){

      mAM.freq(mOsc.freq()*amRatio);            // set AM freq according to ratio
      float amAmt = mAMEnv();                    // AM amount envelope

      float s1 = mOsc();                        // non-modulated signal
      s1 = s1*(1-amAmt) + (s1*mAM())*amAmt;    // mix modulated and non-modulated

      s1 *= mAmpEnv() *amp;

      float s2;
      mEnvFollow(s1);
      mPan(s1, s1,s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if(mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(amplitude,  amplitude, -4);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
  }

  virtual void onTriggerOn() override {
    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.lengths()[1] = 0.001;
    mAmpEnv.release(getInternalParameterValue("releaseTime"));

    mAmpEnv.levels()[1]= getInternalParameterValue("sustain");
    mAmpEnv.levels()[2]= getInternalParameterValue("sustain");

    mAMEnv.levels(getInternalParameterValue("am1"),
                  getInternalParameterValue("am2"),
                  getInternalParameterValue("am2"),
                  getInternalParameterValue("am1"));

    mAMEnv.lengths(getInternalParameterValue("amRise"),
                   1-getInternalParameterValue("amRise"));

    mPan.pos(getInternalParameterValue("pan"));

    mAmpEnv.reset();
    mAMEnv.reset();
    // Map table number to table in memory
    switch (int(getInternalParameterValue("amFunc"))) {
    case 0: mAM.source(tbSin); break;
    case 1: mAM.source(tbSqr); break;
    case 2: mAM.source(tbPls); break;
    case 3: mAM.source(tbDin); break;
    }
  }

  virtual void onTriggerOff() override {
      mAmpEnv.triggerRelease();
      mAMEnv.triggerRelease();
  }
};



class MyApp : public App
{
public:
  SynthGUIManager<OscAM> synthManager {"synth6"};
  //    ParameterMIDI parameterMIDI;
  int midiNote;

  virtual void onInit( ) override {
    imguiInit();
    navControl().active(false);  // Disable navigation via keyboard, since we
                              // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    gam::addWave(tbSin, SINE);
    gam::addWave(tbSqr, SQUARE);
    gam::addWave(tbPls, IMPULSE, 4);

    // inharmonic partials
    {    float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
      float C[] = {10, 27, 54, 81, 108, 135};
      addSines(tbDin, A,C,6);
    }

  }

    void onCreate() override {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth2.synthSequence");
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

        // Draw GUI
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
        if (midiNote > 0) {
            synthManager.voice()->setInternalParameterValue(
                "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
            synthManager.triggerOn(midiNote);
        }
        }
        return true;
    }

    bool onKeyUp(Keyboard const& k) override {
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0) {
        synthManager.triggerOff(midiNote);
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
