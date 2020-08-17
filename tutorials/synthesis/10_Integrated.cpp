
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
    tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048), tbPls(2048), tbDin(2048),
    tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);
class OscEnv : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Osc<> mOsc;
  gam::ADSR<> mAmpEnv;
  gam::EnvFollow<>
      mEnvFollow;  // envelope follower to connect audio output to graphics

  // Additional members
  Mesh mMesh;

  // Initialize voice. This function will nly be called once per voice
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 0.3, 0.3,
                   0);  // These tables are not normalized, so scale to 0.3
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
    createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("table", 0, 0, 8);
  }

  //
  virtual void onProcess(AudioIOData& io) override {
    updateFromParameters();
    while (io()) {
      float s1 =
          0.1 * mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
  }

  void onProcess(Graphics& g) override {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    g.pushMatrix();
    g.translate(amplitude+frequency/2000, amplitude/2+frequency/2000, -4);
    g.scale(0.1, 0.1, 0.1);
    g.color(mEnvFollow.value(), frequency / 2000, mEnvFollow.value() * 10, 0.6);
    g.draw(mMesh);
    g.popMatrix();
  }

  virtual void onTriggerOn() override {
    mAmpEnv.reset();
    updateFromParameters();
    // Map table number to table in memory
    switch (int(getInternalParameterValue("table"))) {
      case 0:
        mOsc.source(tbSaw);
        break;
      case 1:
        mOsc.source(tbSqr);
        break;
      case 2:
        mOsc.source(tbImp);
        break;
      case 3:
        mOsc.source(tbSin);
        break;
      case 4:
        mOsc.source(tbPls);
        break;
      case 5:
        mOsc.source(tb__1);
        break;
      case 6:
        mOsc.source(tb__2);
        break;
      case 7:
        mOsc.source(tb__3);
        break;
      case 8:
        mOsc.source(tb__4);
        break;
    }
  }

  virtual void onTriggerOff() override { mAmpEnv.triggerRelease(); }

  void updateFromParameters() {
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.decay(getInternalParameterValue("attackTime"));
    mAmpEnv.release(getInternalParameterValue("releaseTime"));
    mAmpEnv.sustain(getInternalParameterValue("sustain"));
    mAmpEnv.curve(getInternalParameterValue("curve"));
    mPan.pos(getInternalParameterValue("pan"));
  }
};


