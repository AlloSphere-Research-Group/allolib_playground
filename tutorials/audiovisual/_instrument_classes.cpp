// Just Instrument Classes

#include <cstdio> // for printing to stdout

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"

using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048
// tables for oscillator
gam::ArrayPow2<float> tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048), tbDin(2048),
    tbPls(2048), tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);
Vec3f randomVec3f(float scale)
{
  return Vec3f(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS()) * scale;
}
// 01_SineEnv
class SineEnv : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;
  // Draw parameters
  Mesh mMesh;
  double a = 0;
  double b = 0;
  double timepose = 0;
  Vec3f note_position;
  Vec3f note_direction;

  // Additional members
  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addSphere(mMesh, 0.3, 50, 50);
    mMesh.decompress();
    mMesh.generateNormals();

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 1.0, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    // Initalize MIDI device input
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override
  {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f))
      free();
  }

  // The graphics processing function
  void onProcess(Graphics &g) override
  {
    a += 0.29;
    b += 0.23;
    timepose += 0.02;
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    // Now draw
    g.pushMatrix();
    g.depthTesting(true);
    g.lighting(true);
    g.translate(note_position + note_direction * timepose);
    g.rotate(a, Vec3f(0, 1, 0));
    g.rotate(b, Vec3f(1));
    g.scale(0.3 + mAmpEnv() * 0.2, 0.3 + mAmpEnv() * 0.5, amplitude);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override
  {
    float angle = getInternalParameterValue("frequency") / 200;
    mAmpEnv.reset();
    a = al::rnd::uniform();
    b = al::rnd::uniform();
    timepose = 0;
    note_position = {0, 0, -15};
    note_direction = {sin(angle), cos(angle), 0};
  }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// 02_OscEnv
class OscEnv : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Osc<> mOsc;
  gam::ADSR<> mAmpEnv;
  gam::EnvFollow<>
      mEnvFollow;  // envelope follower to connect audio output to graphics
  int mtable;
  // Additional members
  static const int numb_waveform = 9;
  Mesh mMesh[numb_waveform];
  bool wireframe = false;
  bool vertexLight = false;
  double a_rotate = 0;
  double b_rotate = 0;
  double timepose = 0;

  // Initialize voice. This function will nly be called once per voice
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 0.3, 0.3,
                   0);  // These tables are not normalized, so scale to 0.3
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued

    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 1.0, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
    createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("table", 0, 0, 8);

    // Table & Visual meshes
    // Now We have the mesh according to the waveform
    gam::addSinesPow<1>(tbSaw, 9, 1);
    addCone(mMesh[0],1, Vec3f(0,0,5), 40, 1); //tbSaw

    gam::addSinesPow<1>(tbSqr, 9, 2);
    addCube(mMesh[1]);  // tbSquare

    gam::addSinesPow<0>(tbImp, 9, 1);
    addPrism(mMesh[2],1,1,1,100); // tbImp

    gam::addSine(tbSin);
    addSphere(mMesh[3], 0.3, 16, 100); // tbSin

// About: addSines (dst, amps, cycs, numh)
// \param[out] dst		destination array
// \param[in] amps		harmonic amplitudes of series, size must be numh - A[]
// \param[in] cycs		harmonic numbers of series, size must be numh - C[]
// \param[in] numh		total number of harmonics
    float scaler = 0.15;
    float hscaler = 1;

    { //tbPls
      float A[] = {1, 1, 1, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tbPls, A, 8); 
      addWireBox(mMesh[4],2);    // tbPls
    }
    { // tb__1 
      float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08, 0, 0};
      float C[] = {1, 4, 7, 11, 15, 18, 0, 0 };
      gam::addSines(tb__1, A, C, 6);
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[5], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);

        // addSphere(mMesh[5],scaler * A[i], 16, 30); // tb__1
      }
    }
    { // inharmonic partials
      float A[] = {0.5, 0.8, 0.7, 1, 0.3, 0.4, 0.2, 0.12};
      float C[] = {3, 4, 7, 8, 11, 12, 15, 16}; 
      gam::addSines(tb__2, A, C, 8); // tb__2
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[6], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
    { // inharmonic partials
      float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08, 0 , 0};
      float C[] = {10, 27, 54, 81, 108, 135, 0, 0};
      gam::addSines(tb__3, A, C, 6); // tb__3
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[7], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
  { // harmonics 20-27
      float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tb__4, A, 8, 20); // tb__4
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[8], hscaler * A[i], hscaler * A[i+1], 1 + 0.3*i);
      }
    }
    // { // Write your own waveform!
    //   float A[] = {1, 1, 1, 1, 1, 1};
    //   float C[] = {2, 3, 5, 7, 11, 13}; // prime numbers?
    //   gam::addSines(tb__3, A, C, 6);
    //   addPrism(mMesh[7], scaler * A[0]*C[0], scaler * A[1]*C[1], 1, 100, 27);// tb__2
    //   addPrism(mMesh[7], scaler * A[2]*C[2], scaler * A[3]*C[3], 2, 100, 27*2);// tb__2
    //   addPrism(mMesh[7], scaler * A[4]*C[4], scaler * A[5]*C[5], 3, 100, 27*3); // tb__3
    // }


