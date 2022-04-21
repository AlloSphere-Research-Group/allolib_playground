// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 04. FM Vib-Visual (Mesh & Spectrum)
// Press '[' or ']' to turn on & off GUI
// Able to play with MIDI device
// Myungin Lee

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

// using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

// tables for oscillator
gam::ArrayPow2<float> tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048),
    tbPls(2048), tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);

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
  static const int numb_waveform = 9;
  Mesh mMesh[numb_waveform];
  bool wireframe = false;
  bool vertexLight = false;

  void init() override
  {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);
    mModEnv.levels(0, 1, 1, 0);
    mVibEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
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
    g.translate(timepose, getInternalParameterValue("freq") / 200 - 3, -4);
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
    updateFromParameters();
    updateWaveform();

    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);

    mVibEnv.lengths()[0] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[1] = getInternalParameterValue("vibRise");
    mVibEnv.lengths()[3] = getInternalParameterValue("vibRise");
    mAmpEnv.reset();
    mVibEnv.reset();
    mModEnv.reset();
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

    mAmpEnv.levels()[1] = 1.0;
    mAmpEnv.levels()[2] = getInternalParameterValue("sustain");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[3] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[3] = getInternalParameterValue("releaseTime");

    mModEnv.lengths()[1] = mAmpEnv.lengths()[1];
    mVibEnv.levels()[1] = getInternalParameterValue("vibRate1");
    mVibEnv.levels()[2] = getInternalParameterValue("vibRate2");
    mVibDepth = getInternalParameterValue("vibDepth");
    mVibRise = getInternalParameterValue("vibRise");
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

class MyApp : public App, public MIDIMessageHandler
{
public:
  SynthGUIManager<FMWT> synthManager{"synth4VibWT"};
  RtMidiIn midiIn; // MIDI input carrier
  //    ParameterMIDI parameterMIDI;
  int midiNote;
  float mVibFrq;
  float mVibDepth;
  float tscale = 1;

  Mesh mSpectrogram;
  vector<float> spectrum;
  bool showGUI = true;
  bool showSpectro = true;
  bool navi = false;
  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  void onInit() override
  {
    imguiInit();

    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
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

  void onCreate() override
  {
    // Play example sequence. Comment this line to start from scratch
    //    synthManager.synthSequencer().playSequence("synth2.synthSequence");
    synthManager.synthRecorder().verbose(true);
    nav().pos(3, 0, 17);
  }

  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
    // STFT
    while (io())
    {
      if (stft(io.out(0)))
      { // Loop through all the frequency bins
        for (unsigned k = 0; k < stft.numBins(); ++k)
        {
          // Here we simply scale the complex sample
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
          // spectrum[k] = stft.bin(k).real();
        }
      }
    }
  }

  void onAnimate(double dt) override
  {
    navControl().active(navi); // Disable navigation via keyboard, since we
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override
  {
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
      g.scale(10.0 / FFT_SIZE, 1000, 1.0);
      g.draw(mSpectrogram);
      g.popMatrix();
    }
    // GUI is drawn here
    if (showGUI)
    {
      imguiDraw();
    }
  }
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
            "freq", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.voice()->setInternalParameterValue(
            "attackTime", 0.01 / m.velocity());
        synthManager.triggerOn(midiNote);
      }
      else
      {
        synthManager.triggerOff(midiNote);
      }
      break;
    }
    case MIDIByte::NOTE_OFF:
    {
      int midiNote = m.noteNumber();
      printf("Note OFF %u, Vel %f", m.noteNumber(), m.velocity());
      synthManager.triggerOff(midiNote);
      break;
    }
    default:;
    }
  }
  bool onKeyDown(Keyboard const &k) override
  {
    if (ParameterGUI::usingKeyboard())
    { // Ignore keys if GUI is using them
      return true;
    }
    if (!navi)
    {
      if (k.shift())
      {
        // If shift pressed then keyboard sets preset
        int presetNumber = asciiToIndex(k.key());
        synthManager.recallPreset(presetNumber);
      }
      else
      {
        // Otherwise trigger note for polyphonic synth
        int midiNote = asciiToMIDI(k.key()) - 24;
        if (midiNote > 0)
        {
          synthManager.voice()->setInternalParameterValue(
              "freq", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
          synthManager.triggerOn(midiNote);
        }
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
    case '-':
      tscale -= 0.1;
      break;
    case '+':
      tscale += 0.1;
      break;
    case '=':
      navi = !navi;
      break;
    }
    return true;
  }

  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key()) - 24;
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }
};

int main()
{
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}
