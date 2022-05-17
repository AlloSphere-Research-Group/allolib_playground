// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 06. FM Vib-Visual (Object & Spectrum)
// Press '[' or ']' to turn on & off GUI
// Able to play with MIDI device
// ***MacOS may require manual installation of assets3D. 
// brew install assimp
// https://github.com/assimp/assimp/blob/master/Build.md
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
#include "al/math/al_Random.hpp"

#include "al_ext/assets3d/al_Asset.hpp"
#include <algorithm> 
#include <cstdint>   
#include <vector>

using namespace gam;
using namespace al;
using namespace std;

// tables for oscillator
gam::ArrayPow2<float>
    tbSin(2048), tbSqr(2048), tbPls(2048), tbDin(2048);
#define FFT_SIZE 4048
Vec3f randomVec3f(float scale)
{
  return Vec3f(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS()) * scale;
}

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

  Scene *ascene{nullptr};
  Vec3f scene_min, scene_max, scene_center;
  Texture tex;
  vector<Mesh> mMesh;
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
    // Import .obj file for the mesh
    // std::string fileName = "../obj/ducky.obj";
    std::string fileName = "../obj/flower01.obj";

    ascene = Scene::import(fileName);
    if (!ascene)
    {
      printf("error reading %s\n", fileName.c_str());
      return;
    }
    ascene->getBounds(scene_min, scene_max);
    scene_center = (scene_min + scene_max) / 2.f;
    // ascene->print();
    mMesh.resize(ascene->meshes());
    for (int i = 0; i < ascene->meshes(); i += 1)
    {
      ascene->mesh(i, mMesh[i]);
    }

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
    g.translate(radius * sin(timepose) + 2, radius * cos(timepose), -25 + pan);

    // Rotate
    g.rotate(b_rotate, spinner);

    // g.scale(1 + mAM() * 0.02, 1 + mAM() * 0.02, 1 + mAM() * 0.02);
    float tmp = scene_max[0] - scene_min[0];
    tmp = std::max(scene_max[1] - scene_min[1], tmp);
    tmp = std::max(scene_max[2] - scene_min[2], tmp);
    tmp = 2.f / tmp;
    g.scale(tmp);
    // center the model
    g.color(HSV(mOsc.freq() * getInternalParameterValue("amRatio") / 1000 + mAM() * 0.01, 0.5 + mAmpEnv() * 0.5, 0.05 + 5 * mAmpEnv()));

    //    tex.bind(0);
    //    g.texture(); // use texture to color the mesh
    // draw all the meshes in the scene

    for (auto &m : mMesh)
    {
      g.draw(m);
    }

    //    tex.unbind(0);

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

class MyApp : public App, public MIDIMessageHandler
{
public:
  SynthGUIManager<OscAM> synthManager{"synth6"};
  //    ParameterMIDI parameterMIDI;
  int midiNote;
  OscAM oscam;
  RtMidiIn midiIn; // MIDI input carrier
  Mesh mSpectrogram;
  vector<float> spectrum;
  bool showGUI = true;
  bool showSpectro = true;
  bool navi = false;
  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  virtual void onInit() override
  {
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
    imguiInit();
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    gam::addWave(tbSin, SINE);
    gam::addWave(tbSqr, SQUARE);
    gam::addWave(tbPls, IMPULSE, 4);

    // inharmonic partials
    {
      float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
      float C[] = {10, 27, 54, 81, 108, 135};
      addSines(tbDin, A, C, 6);
    }
  }

  void onCreate() override
  {
    navControl().active(false);
    // Play example sequence. Comment this line to start from scratch
    //    synthManager.synthSequencer().playSequence("synth2.synthSequence");
    synthManager.synthRecorder().verbose(true);
    nav().pos(2, 0, 0);
  }

  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
    while (io())
    {
      io.out(0) = log(io.out(0) + 1) / 3;
      io.out(1) = log(io.out(1) + 1) / 3;
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
    // Draw GUI
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
    // Map table number to table in memory
    oscam.mtable = int(synthManager.voice()->getInternalParameterValue("table"));
  }

  void onDraw(Graphics &g) override
  {
    g.clear(0);
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
      g.translate(-3, -3, -17);
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
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0)
        {
          synthManager.voice()->setInternalParameterValue(
              "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
          synthManager.voice()->setInternalParameterValue("table", oscam.mtable);
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
    case '=':
      navi = !navi;
      break;
    }
    return true;
  }

  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
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