//int addSurfaceLoop(Mesh& m, int Nx, int Ny, int loopMode, double width, double height, double x, double y) 


    // Scale and generate normals
    for (int i = 0; i < numb_waveform; ++i) {
      mMesh[i].scale(0.4);

      int Nv = mMesh[i].vertices().size();
      for (int k = 0; k < Nv; ++k) {
        mMesh[i].color(HSV(float(k) / Nv, 0.3, 1));
      }

      if (!vertexLight && mMesh[i].primitive() == Mesh::TRIANGLES) {
        mMesh[i].decompress();
      }
      mMesh[i].generateNormals();
    }
  }

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
    a_rotate += 0.81;
    b_rotate += 0.78;
    timepose -= 0.06;
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    int shape = getInternalParameterValue("table");

    // static Light light;
    g.polygonMode(wireframe ? GL_LINE : GL_FILL);
    // light.pos(0, 0, 0);
    gl::depthTesting(true);
    g.lighting(true);
    // g.light(light);
    g.pushMatrix();
    g.depthTesting(true);
    g.translate( timepose, getInternalParameterValue("frequency") / 200 - 3 , -15);
    g.rotate(a_rotate, Vec3f(0, 1, 1));
    g.rotate(b_rotate, Vec3f(1));    
    g.scale(0.5 + mAmpEnv() * 2, 0.5 + mAmpEnv() * 2, 0.03 + 0.1*mAmpEnv() );
    g.color(HSV(frequency / 1000, 0.6 + mAmpEnv() * 0.1, 0.6 + 0.5 * mAmpEnv()));
    g.draw(mMesh[shape]);
    g.popMatrix();
  } 

  virtual void onTriggerOn() override {
    mAmpEnv.reset();
    updateFromParameters();
    updateWaveform();
    timepose = 10;
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
  void updateWaveform(){
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

};

// 03_Vib
class Vib : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Osc<> mOsc;
  gam::Sine<> mVib;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mVibEnv;
  gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
  int mtable;
  // Additional members
  static const int numb_waveform = 9;
  Mesh mMesh[numb_waveform];
  bool wireframe = false;
  bool vertexLight = false;
  double a_rotate = 0;
  double b_rotate = 0;
  double timepose = 0;
  float vibValue;
  float outFreq;
  
  // Initialize voice. This function will nly be called once per voice
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 0.3, 0.3,
                   0);  // These tables are not normalized, so scale to 0.3
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued
    mVibEnv.curve(0);

    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 1.0, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
    createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("table", 0, 0, 8);
    createInternalTriggerParameter("vibRate1", 3.5, 0.2, 20);
    createInternalTriggerParameter("vibRate2", 5.8, 0.2, 20);
    createInternalTriggerParameter("vibRise", 0.5, 0.1, 2);
    createInternalTriggerParameter("vibDepth", 0.005, 0.0, 0.3);

    // Table & Visual meshes
    // Now We have the mesh according to the waveform
    gam::addSinesPow<1>(tbSaw, 9, 1);
    addCone(mMesh[0],1, Vec3f(0,0,5), 40, 1); //tbSaw

    gam::addSinesPow<1>(tbSqr, 9, 2);
    addCube(mMesh[1]);  // tbSquare

    gam::addSinesPow<0>(tbImp, 9, 1);
    addPrism(mMesh[2],1,1,1,100); // tbImp

    gam::addSine(tbSin);
    addSphere(mMesh[3], 0.3, 16, 100); // tbSin

