#include <cstdio>  // for printing to stdout

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Font.hpp"

// using namespace gam;
using namespace al;

// This example shows how to use SynthVoice and SynthManagerto create an audio
// visual synthesizer. In a class that inherits from SynthVoice you will
// define the synth's voice parameters and the sound and graphic generation
// processes in the onProcess() functions.

// The helper function used to visualize which keys pressed or released on a virtual piano.
int asciiToKeyLabelIndex(int asciiKey) {
  switch (asciiKey) {
  case '2':
    return 30;
  case '3':
    return 31;
  case '5':
    return 33;
  case '6':
    return 34;
  case '7':
    return 35;
  case '9':
    return 37;
  case '0':
    return 38;

  case 'q':
    return 10;
  case 'w':
    return 11;
  case 'e':
    return 12;
  case 'r':
    return 13;
  case 't':
    return 14;
  case 'y':
    return 15;
  case 'u':
    return 16;
  case 'i':
    return 17;
  case 'o':
    return 18;
  case 'p':
    return 19;

  case 's':
    return 20;
  case 'd':
    return 21;
  case 'g':
    return 23;
  case 'h':
    return 24;
  case 'j':
    return 25;
  case 'l':
    return 27;
  case ';':
    return 28;

  case 'z':
    return 0;
  case 'x':
    return 1;
  case 'c':
    return 2;
  case 'v':
    return 3;
  case 'b':
    return 4;
  case 'n':
    return 5;
  case 'm':
    return 6;
  case ',':
    return 7;
  case '.':
    return 8;
  case '/':
    return 9;
  }
  return 0;
}

