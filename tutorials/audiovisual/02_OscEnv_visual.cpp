// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 02. OscEnv (Mesh & Spectrum)
// This example shows how to form the waveform and visualize through the spectrum
// Press '[' or ']' to turn on & off GUI 
// Able to play with MIDI device
// Myungin Lee

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"
#include <cstdio>  

// using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

// tables for oscillator
gam::ArrayPow2<float> tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048),
    tbPls(2048), tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);

// This is the same SineEnv class defined in graphics/synth1.cpp
// It inclludes drawing code
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
        addPrism(mMesh[5], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1, 100, 1 + 0.3*i);
        addSphere(mMesh[5],scaler * A[i], 16, 30); // tb__1
      }
    }
    { // inharmonic partials
      float A[] = {0.5, 0.8, 0.7, 1, 0.3, 0.4, 0.2, 0.12};
      float C[] = {3, 4, 7, 8, 11, 12, 15, 16}; 
      gam::addSines(tb__2, A, C, 8); // tb__2
      // addSphere(mMesh[6],scaler * A[0], 16, 100);
      for (int i = 0; i < 7; i++){
        addPrism(mMesh[6], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1, 100, 1 + 0.4*i);
        addSphere(mMesh[6],scaler * A[i], 16, 30); // tb__1
      }
    }
    { // inharmonic partials
      float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08, 0 , 0};
      float C[] = {10, 27, 54, 81, 108, 135, 0, 0};
      gam::addSines(tb__3, A, C, 6); // tb__3
      for (int i = 0; i < 7; i++){
        addPrism(mMesh[7], scaler * A[i]*C[i], scaler * A[i+1]*C[i+1], 1, 100, 0.27*i);
        addSphere(mMesh[7],scaler * A[i], 16, 30); // tb__1
      }
    }
  { // harmonics 20-27
      float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
      gam::addSines(tb__4, A, 8, 20); // tb__4
      for (int i = 0; i < 7; i++){
        addPrism(mMesh[8], hscaler * A[i], hscaler * A[i+1], 1, 100, i);
        addSphere(mMesh[8],hscaler * A[i], 16, 30); // tb__1
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
    g.translate( timepose, getInternalParameterValue("frequency") / 200 - 3 , -4);
    g.rotate(a_rotate, Vec3f(0, 1, 1));
    g.rotate(b_rotate, Vec3f(1));    g.scale(0.3 + mAmpEnv() * 0.2, 0.3 + mAmpEnv() * 0.5, amplitude);
    g.scale(3 + mAmpEnv() * 0.5, 3 + mAmpEnv() * 0.5, 5 + mAmpEnv() );
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

// We make an app.
class MyApp : public App, public MIDIMessageHandler {
 public:
  OscEnv oscenv;
  RtMidiIn midiIn; // MIDI input carrier
  Mesh mSpectrogram;
  vector<float> spectrum;
  bool showGUI = true;
  bool showSpectro = true;
  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  virtual void onInit() override {
    // Check for connected MIDI devices
    if (midiIn.getPortCount() > 0)
    {
      // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
      // callback called whenever a MIDI message is received
      MIDIMessageHandler::bindTo(midiIn);

      // Open the last device found
      unsigned int port = midiIn.getPortCount() - 1;
      midiIn.openPort(port);
      printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
    }
    else
    {
      printf("Error: No MIDI devices found.\n");
    }
    // Declare the size of the spectrum 
    spectrum.resize(FFT_SIZE / 2 + 1);
  }

  void onCreate() override {
    imguiInit();
    nav().pos(2, 0, 17);  
    navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    // Play example sequence. Comment this line to start from scratch
    synthManager.synthSequencer().playSequence("synth2.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  void onSound(AudioIOData& io) override {
    synthManager.render(io);  // Render audio
    // STFT
    while (io())
    {
      if (stft(io.out(0)))
      { // Loop through all the frequency bins
        for (unsigned k = 0; k < stft.numBins(); ++k)
        {
          // Here we simply scale the complex sample
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3) );
          //spectrum[k] = stft.bin(k).real();
        }
      }
    }
  }

  void onAnimate(double dt) override {
    // Draw GUI
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
    // Map table number to table in memory
    oscenv.mtable = int(synthManager.voice()->getInternalParameterValue("table"));
  }

  void onDraw(Graphics& g) override {
    g.clear();
    synthManager.render(g);
    // // Draw Spectrum
    mSpectrogram.reset();
    mSpectrogram.primitive(Mesh::LINE_STRIP);
    if (showSpectro)
    {
      for (int i = 0; i < FFT_SIZE / 2; i++)
      {
        mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
        mSpectrogram.vertex(i, spectrum[i], 0.0);
      }
      g.meshColor(); // Use the color in the mesh
      g.pushMatrix();
      g.translate(-3, -3, 0);
      g.scale(20.0 / FFT_SIZE, 100, 1.0);
      g.draw(mSpectrogram);
      g.popMatrix();
    }
    // GUI is drawn here
    if (showGUI)
    {
      imguiDraw();
    }
  }
  // This gets called whenever a MIDI message is received on the port
  void onMIDIMessage(const MIDIMessage &m)
  {
    switch (m.type())
    {
    case MIDIByte::NOTE_ON:
    {
      int midiNote = m.noteNumber();
      if (midiNote > 0 && m.velocity() > 0.001)
      {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.voice()->setInternalParameterValue(
            "attackTime", m.velocity());
        synthManager.triggerOn(midiNote);
        printf("On Note %u, Vel %f", m.noteNumber(), m.velocity());
      }
      else
      {
        synthManager.triggerOff(midiNote);
        printf("Off Note %u, Vel %f", m.noteNumber(), m.velocity());
      }
      break;
    }
    }
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
        synthManager.voice()->setInternalParameterValue("table", oscenv.mtable);
        synthManager.triggerOn(midiNote);
      }
    }
    switch (k.key())
    {
    case ']':
      showGUI = !showGUI;
      break;
    case '[':
      showSpectro = !showSpectro;
      break;
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

  // GUI manager for OscEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<OscEnv> synthManager{"OscEnv"};
};

int main() {  // Create app instance
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);
  app.audioIO().print();

  app.start();
  return 0;
}