// About: addSines (dst, amps, cycs, numh)
// \param[out] dst		destination array
// \param[in] amps		harmonic amplitudes of series, size must be numh - A[]
// \param[in] cycs		harmonic numbers of series, size must be numh - C[]
// \param[in] numh		total number of harmonics
    float scaler = 0.15;
    float hscaler = 1;

    { //tbPls
      float A[] = {1, 1, 1, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tbPls, A, 8); 
      addWireBox(mMesh[4],2);    // tbPls
    }
    { // tb__1 
      float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08, 0, 0};
      float C[] = {1, 4, 7, 11, 15, 18, 0, 0 };
      gam::addSines(tb__1, A, C, 6);
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[5], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);

        // addSphere(mMesh[5],scaler * A[i], 16, 30); // tb__1
      }
    }
    { // inharmonic partials
      float A[] = {0.5, 0.8, 0.7, 1, 0.3, 0.4, 0.2, 0.12};
      float C[] = {3, 4, 7, 8, 11, 12, 15, 16}; 
      gam::addSines(tb__2, A, C, 8); // tb__2
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[6], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
    { // inharmonic partials
      float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08, 0 , 0};
      float C[] = {10, 27, 54, 81, 108, 135, 0, 0};
      gam::addSines(tb__3, A, C, 6); // tb__3
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[7], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
  { // harmonics 20-27
      float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tb__4, A, 8, 20); // tb__4
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[8], hscaler * A[i], hscaler * A[i+1], 1 + 0.3*i);
      }
    }

    // Scale and generate normals
    for (int i = 0; i < numb_waveform; ++i) {
      mMesh[i].scale(0.4);

      int Nv = mMesh[i].vertices().size();
      for (int k = 0; k < Nv; ++k) {
        mMesh[i].color(HSV(float(k) / Nv, 0.3, 1));
      }

      if (!vertexLight && mMesh[i].primitive() == Mesh::TRIANGLES) {
        mMesh[i].decompress();
      }
      mMesh[i].generateNormals();
    }
  }

  //
  virtual void onProcess(AudioIOData& io) override {
    updateFromParameters();
    float oscFreq = getInternalParameterValue("frequency");
    float vibDepth = getInternalParameterValue("vibDepth");
    outFreq = oscFreq + vibValue * vibDepth * oscFreq;
    while (io()) {
      mVib.freq(mVibEnv());
      vibValue = mVib();
       mOsc.freq(outFreq);
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
    a_rotate += 0.81;
    b_rotate += 0.78;
    timepose -= 0.06;
    int shape = getInternalParameterValue("table");
    // static Light light;
    g.polygonMode(wireframe ? GL_LINE : GL_FILL);
    // light.pos(0, 0, 0);
    gl::depthTesting(true);
    g.lighting(true);
    // g.light(light);
    g.pushMatrix();
    g.depthTesting(true);
    g.translate( timepose, outFreq / 200 - 3 , -15);
    g.rotate(a_rotate, Vec3f(0, 1, 1));
    g.rotate(b_rotate, Vec3f(1));    
    g.scale(0.5 + mAmpEnv() * 2, 0.5 + mAmpEnv() * 2, 0.03 + 0.1*mAmpEnv() );
    g.color(HSV(outFreq / 1000, 0.6 + mAmpEnv() * 0.1, 0.6 + 0.5 * mAmpEnv()));
    g.draw(mMesh[shape]);
    g.popMatrix();
  } 

  virtual void onTriggerOn() override {
    mAmpEnv.reset();
    mVibEnv.reset();
    updateFromParameters();
    updateWaveform();
    timepose = 10;
  }

  virtual void onTriggerOff() override { 
    mAmpEnv.triggerRelease(); 
    mVibEnv.triggerRelease();
  }

  void updateFromParameters() {
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.decay(getInternalParameterValue("attackTime"));
    mAmpEnv.release(getInternalParameterValue("releaseTime"));
    mAmpEnv.sustain(getInternalParameterValue("sustain"));
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
  void updateWaveform(){
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

};

// 04_FMvib
class FM : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::ADSR<> mVibEnv;

  gam::Sine<> car, mod, mVib; // carrier, modulator sine oscillators
  double a = 0;
  double b = 0;
  double timepose = 10;
  Mesh ball;

  // Additional members
  float mVibFrq;
  float mVibDepth;
  float mVibRise;

  void init() override
  {
    mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);
 
    mModEnv.levels(0, 1, 1, 0);
    mVibEnv.levels(0, 1, 1, 0);
    //      mVibEnv.curve(0);
    addSphere(ball, 1, 100, 100);
    ball.decompress();
    ball.generateNormals();

    // We have the mesh be a sphere
    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.05, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.5, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.65, 0.1, 1.0);

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
  void onProcess(AudioIOData &io) override
  {
    mVib.freq(mVibEnv());
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io())
    {
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
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001))
      free();
  }

  void onProcess(Graphics &g) override
  {
    a += 0.29;
    b += 0.23;
    timepose -= 0.06;
    g.pushMatrix();
    g.depthTesting(true);
    g.lighting(true);
    g.translate(timepose, getInternalParameterValue("freq") / 200 - 3, -15);
    g.rotate(mVib() + a, Vec3f(0, 1, 0));
    g.rotate(mVibDepth + b, Vec3f(1));
    float scaling = getInternalParameterValue("amplitude") / 10;
    g.scale(scaling + getInternalParameterValue("modMul") / 10, scaling + getInternalParameterValue("carMul") / 30, scaling + mEnvFollow.value() * 5);
    g.color(HSV(getInternalParameterValue("modMul") / 20, getInternalParameterValue("carMul") / 20, 0.5 + getInternalParameterValue("attackTime")));
    g.draw(ball);
    g.popMatrix();
  }

  void onTriggerOn() override
  {
    timepose = 10;
    mAmpEnv.reset();
    mVibEnv.reset();
    mModEnv.reset();
    mVib.phase(0);
    mod.phase(0);
    updateFromParameters();

    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
  }
  void onTriggerOff() override
  {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
    mVibEnv.triggerRelease();
  }

  void updateFromParameters()
  {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.release(getInternalParameterValue("releaseTime"));
    mAmpEnv.sustain(getInternalParameterValue("sustain"));

    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[3] = getInternalParameterValue("releaseTime");

    mVibEnv.levels(getInternalParameterValue("vibRate1"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate1"));
    mVibEnv.lengths()[0] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[1] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[3] = getInternalParameterValue("vibRise");
    mVibDepth = getInternalParameterValue("vibDepth");
    
    mPan.pos(getInternalParameterValue("pan"));
  }
};

// 04_FMvib_wavetable
class FMWT : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::ADSR<> mVibEnv;

  gam::Sine<> mod, mVib; // carrier, modulator sine oscillators
  gam::Osc<> car;
  double a = 0;
  double b = 0;
  double timepose = 10;
  // Additional members
  float mVibFrq;
  float mVibDepth;
  float mVibRise;
  int mtable;
  static const int numb_waveform = 9;
  Mesh mMesh[numb_waveform];
  bool wireframe = false;
  bool vertexLight = false;

  void init() override
  {
    //      mAmpEnv.curve(0); // linear segments
    // mAmpEnv.levels(0, 1, 1, 0);
    mModEnv.levels(0, 1, 1, 0);
    mVibEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);

    // We have the mesh be a sphere
    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.3, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.65, 0.1, 1.0);

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
    createInternalTriggerParameter("table", 0, 0, 8);

    // Table & Visual meshes
    // Now We have the mesh according to the waveform
    gam::addSinesPow<1>(tbSaw, 9, 1);
    addCone(mMesh[0],1, Vec3f(0,0,5), 40, 1); //tbSaw

    gam::addSinesPow<1>(tbSqr, 9, 2);
    addCube(mMesh[1]);  // tbSquare

    gam::addSinesPow<0>(tbImp, 9, 1);
    addPrism(mMesh[2],1,1,1,100); // tbImp

    gam::addSine(tbSin);
    addSphere(mMesh[3], 0.3, 16, 100); // tbSin

