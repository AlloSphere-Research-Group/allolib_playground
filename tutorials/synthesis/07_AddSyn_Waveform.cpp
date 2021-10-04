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

#define CHANNEL_COUNT 2
#define SAMPLE_RATE 48000.0f
#define BLOCK_SIZE 512

Parameter hue{"hue", "", 1, 0, 1};
Parameter sat{"sat", "", 1, 0, 1};
Parameter val{"val", "", 1, 0, 1};
ParameterMenu hueMapping{"hue mapping"};
ParameterMenu satMapping{"sat mapping"};
ParameterMenu valMapping{"val mapping"};
vector<string> mappingType{"amp", "freq", "ampStri", "ampLow", "ampUp"};

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
    
    if(hueMapping.getCurrent() == mappingType[0]) {
      hue = amp / 0.3;
    } else if(hueMapping.getCurrent() == mappingType[1]) {
      hue = freq / 5000.0;
    } else if(hueMapping.getCurrent() == mappingType[2]) {
      hue = ampStri;
    } else if(hueMapping.getCurrent() == mappingType[3]) {
      hue = ampLow;
    } else if(hueMapping.getCurrent() == mappingType[4]) {
      hue = ampUp;
    }

    if(satMapping.getCurrent() == mappingType[0]) {
      sat = amp / 0.3;
    } else if(satMapping.getCurrent() == mappingType[1]) {
      sat = freq / 5000.0;
    } else if(satMapping.getCurrent() == mappingType[2]) {
      sat = ampStri;
    } else if(satMapping.getCurrent() == mappingType[3]) {
      sat = ampLow;
    } else if(satMapping.getCurrent() == mappingType[4]) {
      sat = ampUp;
    }

    if(valMapping.getCurrent() == mappingType[0]) {
      val = amp / 0.3;
    } else if(valMapping.getCurrent() == mappingType[1]) {
      val = freq / 5000.0;
    } else if(valMapping.getCurrent() == mappingType[2]) {
      val = ampStri;
    } else if(valMapping.getCurrent() == mappingType[3]) {
      val = ampLow;
    } else if(valMapping.getCurrent() == mappingType[4]) {
      val = ampUp;
    }

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

    if(mEnvStri.done() && mEnvUp.done() && mEnvLow.done() && (mEnvFollow.value() < 0.001)) free();
  }

  virtual void onProcess(Graphics &g) {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

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
    mEnvStri.triggerRelease();
    mEnvLow.triggerRelease();
    mEnvUp.triggerRelease();
  }
};

class MyApp : public App, MIDIMessageHandler {
public:
  SynthGUIManager<AddSyn> synthManager {"AddSyn"};

  int midiNote;
  
  float waveformData[BLOCK_SIZE * CHANNEL_COUNT]{0};
  Mesh waveformMesh[2]{Mesh::LINE_STRIP, Mesh::LINE_STRIP};

  RtMidiIn midiIn;

  virtual void onInit( ) override {
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
    }
    else {
      printf("No MIDI devices found.\n");
    }
  }

  void onCreate() override {
    // Play example sequence. Comment this line to start from scratch
    //    synthManager.synthSequencer().playSequence("synth7.synthSequence");
    synthManager.synthRecorder().verbose(true);

    hueMapping.setElements(mappingType);
    satMapping.setElements(mappingType);
    valMapping.setElements(mappingType);
  }

  void onSound(AudioIOData& io) override {
    synthManager.render(io);  // Render audio

    memcpy(&waveformData, io.outBuffer(), BLOCK_SIZE * CHANNEL_COUNT * sizeof(float));
  }

  void onAnimate(double dt) override {
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();

    ParameterGUI::beginPanel("Visual Param");
    ParameterGUI::drawParameterMeta(&hue);
    ParameterGUI::drawMenu(&hueMapping);
    
    ParameterGUI::drawParameterMeta(&sat);
    ParameterGUI::drawMenu(&satMapping);

    ParameterGUI::drawParameterMeta(&val);
    ParameterGUI::drawMenu(&valMapping);

    ParameterGUI::endPanel();

    imguiEndFrame();

    float w = float(width());
    float h = float(height());
    float chRatio = 1 / float(CHANNEL_COUNT);
    float hSegment = chRatio * h;

    for(int ch = 0; ch < CHANNEL_COUNT; ch++) {
      waveformMesh[ch].reset();
      float yBase = (CHANNEL_COUNT - ch - 1) * hSegment;
      for(int i = 0; i < BLOCK_SIZE; i++) {
        float x = w * (i / float(BLOCK_SIZE));
        float y = hSegment * ((waveformData[ch * BLOCK_SIZE + i] + 1)/ 2.0) + yBase;
        
        waveformMesh[ch].vertex(x, y);
        waveformMesh[ch].color(HSV(hue, sat, val));
      }
    }
  }

  void onDraw(Graphics& g) override {
    g.clear();
    synthManager.render(g);
        
    g.camera(Viewpoint::ORTHO_FOR_2D);

    g.meshColor();
    for(int ch = 0; ch < CHANNEL_COUNT; ch++) { 
      g.draw(waveformMesh[ch]);
    }

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

  // This gets called whenever a MIDI message is received on the port
  void onMIDIMessage(const MIDIMessage& m) {
    printf("%s: ", MIDIByte::messageTypeString(m.status()));

    // Here we demonstrate how to parse common channel messages
    switch (m.type()) {
    case MIDIByte::NOTE_ON:
      synthManager.voice()->setInternalParameterValue(
          "frequency", ::pow(2.f, (m.noteNumber() - 69.f) / 12.f) * 432.f);
      synthManager.triggerOn(m.noteNumber());
      break;

    case MIDIByte::NOTE_OFF:
      if (m.noteNumber() > 0) {
        synthManager.triggerOff(m.noteNumber());
      }
      break;
    default:;
    }
  }
};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(SAMPLE_RATE, BLOCK_SIZE, CHANNEL_COUNT, 0);

  app.start();
}