class Vib : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Osc<> mOsc;
  gam::Sine<> mVib;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mVibEnv;
  gam::EnvFollow<> mEnvFollow;

  float vibValue;

  // Additional members
  Mesh mMesh;

  void init() override {
    mAmpEnv.curve(0);  // linear segments
    mAmpEnv.levels(0, 1, 1, 0);
    mVibEnv.curve(0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
    createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("table", 0, 0, 8);
    createInternalTriggerParameter("vibRate1", 3.5, 0.2, 20);
    createInternalTriggerParameter("vibRate2", 5.8, 0.2, 20);
    createInternalTriggerParameter("vibRise", 0.5, 0.1, 2);
    createInternalTriggerParameter("vibDepth", 0.005, 0.0, 0.3);
  }

  void onProcess(AudioIOData& io) override {
    float oscFreq = getInternalParameterValue("frequency");
    float amp = getInternalParameterValue("amplitude");
    float vibDepth = getInternalParameterValue("vibDepth");
    while (io()) {
      mVib.freq(mVibEnv());
      vibValue = mVib();
      mOsc.freq(oscFreq + vibValue * vibDepth * oscFreq);

      float s1 = mOsc() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // if(mAmpEnv.done()) free();
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    g.pushMatrix();
    g.translate(amplitude, amplitude, -4);
    float scaling = vibValue + getInternalParameterValue("vibDepth");
    g.scale(scaling * frequency / 200, scaling * frequency / 400, scaling * 1);
    g.color(mEnvFollow.value(), frequency / 1000, mEnvFollow.value() * 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    //        mAmpEnv.totalLength(mDur, 1);
    updateFromParameters();

    mAmpEnv.reset();
    mVibEnv.reset();
    // Map table number to table in memory
    switch (int(getInternalParameterValue("table"))) {
      case 0:
        mOsc.source(tbSaw);
        break;
      case 1:
        mOsc.source(tbSqr);
        break;
      case 2:
        mOsc.source(tbImp);
        break;
      case 3:
        mOsc.source(tbSin);
        break;
      case 4:
        mOsc.source(tbPls);
        break;
      case 5:
        mOsc.source(tb__1);
        break;
      case 6:
        mOsc.source(tb__2);
        break;
      case 7:
        mOsc.source(tb__3);
        break;
      case 8:
        mOsc.source(tb__4);
        break;
    }
  }

  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mVibEnv.triggerRelease();
  }

  void updateFromParameters() {
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.decay(getInternalParameterValue("attackTime"));
    mAmpEnv.release(getInternalParameterValue("releaseTime"));
    mAmpEnv.curve(getInternalParameterValue("curve"));
    mPan.pos(getInternalParameterValue("pan"));
    mVibEnv.levels(getInternalParameterValue("vibRate1"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate1"));
    mVibEnv.lengths()[0] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[1] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[3] = getInternalParameterValue("vibRise");
  }
};


class FM : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::ADSR<> mVibEnv;

  gam::Sine<> car, mod, mVib;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;
  float mDur;
  float mModAmt = 50;
  float mVibFrq;
  float mVibDepth;
  float mVibRise;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments

    mAmpEnv.levels(0, 1, 1, 0);
    mModEnv.levels(0, 1, 1, 0);
    mVibEnv.levels(0, 1, 1, 0);
    //      mVibEnv.curve(0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("frequency", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.75, 0.1, 1.0);

    // FM index
    createInternalTriggerParameter("idx1", 0.01, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 7, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 5, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 1, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 1.0007, 0.0, 20.0);

    createInternalTriggerParameter("vibRate1", 0.01, 0.0, 10.0);
    createInternalTriggerParameter("vibRate2", 0.5, 0.0, 10.0);
    createInternalTriggerParameter("vibRise", 0, 0.0, 10.0);
    createInternalTriggerParameter("vibDepth", 0, 0.0, 10.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    updateFromParameters();
    mVib.freq(mVibEnv());
    float carBaseFreq =
        getInternalParameterValue("frequency") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("frequency") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      mVib.freq(mVibEnv());
      car.freq((1 + mVib() * mVibDepth) * carBaseFreq +
               mod() * mModEnv() * modScale);
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
    g.translate(getInternalParameterValue("frequency") / 300 - 2,
                (getInternalParameterValue("idx3") +
                 getInternalParameterValue("idx2")) /
                        15 -
                    1,
                -4);
    float scaling = getInternalParameterValue("amplitude") / 3;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    updateFromParameters();

    float modFreq =
        getInternalParameterValue("frequency") * getInternalParameterValue("modMul");
    mod.freq(modFreq);

    mVibEnv.lengths()[0] = mDur * (1 - mVibRise);
    mVibEnv.lengths()[1] = mDur * mVibRise;
    mAmpEnv.reset();
    mVibEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
    mVibEnv.triggerRelease();
  }

  void updateFromParameters() {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.levels()[1] = 1.0;
    // mAmpEnv.levels()[2] = 1.0;
    mAmpEnv.levels()[2] = getInternalParameterValue("sustain");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[3] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[3] = getInternalParameterValue("releaseTime");

    mAmpEnv.totalLength(getInternalParameterValue("dur"), 1);
    mModEnv.lengths()[1] = mAmpEnv.lengths()[1];
    mVibEnv.levels()[1] = getInternalParameterValue("vibRate1");
    mVibEnv.levels()[2] = getInternalParameterValue("vibRate2");
    mVibDepth = getInternalParameterValue("vibDepth");
    mVibRise = getInternalParameterValue("vibRise");
    mPan.pos(getInternalParameterValue("pan"));

  }
};

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


class PluckedString : public SynthVoice {
public:
    float mAmp;
    float mDur;
    float mPanRise;
    gam::Pan<> mPan;
    gam::NoiseWhite<> noise;
    gam::Decay<> env;
    gam::MovingAvg<> fil {2};
    gam::Delay<float, gam::ipl::Trunc> delay;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;
    gam::Env<2> mPanEnv;

    // Additional members
    Mesh mMesh;

    virtual void init(){
        mAmp  = 1;
        mDur = 2;
        mAmpEnv.curve(4); // make segments lines
        mAmpEnv.levels(1,1,0);
        mPanEnv.curve(4);
        env.decay(0.1);
        delay.maxDelay(1./27.5);
        delay.delay(1./440.0);


        addDisc(mMesh, 1.0, 30);
        createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.001, 0.001, 1.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("Pan1", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("Pan2", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("PanRise", 0.0, -1.0, 1.0); // range check
    }
    
//    void reset(){ env.reset(); }

    float operator() (){
        return (*this)(noise()*env());
    }
    float operator() (float in){
        return delay(
                     fil( delay() + in )
                     );
    }

    virtual void onProcess(AudioIOData& io) override {

        while(io()){
            mPan.pos(mPanEnv());
            float s1 =  (*this)() * mAmpEnv() * mAmp;
            float s2;
            mEnvFollow(s1);
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
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
        updateFromParameters();
        mAmpEnv.reset();
        env.reset();
        delay.zero();
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
    }

    void updateFromParameters() {
        mPanEnv.levels(getInternalParameterValue("Pan1"),
                       getInternalParameterValue("Pan2"),
                       getInternalParameterValue("Pan1"));
        mPanRise = getInternalParameterValue("PanRise");
        delay.freq(getInternalParameterValue("frequency"));
        mAmp = getInternalParameterValue("amplitude");
        mAmpEnv.levels()[1] = 1.0;
        mAmpEnv.levels()[2] = getInternalParameterValue("sustain");
        mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
        mAmpEnv.lengths()[3] = getInternalParameterValue("releaseTime");

        mPanEnv.lengths()[0] = mDur * (1-mPanRise);
        mPanEnv.lengths()[1] = mDur * mPanRise;
    }

};

class MyApp : public App 
{
    public:
    SynthGUIManager<OscTrm> synthManager {"integrated_inst"};
    //    ParameterMIDI parameterMIDI;
    int midiNote;
    //    ParameterMIDI parameterMIDI;
    float harmonicSeriesScale[20];
    float halfStepScale[20];
    float halfStepInterval = 1.05946309; // 2^(1/12)

    virtual void onInit( ) override {
        imguiInit();
        navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering
        // Set sampling rate for Gamma objects from app's audio
        gam::sampleRate(audioIO().framesPerSecond());
        // Additive Synth Related
        initScaleToHarmonicSeries();
        initScaleTo12TET(110);
        // Tremolo 
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

        {    float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
          float C[] = {10, 27, 54, 81, 108, 135};
          addSines(tbDin, A,C,6);
        }        
    }
    void onCreate() override {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth2.synthSequence");
        synthManager.synthRecorder().verbose(true);
        // Add another class used
        synthManager.synth().registerSynthClass<OscEnv>();
        synthManager.synth().registerSynthClass<Vib>();
        synthManager.synth().registerSynthClass<FM>();
        synthManager.synth().registerSynthClass<OscAM>();
//        synthManager.synth().registerSynthClass<OscTrm>();
        synthManager.synth().registerSynthClass<AddSyn>();
        synthManager.synth().registerSynthClass<Sub>();
        synthManager.synth().registerSynthClass<AddSyn>();
        synthManager.synth().registerSynthClass<PluckedString>();

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
            voice->setInternalParameterValue("frequency", gam::rnd::uni(minFreq,maxFreq));
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
        voice->setInternalParameterValue("frequency", randomFrom12TET());
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
