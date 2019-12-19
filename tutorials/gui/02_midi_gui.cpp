
#include "al/core.hpp"

#include "al/util/ui/al_ParameterGUI.hpp"
#include "al/util/ui/al_ParameterMIDI.hpp"

#include "al/core/math/al_Random.hpp"

using namespace al;

struct MyApp: public App {
  Parameter gain {"gain", "", 0.2f, "", 0.0, 1.0};
  ParameterMIDI parameterMIDI;

  rnd::Random<> random;

  void onCreate() override {
    ParameterGUI::initialize();
    parameterMIDI.connectControl(gain, 1, 1);
  }

  void onDraw(Graphics &g) override {
    g.clear(0.1);
    ParameterGUI::beginDraw();
    ParameterGUI::beginPanel("MIDI");
    ParameterGUI::draw(&gain);
    ParameterGUI::drawParameterMIDI(&parameterMIDI);
    ParameterGUI::endPanel();
    ParameterGUI::endDraw();
  }

  void onSound(AudioIOData &io) override {
    while (io()) {
       io.out(0) = random.uniform() * gain;
    }
  }

  void onExit() override {
    ParameterGUI::cleanup();
  }

};


int main(int argc, char *argv[])
{
  MyApp app;
  // You can use any of these methods

  // Use default device
  AudioDevice dev = AudioDevice::defaultOutput();
  // Use device 3
//  AudioDevice dev(3);
  // Use device called 'AudioFire 12'
//  AudioDevice dev("AudioFire 12");
  float sampleRate = 44100;
  int bufferSize = 256;
  int numInputChannels = 2;
  int numOutputChannels = 2;
  app.initAudio(dev, sampleRate, bufferSize, numOutputChannels, numInputChannels);
  // You can also initialize the default device using:
//  app.initAudio(AudioApp::IN_AND_OUT);
  // Or use a device with default settings:
//  app.initAudio(dev);


  app.start();

  return 0;
}
