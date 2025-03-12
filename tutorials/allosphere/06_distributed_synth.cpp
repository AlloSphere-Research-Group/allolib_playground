/*
Distributed Synth:

This app demonstrates how to create a distributed synthesizer using the
DistributedApp, DistributedScene, SynthGUIManager, and PositionedVoice classes.

Joel A. Jaffe
2025-03-11

*/

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/app/al_DistributedApp.hpp"
#include "al/scene/al_DistributedScene.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"

using namespace al;
using namespace std;

/**
 * @brief Class for managing the GUI of a distributed synth
 */
template <class VoiceType>
class DistributedSynthGUIManager : public SynthGUIManager<VoiceType>
{
public:
  DistributedSynthGUIManager(std::string name = "") : SynthGUIManager<VoiceType>(name) {}
  ~DistributedSynthGUIManager() {}

  void initScene(DistributedApp &app) 
  {
    this->mScene.template registerSynthClass<VoiceType>();
    this->synthSequencer().registerSynth(mScene);
    app.registerDynamicScene(this->mScene);
  }

private:
  DistributedScene mScene{TimeMasterMode::TIME_MASTER_CPU};
};

/**
 * @brief A SineEnv class similar to the one in `tutorial/audiovisual/01_SineEnv_visual.cpp`,
 * but implemented as a PositionedVoice.
 */
class SineEnv : public PositionedVoice
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
  double a;
  double b;
  double spin = al::rnd::uniformS();
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
    a += spin;
    b += spin;
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
    note_position = {0, 0, 0};
    note_direction = {sin(angle), cos(angle), 0};
  }

  void onTriggerOff() override { mAmpEnv.release(); }
};

/**
 * @brief The our main app, 
 * based on the one in `tutorial/audiovisual/01_SineEnv_visual.cpp`
 */
class DistributedSynthApp : public DistributedApp, public MIDIMessageHandler
{
public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  DistributedSynthGUIManager<SineEnv> synthManager{"SineEnv"};
  RtMidiIn midiIn; // MIDI input carrier
  ParameterMIDI parameterMIDI;
  bool showGUI = true;
  bool navi = false;

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override
  {
    synthManager.initScene(*this); // init scene
    navControl().active(false); // Disable navigation via keyboard

    if (isPrimary()) 
    {
      nav().pos(0, 0, 13);
      // Set sampling rate for Gamma objects from app's audio
      gam::sampleRate(audioIO().framesPerSecond());

      imguiInit();

      // Play example sequence. Comment this line to start from scratch
      synthManager.synthSequencer().playSequence("synth1.synthSequence");
      
      // Map MIDI controls to parameters here
      parameterMIDI.connectControl(synthManager.voice()->getInternalParameter("amplitude"), 1, 1);
      parameterMIDI.connectControl(synthManager.voice()->getInternalParameter("attackTime"), 2, 1);
      parameterMIDI.connectControl(synthManager.voice()->getInternalParameter("releaseTime"), 3, 1);
      parameterMIDI.connectControl(synthManager.voice()->getInternalParameter("pan"), 4, 1);      
    }    
    else 
    {
      nav().pos(0, 0, 1);
    }                  

  }

  void onInit()
  {
    if (isPrimary())
    {
      // Check for connected MIDI devices
      if (midiIn.getPortCount() > 0)
      {
        // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
        // callback called whenever a MIDI message is received
        MIDIMessageHandler::bindTo(midiIn);

        // Open the last device found
        unsigned int port = 0; //midiIn.getPortCount() - 1;
        midiIn.openPort(port);
        printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
      
        parameterMIDI.open(port, true);
      }
      else
      {
        printf("Error: No MIDI devices found.\n");
      }
    }
  }
  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData &io) override
  {
    if (isPrimary())
    {
      synthManager.render(io); // Render audio
    }
  }

  void onAnimate(double dt) override
  {
    if (isPrimary())
    {
      // The GUI is prepared here
      imguiBeginFrame();
      // Draw a window that contains the synth control panel
      synthManager.drawSynthControlPanel();
      ParameterGUI::drawParameterMIDI(&parameterMIDI);
      imguiEndFrame();
      navControl().active(navi);
    }
  }

  // The graphics callback function.
  void onDraw(Graphics &g) override
  {
    // Black background
    g.clear(); 

    // Render the synth's graphics
    synthManager.render(g);

    // Render GUI on Primary
    if (isPrimary())
    {
      if (showGUI)
      {
        imguiDraw();
      }
    }
  }

  // This gets called whenever a MIDI message is received on the port
  void onMIDIMessage(const MIDIMessage &m)
  {
    if (isPrimary())
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
              "attackTime", 0.1/m.velocity());
          synthManager.triggerOn(midiNote);
          printf("On Note %u, Vel %f \n", m.noteNumber(), m.velocity());
        }
        else
        {
          synthManager.triggerOff(midiNote);
          printf("Off Note %u, Vel %f \n", m.noteNumber(), m.velocity());
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
  }
  
  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const &k) override
  {
    if (isPrimary())
    {
      if (ParameterGUI::usingKeyboard())
      { // Ignore keys if GUI is using
        // keyboard
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
            synthManager.voice()->setInternalParameterValue("frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
            synthManager.triggerOn(midiNote);
          }
        }
      }
      switch (k.key())
      {
      case ']':
        showGUI = !showGUI;
        break;
      case '=':
        navi = !navi;
        break;
      }
      return true;
    }
  }

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const &k) override
  {
    if (isPrimary()) 
    {
      int midiNote = asciiToMIDI(k.key());
      if (midiNote > 0)
      {
        synthManager.triggerOff(midiNote);
      }
      return true;
    }
  }

  void onExit() override 
  { 
    if (isPrimary()) 
    {
      imguiShutdown(); 
    }
  }
};

int main()
{
  // Create app instance
  DistributedSynthApp mDistributedSynthApp;

  // Set up audio... only do this on primary?
  mDistributedSynthApp.configureAudio(48000., 512, 2, 0);
  mDistributedSynthApp.start();
  return 0;
}