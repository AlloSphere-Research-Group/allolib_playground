

#include "Gamma/Noise.h"
#include "al/app/al_App.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_ParameterGUI.hpp"

using namespace al;

// This example shows how you can build a GUI to select audio device and set
// audio parameters

struct MyApp : public App {
  Parameter gain{"gain", "", 0.2f};
  gam::NoisePink<> noise;

  void onCreate() override {
    imguiInit();
    // You can also set min and max in the Paramter constructor arguments
    gain.min(0.0);
    gain.max(1.0);
  }

  void onAnimate(double dt) override {
    imguiBeginFrame();
    ParameterGUI::beginPanel("Audio");
    ParameterGUI::drawParameterMeta(&gain);
    ParameterGUI::drawAudioIO(audioIO());
    ParameterGUI::endPanel();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override {
    g.clear(0.1);
    imguiDraw();
  }

  void onSound(AudioIOData &io) override {
    while (io()) {
      io.out(0) = noise() * gain;
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
