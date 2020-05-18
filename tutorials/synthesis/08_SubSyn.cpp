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
class Sub : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.0, 0.0, 1.0);
        createInternalTriggerParameter("envDur",1, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cf2", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("bw1", 700.0, 10.0, 5000);
        createInternalTriggerParameter("bw2", 900.0, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("hmnum", 12.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.0, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
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
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};



class MyApp : public App
{
public:
  SynthGUIManager<Sub> synthManager {"synth8"};
  //    ParameterMIDI parameterMIDI;

  virtual void onInit( ) override {
    imguiInit();
    navControl().active(false);  // Disable navigation via keyboard, since we
                              // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

    void onCreate() override {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth8.synthSequence");
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