// About: addSines (dst, amps, cycs, numh)
// \param[out] dst		destination array
// \param[in] amps		harmonic amplitudes of series, size must be numh - A[]
// \param[in] cycs		harmonic numbers of series, size must be numh - C[]
// \param[in] numh		total number of harmonics
    float scaler = 0.15;
    float hscaler = 1;

    { //tbPls
      float A[] = {1, 1, 1, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tbPls, A, 8); 
      addWireBox(mMesh[4],2);    // tbPls
    }
    { // tb__1 
      float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08, 0, 0};
      float C[] = {1, 4, 7, 11, 15, 18, 0, 0 };
      gam::addSines(tb__1, A, C, 6);
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[5], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);

        // addSphere(mMesh[5],scaler * A[i], 16, 30); // tb__1
      }
    }
    { // inharmonic partials
      float A[] = {0.5, 0.8, 0.7, 1, 0.3, 0.4, 0.2, 0.12};
      float C[] = {3, 4, 7, 8, 11, 12, 15, 16}; 
      gam::addSines(tb__2, A, C, 8); // tb__2
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[6], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
    { // inharmonic partials
      float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08, 0 , 0};
      float C[] = {10, 27, 54, 81, 108, 135, 0, 0};
      gam::addSines(tb__3, A, C, 6); // tb__3
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[7], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1 + 0.3*i);
      }
    }
  { // harmonics 20-27
      float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tb__4, A, 8, 20); // tb__4
      for (int i = 0; i < 7; i++){
        addWireBox(mMesh[8], hscaler * A[i], hscaler * A[i+1], 1 + 0.3*i);
      }
    }

    // Scale and generate normals
    for (int i = 0; i < numb_waveform; ++i) {
      mMesh[i].scale(0.4);

      int Nv = mMesh[i].vertices().size();
      for (int k = 0; k < Nv; ++k) {
        mMesh[i].color(HSV(float(k) / Nv, 0.3, 1));
      }

      if (!vertexLight && mMesh[i].primitive() == Mesh::TRIANGLES) {
        mMesh[i].decompress();
      }
      mMesh[i].generateNormals();
    }


  }

  //
  void onProcess(AudioIOData &io) override
  {
    mVib.freq(mVibEnv());
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale = getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude") * 0.01;
    while (io())
    {
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
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001))
      free();
  }

  void onProcess(Graphics &g) override
  {
    a += 0.29;
    b += 0.23;
    timepose -= 0.06;
    int shape = getInternalParameterValue("table");
    g.polygonMode(wireframe ? GL_LINE : GL_FILL);
    // light.pos(0, 0, 0);
    gl::depthTesting(true);
    g.pushMatrix();
    g.depthTesting(true);
    g.lighting(true);
    g.translate(timepose, getInternalParameterValue("freq") / 200 - 3, -15);
    g.rotate(mVib() + a, Vec3f(0, 1, 0));
    g.rotate(mVib() * mVibDepth + b, Vec3f(1));
    float scaling = getInternalParameterValue("amplitude") * 10;
    g.scale(scaling + getInternalParameterValue("modMul") / 2, scaling + getInternalParameterValue("carMul") / 20, scaling + mEnvFollow.value() * 5);
    g.color(HSV(getInternalParameterValue("modMul") / 20, getInternalParameterValue("carMul") / 20, 0.5 + getInternalParameterValue("attackTime")));
    g.draw(mMesh[shape]);
    g.popMatrix();
  }

  void onTriggerOn() override
  {
    timepose = 10;
    mAmpEnv.reset();
    mVibEnv.reset();
    mModEnv.reset();
    mVib.phase(0);
    mod.phase(0);
    updateFromParameters();
    updateWaveform();

    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
  }
  void onTriggerOff() override
  {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
    mVibEnv.triggerRelease();
  }

  void updateFromParameters()
  {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.release(getInternalParameterValue("releaseTime"));
    mAmpEnv.sustain(getInternalParameterValue("sustain"));

    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[3] = getInternalParameterValue("releaseTime");

    mVibEnv.levels(getInternalParameterValue("vibRate1"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate2"),
                   getInternalParameterValue("vibRate1"));
    mVibEnv.lengths()[0] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[1] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[3] = getInternalParameterValue("vibRise");
    mVibDepth = getInternalParameterValue("vibDepth");
    
    mPan.pos(getInternalParameterValue("pan"));
  }
  void updateWaveform(){
        // Map table number to table in memory
    switch (int(getInternalParameterValue("table"))) {
      case 0:
        car.source(tbSaw);
        break;
      case 1:
        car.source(tbSqr);
        break;
      case 2:
        car.source(tbImp);
        break;
      case 3:
        car.source(tbSin);
        break;
      case 4:
        car.source(tbPls);
        break;
      case 5:
        car.source(tb__1);
        break;
      case 6:
        car.source(tb__2);
        break;
      case 7:
        car.source(tb__3);
        break;
      case 8:
        car.source(tb__4);
        break;
    }
  }


};

