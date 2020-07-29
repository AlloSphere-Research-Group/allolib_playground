
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

// using namespace gam;
using namespace al;
using namespace std;

// tables for oscillator
gam::ArrayPow2<float>
    tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048), tbPls(2048),
    tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);


class OscTrm : public SynthVoice {
public:

    // Unit generators
    gam::Pan<> mPan;
    gam::Sine<> mTrm;
    gam::Osc<> mOsc;
    gam::ADSR<> mTrmEnv;
    //gam::Env<2> mTrmEnv;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics

    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    virtual void init() {

        // Intialize envelope
        mAmpEnv.curve(0); // make segments lines
        mAmpEnv.levels(0,0.3,0.3,0); // These tables are not normalized, so scale to 0.3
//        mAmpEnv.sustainPoint(1); // Make point 2 sustain until a release is issued

        mTrmEnv.curve(0);
        mTrmEnv.levels(0,1,1,0);
//        mTrmEnv.sustainPoint(1); // Make point 2 sustain until a release is issued

        // We have the mesh be a sphere
        addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("table", 0, 0, 8);
        createInternalTriggerParameter("trm1", 3.5, 0.2, 20);
        createInternalTriggerParameter("trm2", 5.8, 0.2, 20);
        createInternalTriggerParameter("trmRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("trmDepth", 0.1, 0.0, 1.0);
    }

    //
    virtual void onProcess(AudioIOData& io) override {
        //updateFromParameters();
        float oscFreq = getInternalParameterValue("frequency");
        float amp = getInternalParameterValue("amplitude");
        float trmDepth = getInternalParameterValue("trmDepth");
         while(io()){

            mTrm.freq(mTrmEnv());
            //float trmAmp = mAmp - mTrm()*mTrmDepth; // Replaced with line below
            float trmAmp = (mTrm()*0.5+0.5)*trmDepth + (1-trmDepth); // Corrected
            float s1 = mOsc() * mAmpEnv() * trmAmp * amp;
            float s2;
            mEnvFollow(s1);
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        // We need to let the synth know that this voice is done
        // by calling the free(). This takes the voice out of the
        // rendering chain
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

    virtual void onProcess(Graphics &g) {
            float frequency = getInternalParameterValue("frequency");
            float amplitude = getInternalParameterValue("amplitude");
            g.pushMatrix();
            g.translate(amplitude,  amplitude, -4);
            //g.scale(frequency/2000, frequency/4000, 1);
            float scaling = getInternalParameterValue("trmDepth");
            g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
            g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
            g.draw(mMesh);
            g.popMatrix();
     }

    virtual void onTriggerOn() override {
        updateFromParameters();

        mAmpEnv.reset();
        mTrmEnv.reset();
        
        // Map table number to table in memory
        switch (int(getInternalParameterValue("table"))) {
        case 0: mOsc.source(tbSaw); break;
        case 1: mOsc.source(tbSqr); break;
        case 2: mOsc.source(tbImp); break;
        case 3: mOsc.source(tbSin); break;
        case 4: mOsc.source(tbPls); break;
        case 5: mOsc.source(tb__1); break;
        case 6: mOsc.source(tb__2); break;
        case 7: mOsc.source(tb__3); break;
        case 8: mOsc.source(tb__4); break;
        }
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
        mTrmEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
        mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.sustain(getInternalParameterValue("sustain"));
        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));

        mTrmEnv.levels(getInternalParameterValue("trm1"),
                       getInternalParameterValue("trm2"),
                       getInternalParameterValue("trm2"),
                       getInternalParameterValue("trm1"));

        mTrmEnv.attack(getInternalParameterValue("trmRise"));
        mTrmEnv.decay(getInternalParameterValue("trmRise"));
        mTrmEnv.release(getInternalParameterValue("trmRise"));
    }
};


// We make an app.
class MyApp : public App
{
    public:
    SynthGUIManager<OscTrm> synthManager {"synth5"};
    //    ParameterMIDI parameterMIDI;
    int midiNote;


    virtual void onInit( ) override {
        imguiInit();
        navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering
        // Set sampling rate for Gamma objects from app's audio
        gam::sampleRate(audioIO().framesPerSecond());


        gam::addSinesPow<1>(tbSaw, 9,1);
        gam::addSinesPow<1>(tbSqr, 9,2);
        gam::addSinesPow<0>(tbImp, 9,1);
        gam::addSine(tbSin);

        {    float A[] = {1,1,1,1,0.7,0.5,0.3,0.1};
            gam::addSines(tbPls, A,8);
        }

        {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
            float C[] = {1,4,7,11,15,18};
            gam::addSines(tb__1, A,C,6);
        }

        // inharmonic partials
        {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12};
            float C[] = {3,4,7,8,11,12,15,16};
            gam::addSines(tb__2, A,C,8);
        }

        // inharmonic partials
        {    float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
            float C[] = {10, 27, 54, 81, 108, 135};
            gam::addSines(tb__3, A,C,6);
        }

        // harmonics 20-27
        {    float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
            gam::addSines(tb__4, A,8, 20);
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
