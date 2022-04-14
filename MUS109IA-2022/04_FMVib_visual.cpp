// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 04. FM Vib-Visual (Mesh & Spectrum)
// Press '[' or ']' to turn on & off GUI 
// Able to play with MIDI device
// Myungin Lee

#include <cstdio>  // for printing to stdout

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

class FM : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;
  gam::ADSR<> mVibEnv;

  gam::Sine<> car, mod, mVib;  // carrier, modulator sine oscillators
  double a = 0;
  double b = 0;
  double timepose = 10;
  Mesh ball;

  // Additional members
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
    addSphere(ball, 1, 100, 100);
    ball.decompress();
    ball.generateNormals();

    // We have the mesh be a sphere
    createInternalTriggerParameter("freq", 440, 10, 4000.0);
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
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
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
    a += 0.29;
    b += 0.23;
    timepose -= 0.06;
    g.pushMatrix();
    g.depthTesting(true);
    g.lighting(true);
    g.translate( timepose, getInternalParameterValue("freq") / 200 - 3 , -4);
    g.rotate(mVib() + a, Vec3f(0, 1, 0));
    g.rotate(mVibDepth + b, Vec3f(1));
    float scaling = getInternalParameterValue("amplitude") / 10 ;
    g.scale(scaling + getInternalParameterValue("modMul") / 10, scaling + getInternalParameterValue("carMul") / 30 , scaling + mEnvFollow.value() * 5);
    g.color(HSV(getInternalParameterValue("modMul") / 20, getInternalParameterValue("carMul") / 20, 0.5 + getInternalParameterValue("attackTime")));
    g.draw(ball);
    g.popMatrix();

  }

  void onTriggerOn() override {
    timepose = 10;
    updateFromParameters();
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
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

class MyApp : public App, public MIDIMessageHandler {
 public:
  SynthGUIManager<FM> synthManager{"synth4Vib"};
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
  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  void onInit() override {
    imguiInit();

    navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
    // Check for connected MIDI devices
    if (midiIn.getPortCount() > 0) {
      // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
      // callback called whenever a MIDI message is received
      MIDIMessageHandler::bindTo(midiIn);

      // Open the last device found
      unsigned int port = midiIn.getPortCount() - 1;
      midiIn.openPort(port);
      printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
    } else {
      printf("Error: No MIDI devices found.\n");
    }
    // Declare the size of the spectrum 
    spectrum.resize(FFT_SIZE / 2 + 1);
  }

  void onCreate() override {
    // Play example sequence. Comment this line to start from scratch
    //    synthManager.synthSequencer().playSequence("synth2.synthSequence");
    synthManager.synthRecorder().verbose(true);
    nav().pos(3, 0, 17);  
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
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
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
      g.scale(10.0 / FFT_SIZE, 100, 1.0);
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
            "attackTime", 0.1/m.velocity());
        synthManager.triggerOn(midiNote);
      }
      else
      {
        synthManager.triggerOff(midiNote);
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
            "freq", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
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
    case '-':
      tscale -= 0.1;
      break;
    case '+':
      tscale += 0.1;
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
};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}
