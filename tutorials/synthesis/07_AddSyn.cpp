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

class AddSyn : public SynthVoice {
public:

  gam::Sine<> mOsc;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc2;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc4;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc6;
  gam::Sine<> mOsc7;
  gam::Sine<> mOsc8;
  gam::Sine<> mOsc9;
  gam::ADSR<> mEnvStri;
  gam::ADSR<> mEnvLow;
  gam::ADSR<> mEnvUp;
  gam::Pan<> mPan;
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;

  virtual void init() {

    // Intialize envelopes
    mEnvStri.curve(-4); // make segments lines
    mEnvStri.levels(0,1,1,0);
    mEnvStri.lengths(0.1, 0.1, 0.1);
    mEnvStri.sustain(2); // Make point 2 sustain until a release is issued
    mEnvLow.curve(-4); // make segments lines
    mEnvLow.levels(0,1,1,0);
    mEnvLow.lengths(0.1, 0.1, 0.1);
    mEnvLow.sustain(2); // Make point 2 sustain until a release is issued
    mEnvUp.curve(-4); // make segments lines
    mEnvUp.levels(0,1,1,0);
    mEnvUp.lengths(0.1, 0.1, 0.1);
    mEnvUp.sustain(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amp", 0.01, 0.0, 0.3);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("ampStri", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackStri", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseStri", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustainStri", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("ampLow", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackLow", 0.001, 0.01, 3.0);
    createInternalTriggerParameter("releaseLow", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustainLow", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("ampUp", 0.6, 0.0, 1.0);
    createInternalTriggerParameter("attackUp", 0.01, 0.01, 3.0);
    createInternalTriggerParameter("releaseUp", 0.075, 0.1, 10.0);
    createInternalTriggerParameter("sustainUp", 0.9, 0.0, 1.0);
    createInternalTriggerParameter("freqStri1", 1.0, 0.1, 10);
    createInternalTriggerParameter("freqStri2", 2.001, 0.1, 10);
    createInternalTriggerParameter("freqStri3", 3.0, 0.1, 10);
    createInternalTriggerParameter("freqLow1", 4.009, 0.1, 10);
    createInternalTriggerParameter("freqLow2", 5.002, 0.1, 10);
    createInternalTriggerParameter("freqUp1", 6.0, 0.1, 10);
    createInternalTriggerParameter("freqUp2", 7.0, 0.1, 10);
    createInternalTriggerParameter("freqUp3", 8.0, 0.1, 10);
    createInternalTriggerParameter("freqUp4", 9.0, 0.1, 10);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  virtual void onProcess(AudioIOData& io) override {
    // Parameters will update values once per audio callback
    float freq = getInternalParameterValue("frequency");
    mOsc.freq(freq);
    mOsc1.freq(getInternalParameterValue("freqStri1") * freq);
    mOsc2.freq(getInternalParameterValue("freqStri2") * freq);
    mOsc3.freq(getInternalParameterValue("freqStri3") * freq);
    mOsc4.freq(getInternalParameterValue("freqLow1") * freq);
    mOsc5.freq(getInternalParameterValue("freqLow2") * freq);
    mOsc6.freq(getInternalParameterValue("freqUp1") * freq);
    mOsc7.freq(getInternalParameterValue("freqUp2") * freq);
    mOsc8.freq(getInternalParameterValue("freqUp3") * freq);
    mOsc9.freq(getInternalParameterValue("freqUp4") * freq);
    mPan.pos(getInternalParameterValue("pan"));
    float ampStri = getInternalParameterValue("ampStri");
    float ampUp = getInternalParameterValue("ampUp");
    float ampLow = getInternalParameterValue("ampLow");
    float amp = getInternalParameterValue("amp");
    while(io()){
      float s1 = (mOsc1() + mOsc2() + mOsc3()) * mEnvStri() * ampStri;
      s1 += (mOsc4() + mOsc5()) * mEnvLow() * ampLow;
      s1 += (mOsc6() + mOsc7() + mOsc8() + mOsc9()) * mEnvUp() * ampUp;
      s1 *= amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1,s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    //if(mEnvStri.done()) free();
    if(mEnvStri.done() && mEnvUp.done() && mEnvLow.done() && (mEnvFollow.value() < 0.001)) free();
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

    mEnvStri.attack(getInternalParameterValue("attackStri"));
    mEnvStri.decay(getInternalParameterValue("attackStri"));
    mEnvStri.sustain(getInternalParameterValue("sustainStri"));
    mEnvStri.release(getInternalParameterValue("releaseStri"));

    mEnvLow.attack(getInternalParameterValue("attackLow"));
    mEnvLow.decay(getInternalParameterValue("attackLow"));
    mEnvLow.sustain(getInternalParameterValue("sustainLow"));
    mEnvLow.release(getInternalParameterValue("releaseLow"));

    mEnvUp.attack(getInternalParameterValue("attackUp"));
    mEnvUp.decay(getInternalParameterValue("attackUp"));
    mEnvUp.sustain(getInternalParameterValue("sustainUp"));
    mEnvUp.release(getInternalParameterValue("releaseUp"));

    mPan.pos(getInternalParameterValue("pan"));

    mEnvStri.reset();
    mEnvLow.reset();
    mEnvUp.reset();
  }

  virtual void onTriggerOff() override {
//    std::cout << "trigger off" <<std::endl;
    mEnvStri.triggerRelease();
    mEnvLow.triggerRelease();
    mEnvUp.triggerRelease();
  }


};



class MyApp : public App
{
public:
  SynthGUIManager<AddSyn> synthManager {"synth7"};
  //    ParameterMIDI parameterMIDI;
  int midiNote;
  float harmonicSeriesScale[20];
  float halfStepScale[20];
  float halfStepInterval = 1.05946309; // 2^(1/12)


  virtual void onInit( ) override {
    imguiInit();
    navControl().active(false);  // Disable navigation via keyboard, since we
                              // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

    void onCreate() override {
        initScaleToHarmonicSeries();
        initScaleTo12TET(110);
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth7.synthSequence");
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


  void initScaleToHarmonicSeries() {
    for (int i=0;i<20;++i) {
      harmonicSeriesScale[i] = 100*i;
    }
  }

  void initScaleTo12TET(float lowest) {
    float f = lowest;
    for (int i=0;i<20;++i) {
      halfStepScale[i] = f;
      f *= halfStepInterval;
    }
  }

  float randomFrom12TET() {
    int index = gam::rnd::uni(0,20);
    // std::cout << "index " << index << " is " << myScale[index] << std::endl;
    return halfStepScale[index];
  }

  float randomFromHarmonicSeries() {
      int index = gam::rnd::uni(0,20);
      // std::cout << "index " << index << " is " << myScale[index] << std::endl;
      return harmonicSeriesScale[index];
  }

  void fillTime(float from, float to, float minattackStri, float minattackLow, float minattackUp, float maxattackStri, float maxattackLow, float maxattackUp, float minFreq, float maxFreq) {
        while (from <= to) {
            float nextAtt = gam::rnd::uni((minattackStri+minattackLow+minattackUp),(maxattackStri+maxattackLow+maxattackUp));
            auto *voice = synthManager.synth().getVoice<AddSyn>();
            voice->setTriggerParams({0.03,440, 0.5,0.0001,3.8,0.3,   0.4,0.0001,6.0,0.99,  0.3,0.0001,6.0,0.9,  2,3,4.07,0.56,0.92,1.19,1.7,2.75,3.36, 0.0});
            voice->setInternalParameterValue("attackStr", nextAtt);
            voice->setInternalParameterValue("freq", gam::rnd::uni(minFreq,maxFreq));
            synthManager.synthSequencer().addVoiceFromNow(voice, from, 0.2);
            std::cout << "old from " << from << " plus nextnextAtt " << nextAtt << std::endl;
            from += nextAtt;
        }
  }

  void fillTimeWith12TET(float from, float to, float minattackStri, float minattackLow, float minattackUp, float maxattackStri, float maxattackLow, float maxattackUp) {
      while (from <= to) {

        float nextAtt = gam::rnd::uni((minattackStri+minattackLow+minattackUp),(maxattackStri+maxattackLow+maxattackUp));
        auto *voice = synthManager.synth().getVoice<AddSyn>();
        voice->setTriggerParams({0.03,440, 0.5,0.0001,3.8,0.3,   0.4,0.0001,6.0,0.99,  0.3,0.0001,6.0,0.9,  2,3,4.07,0.56,0.92,1.19,1.7,2.75,3.36, 0.0});
        voice->setInternalParameterValue("attackStr", nextAtt);
        voice->setInternalParameterValue("freq", randomFrom12TET());
        synthManager.synthSequencer().addVoiceFromNow(voice, from, 0.2);
        std::cout << "12 old from " << from << " plus nextAtt " << nextAtt << std::endl;
        from += nextAtt;
      }
  }

};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}