class SineEnv : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued

    addRect(mMesh, 0, 0, 1, 1);
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
    
    createInternalTriggerParameter("pianoKeyX");
    createInternalTriggerParameter("pianoKeyY");
    createInternalTriggerParameter("pianoKeyWidth");
    createInternalTriggerParameter("pianoKeyHeight");
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
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
    while (io()) {
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
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
  }

  // The graphics processing function
  void onProcess(Graphics& g) override {
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    float x = getInternalParameterValue("pianoKeyX");
    float y = getInternalParameterValue("pianoKeyY");

    float w = getInternalParameterValue("pianoKeyWidth");
    float h = getInternalParameterValue("pianoKeyHeight");

    float hue = (frequency - 200) / 1000;
    float sat = amplitude;
    float val = 0.9;
    
    g.pushMatrix();
    g.translate(x, y);
    g.scale(w, h);
    
    g.color(Color(HSV(hue, sat, val), mEnvFollow.value() * 30));
    
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App {
 public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<SineEnv> synthManager{"SineEnv_Piano"};
  
  // Mesh and variables for drawing piano keys
  Mesh meshKey;
  float keyWidth, keyHeight;
  float keyPadding = 2.f;
  float fontSize;
  std::string whitekeyLabels[20] = {"Z","X","C","V","B","N","M",",",".","/",
                                    "Q","W","E","R","T","Y","U","I","O","P"};
  std::string blackkeyLabels[20] = {"S","D","","G","H","J","","L",";","",
                                    "2","3","","5","6","7","","9","0",""};
  // Font renderder
  FontRenderer fontRender;

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override {
    navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Cacluate the size of piano keys based on the app window size
    float w = float(width());
    float h = float(height());
    keyWidth = w / 10.f - keyPadding * 2.f;
    keyHeight = h / 2.f - keyPadding * 2.f;
    fontSize = keyWidth * 0.2;

    // Create a mesh that will be drawn as piano keys
    addRect(meshKey, 0, 0, keyWidth, keyHeight);

    // Set the font renderer
    fontRender.load(Font::defaultFont().c_str(), 60, 1024);

    // Play example sequence. Comment this line to start from scratch
    synthManager.synthSequencer().playSequence("synth1.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData& io) override {
    synthManager.render(io);  // Render audio
  }

  void onAnimate(double dt) override {
    // The GUI is prepared here
    imguiBeginFrame();
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  // The graphics callback function.
  void onDraw(Graphics& g) override {
    g.clear();

    // This example uses only the orthogonal projection for 2D drawing
    g.camera(Viewpoint::ORTHO_FOR_2D);  // Ortho [0:width] x [0:height]

    // Drawing white piano keys
    for(int i = 0; i < 20; i++) {
      int index = i % 10;
      g.pushMatrix();
      
      float c = 0.9;
      float x = (keyWidth + keyPadding * 2) * index + keyPadding;
      float y = 0;

      if(i >= 10) {
        y = keyHeight + keyPadding * 2;
      }

      g.translate(x, y);
      g.color(c);
      g.tint(c);
      g.draw(meshKey);
      
      g.tint(1);
      fontRender.write(whitekeyLabels[i].c_str(), fontSize);
      fontRender.renderAt(g, {keyWidth * 0.5 - 5, keyHeight * 0.1, 0.0});

      g.popMatrix();
    }

    // Drawing balck piano keys
    for(int i = 0; i < 20; i++) {
      int index = i % 10;
      if(index == 2 || index == 6 || index == 9) continue;

      g.pushMatrix();
      
      float c = 0.5;      
      float x = (keyWidth + keyPadding * 2) * index + keyPadding + keyWidth * 0.5;
      float y = keyHeight * 0.5;

      if(i >= 10) {
        y = y + keyHeight + keyPadding * 2;
      }

      g.translate(x, y);
      g.scale(1, 0.5);
      g.color(c);
      g.tint(c);
      g.draw(meshKey);
      
      g.scale(1, 2);
      
      g.tint(1);
      fontRender.write(blackkeyLabels[i].c_str(), fontSize);
      fontRender.renderAt(g, {keyWidth * 0.5 - 5, keyHeight * 0.1, 0.0});
      
      g.popMatrix();
    }

    // Render the synth's graphics
    synthManager.render(g);

    // GUI is drawn here
    imguiDraw();
  }

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const& k) override {
    if (ParameterGUI::usingKeyboard()) {  // Ignore keys if GUI is using
                                          // keyboard
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
        // Check which key is pressed
        int keyIndex = asciiToKeyLabelIndex(k.key());
        
        bool isBlackKey = false;
        if(keyIndex >= 20) {
          keyIndex -= 20;
          isBlackKey = true;
        }

        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);

        float w = keyWidth;
        float h = keyHeight;
        float x = (keyWidth + keyPadding * 2) * (keyIndex % 10) + keyPadding;
        float y = 0;
        
        if(isBlackKey == true) {
          x += keyWidth * 0.5;
          y += keyHeight * 0.5;
          h *= 0.5;
        }
        
        if(keyIndex >= 10) {
          y += keyHeight + keyPadding * 2;
        }
        
        synthManager.voice()->setInternalParameterValue("pianoKeyWidth", w);
        synthManager.voice()->setInternalParameterValue("pianoKeyHeight", h);
        synthManager.voice()->setInternalParameterValue("pianoKeyX", x);
        synthManager.voice()->setInternalParameterValue("pianoKeyY", y);
        
        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const& k) override {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0) {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }
  
  // Whenever the window size changes this function is called
  void onResize(int w, int h) override {
    // Recaculate the size of piano keys based new window size
    keyWidth = w / 10.f - keyPadding * 2.f;
    keyHeight = h / 2.f - keyPadding * 2.f;
    fontSize = keyWidth * 0.2;
    addRect(meshKey, 0, 0, keyWidth, keyHeight);
  }

  void onExit() override { imguiShutdown(); }
};

int main() {
  // Create app instance
  MyApp app;
  
  // Set window size
  app.dimensions(1200, 600);
  
  // Set up audio
  app.configureAudio(48000., 512, 2, 0);
  app.start();
  return 0;
}