// 05_Tremolo
class OscTrm : public SynthVoice
{
public:
    // Unit generators
    gam::Pan<> mPan;
    gam::Sine<> mTrm;
    gam::Osc<> mOsc;
    gam::ADSR<> mTrmEnv;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow; // envelope follower to connect audio output to graphics

    // Additional members
    int mtable;
    static const int numb_waveform = 9;
    Mesh mMesh[numb_waveform];
    bool wireframe = false;
    bool vertexLight = false;
    double a_rotate = 0;
    double b_rotate = 0;
    double timepose = 0;
    // Initialize voice. This function will nly be called once per voice
    virtual void init()
    {
        // Intialize envelope
        mAmpEnv.curve(0);               // make segments lines
        mAmpEnv.levels(0, 0.3, 0.3, 0); // These tables are not normalized, so scale to 0.3
        mTrmEnv.curve(0);
        mTrmEnv.levels(0, 1, 1, 0);
        createInternalTriggerParameter("amplitude", 0.03, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 2.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.6, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("table", 0, 0, 8);
        createInternalTriggerParameter("trm1", 3.5, 0.2, 20);
        createInternalTriggerParameter("trm2", 5.8, 0.2, 20);
        createInternalTriggerParameter("trmRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("trmDepth", 0.1, 0.0, 1.0);

        // Table & Visual meshes
        // Now We have the mesh according to the waveform
        gam::addSinesPow<1>(tbSaw, 9, 1);
        addCone(mMesh[0], 1, Vec3f(0, 0, 5), 40, 1); // tbSaw

        gam::addSinesPow<1>(tbSqr, 9, 2);
        addCube(mMesh[1]); // tbSquare

        gam::addSinesPow<0>(tbImp, 9, 1);
        addPrism(mMesh[2], 1, 1, 1, 100); // tbImp

        gam::addSine(tbSin);
        addSphere(mMesh[3], 0.3, 16, 100); // tbSin

        // About: addSines (dst, amps, cycs, numh)
        // \param[out] dst		destination array
        // \param[in] amps		harmonic amplitudes of series, size must be numh - A[]
        // \param[in] cycs		harmonic numbers of series, size must be numh - C[]
        // \param[in] numh		total number of harmonics
        float scaler = 0.15;
        float hscaler = 1;

        { // tbPls
            float A[] = {1, 1, 1, 1, 0.7, 0.5, 0.3, 0.1};
            gam::addSines(tbPls, A, 8);
            addWireBox(mMesh[4], 2); // tbPls
        }
        { // tb__1
            float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08, 0, 0};
            float C[] = {1, 4, 7, 11, 15, 18, 0, 0};
            gam::addSines(tb__1, A, C, 6);
            for (int i = 0; i < 7; i++)
            {
                addWireBox(mMesh[5], scaler * A[i] * C[i], scaler * A[i + 1] * C[i + 1], 1 + 0.3 * i);

                // addSphere(mMesh[5],scaler * A[i], 16, 30); // tb__1
            }
        }
        { // inharmonic partials
            float A[] = {0.5, 0.8, 0.7, 1, 0.3, 0.4, 0.2, 0.12};
            float C[] = {3, 4, 7, 8, 11, 12, 15, 16};
            gam::addSines(tb__2, A, C, 8); // tb__2
            for (int i = 0; i < 7; i++)
            {
                addWireBox(mMesh[6], scaler * A[i] * C[i], scaler * A[i + 1] * C[i + 1], 1 + 0.3 * i);
            }
        }
        { // inharmonic partials
            float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08, 0, 0};
            float C[] = {10, 27, 54, 81, 108, 135, 0, 0};
            gam::addSines(tb__3, A, C, 6); // tb__3
            for (int i = 0; i < 7; i++)
            {
                addWireBox(mMesh[7], scaler * A[i] * C[i], scaler * A[i + 1] * C[i + 1], 1 + 0.3 * i);
            }
        }
        { // harmonics 20-27
            float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
            gam::addSines(tb__4, A, 8, 20); // tb__4
            for (int i = 0; i < 7; i++)
            {
                addWireBox(mMesh[8], hscaler * A[i], hscaler * A[i + 1], 1 + 0.3 * i);
            }
        }

        // Scale and generate normals
        for (int i = 0; i < numb_waveform; ++i)
        {
            mMesh[i].scale(0.4);

            int Nv = mMesh[i].vertices().size();
            for (int k = 0; k < Nv; ++k)
            {
                mMesh[i].color(HSV(float(k) / Nv, 0.3, 1));
            }

            if (!vertexLight && mMesh[i].primitive() == Mesh::TRIANGLES)
            {
                mMesh[i].decompress();
            }
            mMesh[i].generateNormals();
        }
    }

