#include "al/app/al_App.hpp"
#include "al/scene/al_PolySynth.hpp"

#include "Gamma/Analysis.h"
#include "Gamma/Envelope.h"
#include "Gamma/Filter.h"
#include "Gamma/Noise.h"

using namespace al;

// Frequency coefficients from:
// http://iainmccurdy.org/CsoundRealtimeExamples/AdditiveSynthesis/ModalAddSyn.csd

std::vector<float> xylo = {1, 3.932, 9.538, 16.688, 24.566, 31.147};
std::vector<float> tubularBell = {
    272. / 437,  538. / 437,  874. / 437,  1281. / 437, 1755. / 437,
    2264. / 437, 2813. / 437, 3389. / 437, 4822. / 437, 5255. / 437};
std::vector<float> aluminiumBar = {1, 2.756, 5.423, 8.988, 13.448, 18.680};
std::vector<float> smallHandBell = {1,
                                    1.0019054878049,
                                    1.7936737804878,
                                    1.8009908536585,
                                    2.5201981707317,
                                    2.5224085365854,
                                    2.9907012195122,
                                    2.9940548780488,
                                    3.7855182926829,
                                    3.8061737804878,
                                    4.5689024390244,
                                    4.5754573170732,
                                    5.0296493902439,
                                    5.0455030487805,
                                    6.0759908536585,
                                    5.9094512195122,
                                    6.4124237804878,
                                    6.4430640243902,
                                    7.0826219512195,
                                    7.0923780487805,
                                    7.3188262195122,
                                    7.5551829268293};

struct ModalVoice : public SynthVoice {

  std::vector<gam::Reson<>> modes;
  std::vector<float> amps;
  float globalAmp = 10.;

  gam::NoisePink<> noise;

  gam::Env<3> residualEnv;
  gam::EnvFollow<> envFollow; // track envelope to know when to turn off note.

  float fundamentalFreq = 440;

  void init() override {
    residualEnv.levels(0.0f, 1.0f, 0.0f);
    residualEnv.lengths(0.002f, 0.07f);
  }

  void onProcess(AudioIOData &io) override {
    while (io()) {
      auto ampIt = amps.begin();
      float excitation = noise() * residualEnv();
      for (auto &mode : modes) {
        float out = mode(excitation);
        io.out(0) += *ampIt++ * globalAmp * out;
      }
      envFollow(io.out(0));
    }
    if (envFollow.done(0.0001)) {
      free();
    }
  }

  void onTriggerOn() override {
    residualEnv.reset();
    auto &freqs = smallHandBell;
    modes.resize(freqs.size());
    amps.resize(freqs.size());
    auto modesIt = modes.begin();
    auto ampsIt = amps.begin();
    int counter = 1;
    for (auto f : freqs) {
      modesIt->freq(f * fundamentalFreq);
      //      xylo 0.006
      // aluminium 0.00012
      // tubularBell 0.0004
      // small handbell 0.0004
      // small handbell 0.005 // low frequencies sounds like a pot
      modesIt->width(f * fundamentalFreq * 0.0004);
      *ampsIt = 1.0 / counter++;
      modesIt++;
      ampsIt++;
    }

    for (auto &mode : modes) {
      mode.zero();
    }
  }
};

struct MyApp : public App {

  PolySynth synth;

  void onInit() override { gam::sampleRate(audioIO().framesPerSecond()); }

  void onSound(AudioIOData &io) override { synth.render(io); }

  bool onKeyDown(const Keyboard &k) override {
    auto voice = synth.getVoice<ModalVoice>();
    synth.triggerOn(voice);
    return true;
  }
};

int main() {
  MyApp().start();

  return 0;
}
