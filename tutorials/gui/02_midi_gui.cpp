

#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al/ui/al_ParameterMIDI.hpp"

using namespace al;

// This example shows the connection between a MIDI controller and the gain
// parameter in the App, together with a GUI to set up MIDI device

struct MyApp : public App {
  Parameter gain{"gain", "", 0.2f, 0.0, 1.0};
  ParameterMIDI parameterMIDI;

  rnd::Random<> random;

  void onCreate() override {
    imguiInit();
    parameterMIDI.connectControl(gain, 1, 1);
  }

  void onAnimate(double dt) override {
    imguiBeginFrame();
    ParameterGUI::beginPanel("MIDI");
    ParameterGUI::draw(&gain);
    ParameterGUI::drawParameterMIDI(&parameterMIDI);
    ParameterGUI::endPanel();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override {
    g.clear(0.1);
    imguiDraw();
  }

  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = random.uniform() * gain;
    }
  }

  void onExit() override { imguiShutdown(); }
};

int main(int argc, char *argv[]) {
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
  app.configureAudio(dev, sampleRate, bufferSize, numOutputChannels,
                     numInputChannels);
  // You can also initialize the default device using:
  //  app.initAudio(AudioApp::IN_AND_OUT);
  // Or use a device with default settings:
  //  app.initAudio(dev);

  app.start();

  return 0;
}