    //
    virtual void onProcess(AudioIOData &io) override
    {
        // updateFromParameters();
        float oscFreq = getInternalParameterValue("frequency");
        float amp = getInternalParameterValue("amplitude");
        float trmDepth = getInternalParameterValue("trmDepth");
        while (io())
        {

            mTrm.freq(mTrmEnv());
            // float trmAmp = mAmp - mTrm()*mTrmDepth; // Replaced with line below
            float trmAmp = (mTrm() * 0.5 + 0.5) * trmDepth + (1 - trmDepth); // Corrected
            float s1 = mOsc() * mAmpEnv() * trmAmp * amp;
            float s2;
            mEnvFollow(s1);
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        // We need to let the synth know that this voice is done
        // by calling the free(). This takes the voice out of the
        // rendering chain
        if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f))
            free();
    }

    virtual void onProcess(Graphics &g)
    {
        a_rotate += 0.81;
        b_rotate += 0.78;
        timepose -= 0.06;
        float frequency = getInternalParameterValue("frequency");
        int shape = getInternalParameterValue("table");

        // static Light light;
        g.polygonMode(wireframe ? GL_LINE : GL_FILL);
        // light.pos(0, 0, 0);
        gl::depthTesting(true);
        g.lighting(true);
        // g.light(light);
        g.pushMatrix();
        g.depthTesting(true);
        g.translate(timepose, getInternalParameterValue("frequency") / 200 - 3, -15);
        g.rotate(a_rotate, Vec3f(0, 1, 1));
        g.rotate(b_rotate, Vec3f(1));
        g.scale(0.2 + mAmpEnv() * 0.2 + 0.01 * mTrm(), 0.3 + mAmpEnv() * 0.5 + 0.01 * mTrm(), 0.1 + 0.01 * mTrm());
        g.scale(3 + mAmpEnv() * 0.5, 3 + mAmpEnv() * 0.5, 5 + mAmpEnv());
        g.color(HSV(frequency / 1000, 0.6 + mAmpEnv() * 0.1, 0.6 + 0.5 * mAmpEnv()));
        g.draw(mMesh[shape]);
        g.popMatrix();
    }

    virtual void onTriggerOn() override
    {
        // Rest all the envelopes from previous triggers
        mAmpEnv.reset();
        mTrmEnv.reset();
        mTrm.phase(0); 
        updateFromParameters();
        updateWaveform();
        timepose = 10;
    }

    virtual void onTriggerOff() override
    {
        mAmpEnv.triggerRelease();
        mTrmEnv.triggerRelease();
    }

    void updateFromParameters()
    {
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
    void updateWaveform()
    {
        // Map table number to table in memory
        switch (int(getInternalParameterValue("table")))
        {
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
};

// 06_AM
class OscAM : public SynthVoice
{
public:
  gam::Osc<> mAM;
  gam::ADSR<> mAMEnv;
  gam::Sine<> mOsc;
  gam::ADSR<> mAmpEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::Pan<> mPan;
  int mtable;
  Mesh mMesh;
  float a = 0.f; // current rotation angle
  bool wireframe = false;
  bool vertexLight = false;
  double a_rotate = 0;
  double b_rotate = 0;
  double timepose = 0;
  Vec3f spinner;
  // Initialize voice. This function will nly be called once per voice
  virtual void init()
  {
    addSphere(mMesh, 1, 100, 100);
    mMesh.decompress();
    mMesh.generateNormals();
    mAmpEnv.levels(0, 1, 1, 0);
    //    mAmpEnv.sustainPoint(1);

    mAMEnv.curve(0);
    mAMEnv.levels(0, 1, 1, 0);
    //    mAMEnv.sustainPoint(1);

    // We have the mesh be a sphere

    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 10, 4000.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 4, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.3, 0.1, 1.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("amFunc", 0.0, 0.0, 3.0);
    createInternalTriggerParameter("am1", 0.75, 0.0, 1.0);
    createInternalTriggerParameter("am2", 0.75, 0.0, 1.0);
    createInternalTriggerParameter("amRise", 0.75, 0.1, 1.0);
    createInternalTriggerParameter("amRatio", 0.75, 0.0, 2.0);
  }

  virtual void onProcess(AudioIOData &io) override
  {
    mOsc.freq(getInternalParameterValue("frequency"));

    float amp = getInternalParameterValue("amplitude");
    float amRatio = getInternalParameterValue("amRatio");
    while (io())
    {

      mAM.freq(mOsc.freq() * amRatio); // set AM freq according to ratio
      float amAmt = mAMEnv();          // AM amount envelope

      float s1 = mOsc();                            // non-modulated signal
      s1 = s1 * (1 - amAmt) + (s1 * mAM()) * amAmt; // mix modulated and non-modulated
      // s1 = (s1 * mAM()) * amAmt; // Ring modulation
      s1 *= mAmpEnv() * amp;

      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001))
      free();
  }

  virtual void onProcess(Graphics &g)
  {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    float pan = getInternalParameterValue("pan");
    int shape = getInternalParameterValue("table");
    float radius = frequency / 300;
    b_rotate += 1.1;
    timepose -= 0.04;
    // g.rotate(a_rotate, Vec3f(0, 1, 1));
    // g.rotate(b_rotate, Vec3f(1));

    gl::depthTesting(true);
    g.lighting(true);
    // g.light().dir(1.f, 1.f, 2.f);
    g.pushMatrix();
    g.translate(radius * sin(timepose) + 2, radius * cos(timepose), -15 + pan);

    // Rotate
    g.rotate(b_rotate, spinner);
    g.scale(0.05 * mAM() + 0.3);
    // center the model
    g.color(HSV(mOsc.freq() * getInternalParameterValue("amRatio") / 1000 + mAM() * 0.01, 0.5 + mAmpEnv() * 0.5, 0.05 + 5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();
  }

  virtual void onTriggerOn() override
  {
    mAmpEnv.attack(getInternalParameterValue("attackTime"));
    mAmpEnv.lengths()[1] = 0.001;
    mAmpEnv.release(getInternalParameterValue("releaseTime"));

    mAmpEnv.levels()[1] = getInternalParameterValue("sustain");
    mAmpEnv.levels()[2] = getInternalParameterValue("sustain");

    mAMEnv.levels(getInternalParameterValue("am1"),
                  getInternalParameterValue("am2"),
                  getInternalParameterValue("am2"),
                  getInternalParameterValue("am1"));

    mAMEnv.lengths(getInternalParameterValue("amRise"),
                   1 - getInternalParameterValue("amRise"));

    mPan.pos(getInternalParameterValue("pan"));

    mAmpEnv.reset();
    mAMEnv.reset();
    timepose = 0; // Initiate timeline
    b_rotate = al::rnd::uniform(0, 360);
    spinner = randomVec3f(1);
    // Map table number to table in memory
    switch (int(getInternalParameterValue("amFunc")))
    {
    case 0:
      mAM.source(tbSin);
      break;
    case 1:
      mAM.source(tbSqr);
      break;
    case 2:
      mAM.source(tbPls);
      break;
    case 3:
      mAM.source(tbDin);
      break;
    }
  }

  virtual void onTriggerOff() override
  {
    mAmpEnv.triggerRelease();
    mAMEnv.triggerRelease();
  }
};

// 07 Additive_synth
class AddSyn : public SynthVoice
{
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
  Mesh ball;
  double a = 0;
  double b = 0;
  double timepose = 0;
  Vec3f note_position;
  Vec3f note_direction;
  virtual void init()
  {

    // Intialize envelopes
    mEnvStri.curve(-4); // make segments lines
    mEnvStri.levels(0, 1, 1, 0);
    mEnvStri.lengths(0.1, 0.1, 0.1);
    mEnvStri.sustain(2); // Make point 2 sustain until a release is issued
    mEnvLow.curve(-4);   // make segments lines
    mEnvLow.levels(0, 1, 1, 0);
    mEnvLow.lengths(0.1, 0.1, 0.1);
    mEnvLow.sustain(2); // Make point 2 sustain until a release is issued
    mEnvUp.curve(-4);   // make segments lines
    mEnvUp.levels(0, 1, 1, 0);
    mEnvUp.lengths(0.1, 0.1, 0.1);
    mEnvUp.sustain(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addSphere(ball, 1, 100, 100);
    ball.decompress();
    ball.generateNormals();

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

  virtual void onProcess(AudioIOData &io) override
  {
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
    while (io())
    {
      float s1 = (mOsc1() + mOsc2() + mOsc3()) * mEnvStri() * ampStri;
      s1 += (mOsc4() + mOsc5()) * mEnvLow() * ampLow;
      s1 += (mOsc6() + mOsc7() + mOsc8() + mOsc9()) * mEnvUp() * ampUp;
      s1 *= amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // if(mEnvStri.done()) free();
    if (mEnvStri.done() && mEnvUp.done() && mEnvLow.done() && (mEnvFollow.value() < 0.001))
      free();
  }

  virtual void onProcess(Graphics &g)
  {
    a += 0.29;
    b += 0.23;
    timepose += 0.02;
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    // Now draw
    g.pushMatrix();
    g.depthTesting(true);
    g.lighting(true);
    g.translate(Vec3f(0, 0, -15));
    // g.translate(note_position + note_direction * timepose);
    g.rotate(a, Vec3f(0, 1, 0));
    g.rotate(b, Vec3f(1));
    g.scale(0.3 + mEnvStri() * 0.2, 0.3 + mEnvStri() * 0.5, 1);
    g.color(HSV(frequency / 1000, 0.5 + mEnvStri() * 0.1, 0.3 + 0.5 * mEnvStri()));
    g.draw(ball);
    g.popMatrix();
  }

  virtual void onTriggerOn() override
  {

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
    float angle = getInternalParameterValue("frequency") / 200;

    a = al::rnd::uniform();
    b = al::rnd::uniform();
    timepose = 0;
    note_direction = {sin(angle), cos(angle), 0};
  }

  virtual void onTriggerOff() override
  {
    //    std::cout << "trigger off" <<std::endl;
    mEnvStri.triggerRelease();
    mEnvLow.triggerRelease();
    mEnvUp.triggerRelease();
  }
};

// 08 Subtractive_synth
class Sub : public SynthVoice
{
public:
    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow; // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;
    double a = 0;
    double b = 0;
    double timepose = 0;
    Vec3f note_position;
    Vec3f note_direction;
    // Initialize voice. This function will nly be called once per voice
    void init() override
    {
        mAmpEnv.curve(0);               // linear segments
        mAmpEnv.levels(0, 1.0, 1.0, 0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2);        // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addSphere(mMesh, 1, 100, 100);
        mMesh.decompress();
        mMesh.generateNormals();

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.0, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 1, 0.0, 5.0);
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

    virtual void onProcess(AudioIOData &io) override
    {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while (io())
        {
            // mix oscillator with noise
            float s1 = mOsc() * (1 - noiseMix) + mNoise() * noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }

        if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f))
            free();
    }

    virtual void onProcess(Graphics &g)
    {
        a += 0.29;
        b += 0.23;
        timepose += 0.02;
        // Get the paramter values on every video frame, to apply changes to the
        // current instance
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");
        // Now draw
        g.pushMatrix();
        g.depthTesting(true);
        g.lighting(true);
        // g.translate(note_position);
        g.translate(note_position + note_direction * timepose);
        g.rotate(a, Vec3f(mCFEnv(), mBWEnv(), 0));
        g.rotate(b, Vec3f(mNoise()));
        g.scale(mCFEnv()/ 10000, mBWEnv()/ 10000,  0.3 + 0.1*mNoise());
        g.color(HSV(frequency / 1000, 0.5 + mOsc() * 0.1, 0.3 + 0.1*mNoise()));
        g.draw(mMesh);
        g.popMatrix();
    }
    virtual void onTriggerOn() override
    {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        a = al::rnd::uniform();
        b = al::rnd::uniform();
        timepose = 0;
        note_position = {0, 0, -15};
        float angle = getInternalParameterValue("frequency") / 200;
        note_direction = {sin(angle), cos(angle), 0};
    }

    virtual void onTriggerOff() override
    {
        mAmpEnv.triggerRelease();
        //        mCFEnv.triggerRelease();
        //        mBWEnv.triggerRelease();
    }

    void updateFromParameters()
    {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
        //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1] = getInternalParameterValue("sustain");
        mAmpEnv.levels()[2] = getInternalParameterValue("sustain");

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
        mBWEnv.lengths()[1] = 1 - getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

// 09 Plucked_string
class PluckedString : public SynthVoice
{
public:
    float mAmp;
    float mDur;
    float mPanRise;
    gam::Pan<> mPan;
    gam::NoiseWhite<> noise;
    gam::Decay<> env;
    gam::MovingAvg<> fil{2};
    gam::Delay<float, gam::ipl::Trunc> delay;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;
    gam::Env<2> mPanEnv;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);
    // This time, let's use spectrograms for each notes as the visual components.
    Mesh mSpectrogram;
    vector<float> spectrum;
    double a = 0;
    double b = 0;
    double timepose = 10;
    // Additional members
    Mesh mMesh;

    virtual void init() override
    {
        // Declare the size of the spectrum
        spectrum.resize(FFT_SIZE / 2 + 1);
        mSpectrogram.primitive(Mesh::POINTS);
        mAmpEnv.levels(0, 1, 1, 0);
        mPanEnv.curve(4);
        env.decay(0.1);
        delay.maxDelay(1. / 27.5);
        delay.delay(1. / 440.0);

        addDisc(mMesh, 1.0, 30);
        createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.001, 0.001, 1.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("Pan1", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("Pan2", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("PanRise", 0.0, 0, 3.0); // range check
    }

    //    void reset(){ env.reset(); }

    float operator()()
    {
        return (*this)(noise() * env());
    }
    float operator()(float in)
    {
        return delay(
            fil(delay() + in));
    }

    virtual void onProcess(AudioIOData &io) override
    {

        while (io())
        {
            mPan.pos(mPanEnv());
            float s1 = (*this)() * mAmpEnv() * mAmp;
            float s2;
            mEnvFollow(s1);
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
            // STFT for each notes
            if (stft(s1))
            { // Loop through all the frequency bins
                for (unsigned k = 0; k < stft.numBins(); ++k)
                {
                    // Here we simply scale the complex sample
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                }
            }
        }
        if (mAmpEnv.done() && (mEnvFollow.value() < 0.001))
            free();
    }

    virtual void onProcess(Graphics &g) override
    {
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");
        a += 0.29;
        b += 0.23;
        timepose -= 0.1;

        mSpectrogram.reset();
        // mSpectrogram.primitive(Mesh::LINE_STRIP);

        for (int i = 0; i < FFT_SIZE / 2; i++)
        {
            mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
            mSpectrogram.vertex(i, spectrum[i], 0.0);
        }
        g.meshColor(); // Use the color in the mesh
        g.pushMatrix();
        g.translate(0, 0, -15);
        g.rotate(a, Vec3f(0, 1, 0));
        g.rotate(b, Vec3f(1));
        g.scale(10.0 / FFT_SIZE, 500, 1.0);
        g.pointSize(1);
        g.draw(mSpectrogram);
        g.popMatrix();
    }

    virtual void onTriggerOn() override
    {
        mAmpEnv.reset();
        timepose = 10;
        updateFromParameters();
        env.reset();
        delay.zero();
        mPanEnv.reset();
    }

    virtual void onTriggerOff() override
    {
        mAmpEnv.triggerRelease();
    }

    void updateFromParameters()
    {
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
        mPanEnv.lengths()[0] = mPanRise;
        mPanEnv.lengths()[1] = mPanRise;
    }
};
