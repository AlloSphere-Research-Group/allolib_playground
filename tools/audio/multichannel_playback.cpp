#include "al/app/al_App.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/io/al_Toml.hpp"
#include "al/sound/al_DownMixer.hpp"
#include "al/sound/al_SpeakerAdjustment.hpp"
#include "al/sphere/al_AlloSphereSpeakerLayout.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al/ui/al_FileSelector.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al_ext/soundfile/al_SoundfileBuffered.hpp"

using namespace al;

struct MappedAudioFile {
  std::unique_ptr<SoundFileBuffered> soundfile;
  std::vector<size_t> outChannelMap;
  std::string fileInfoText;
  std::string fileName;
  float gain;
  bool mute{false};
};

class AudioPlayerApp : public App {
public:
  std::string rootDir{""};

  ParameterBool play{"play", "", 0.0};
  ParameterBool downmixStereo{"downmixStereo", "", 0.0};
  Trigger rewind{"rewind"};
  Trigger fw{"fw"};
  Trigger back{"back"};

  bool loadFile(std::string fileName, std::vector<size_t> channelMap,
                float gain, bool loop) {
    soundfiles.push_back(MappedAudioFile());
    soundfiles.back().soundfile = std::make_unique<SoundFileBuffered>(
        File::conformPathToOS(rootDir) + fileName, false, 4096);
    soundfiles.back().soundfile->loop(loop);
    if (!soundfiles.back().soundfile->opened()) {
      std::cerr << "ERROR: opening "
                << File::conformPathToOS(rootDir) + fileName << std::endl;
      return false;
    }
    if (soundfiles.back().soundfile->channels() != channelMap.size()) {
      std::cerr << "Channel mismatch for file " << fileName << ". File has "
                << soundfiles.back().soundfile->channels() << " but "
                << channelMap.size() << " provided. Aborting." << std::endl;
    }
    soundfiles.back().outChannelMap = channelMap;
    soundfiles.back().gain = gain;
    soundfiles.back().fileName = fileName;
    soundfiles.back().fileInfoText +=
        " channels: " +
        std::to_string(soundfiles.back().soundfile->channels()) +
        " sr: " + std::to_string(soundfiles.back().soundfile->frameRate()) +
        "\n";
    soundfiles.back().fileInfoText +=
        " length: " + std::to_string(soundfiles.back().soundfile->frames()) +
        "\n";
    soundfiles.back().fileInfoText +=
        " gain: " + std::to_string(soundfiles.back().gain) + "\n";
    return true;
  }

  // App callbacks
  void onInit() override {
    rewind.registerChangeCallback([&](float /*value*/) {
      play = 0.0;
      for (auto &sf : soundfiles) {
        sf.soundfile->seek(0);
      }
      play = 1.0;
    });
    fw.registerChangeCallback([&](float /*value*/) {
      play = 0.0;
      for (auto &sf : soundfiles) {
        sf.soundfile->seek(sf.soundfile->currentPosition() +
                           5 * sf.soundfile->frameRate());
      }
      play = 1.0;
    });
    back.registerChangeCallback([&](float /*value*/) {
      play = 0.0;
      for (auto &sf : soundfiles) {
        sf.soundfile->seek(sf.soundfile->currentPosition() -
                           5 * sf.soundfile->frameRate());
      }
      play = 1.0;
    });

    AudioDevice dev = AudioDevice::defaultOutput();
    if (sphere::isSphereMachine()) {
      dev = AudioDevice("ECHO X5");
      gainAdjustment.configure(AlloSphereSpeakerLayoutCompensated(), 1.82);
    }
    configureAudio(dev, soundfiles.back().soundfile->frameRate(), 1024,
                   dev.channelsOutMax(), 0);

    audioIO().append(gainAdjustment);

    int highestChannel = 0;
    for (const auto &sf : soundfiles) {
      for (const auto entry : sf.outChannelMap) {
        assert(entry <= INT32_MAX);
        if (highestChannel < static_cast<int32_t>(entry)) {
          highestChannel = static_cast<int32_t>(entry);
        }
      }
    }
    audioIO().channelsOut(highestChannel + 1);
    if (soundfiles.size() == 6) {
      // assume 5.1 to stereo
      mDownMixer.set5_1toStereo(audioIO());
      mDownMixer.setOutputs({0, 1});
    }
  }

  void onCreate() override { imguiInit(); }

  void onDraw(Graphics &g) override {
    imguiBeginFrame();

    ImGui::Begin("Multichannel Player");
    ParameterGUI::draw(&play);
    ParameterGUI::draw(&downmixStereo);
    ParameterGUI::draw(&rewind);

    ParameterGUI::draw(&back);
    ImGui::SameLine(0, 20);
    ParameterGUI::draw(&fw);

    ParameterGUI::drawParameterMeta(audioDomain()->parameters(),
                                    " (Global)##AudioIO");
    ParameterGUI::drawAudioIO(audioIO());
    if (soundfiles.size() > 0) {
      ImGui::Text("Time: %f", soundfiles[0].soundfile->currentPosition() /
                                  soundfiles[0].soundfile->frameRate());
    }
    ImGui::Separator();
    for (auto &sf : soundfiles) {
      ImGui::Text("*** %s", sf.fileName.c_str());
      ImGui::SameLine(0, 20);
      ImGui::PushID(sf.soundfile.get());
      ImGui::Checkbox("Mute", &sf.mute);
      ImGui::Text("%s", sf.fileInfoText.c_str());
      ImGui::PopID();
    }

    ImGui::End();
    imguiEndFrame();
    g.clear(0, 0, 0);
    imguiDraw();
  }

  void onSound(AudioIOData &io) override {
    float buffer[2048 * 60];
    if (play.get() == 1.0f) {
      for (auto &sf : soundfiles) {
        int numChannels = sf.soundfile->channels();
        int framesRead = sf.soundfile->read(buffer, io.framesPerBuffer());
        if (framesRead != io.framesPerBuffer()) {
          std::cout << "short buffer " << framesRead << std::endl;
        }
        for (size_t i = 0; i < sf.outChannelMap.size(); i++) {
          size_t outIndex = sf.outChannelMap[i];
          if (!sf.mute) {
            for (uint64_t sample = 0; sample < framesRead; sample++) {
              io.outBuffer(outIndex)[sample] +=
                  sf.gain * buffer[sample * numChannels + i];
            }
          }
        }
      }
      if (downmixStereo.get() == 1.0) {
        mDownMixer.downMix(io);
      }
    }
  }

  void onExit() override {
    for (auto &sf : soundfiles) {
      sf.soundfile->close();
    }
    imguiShutdown();
  }

private:
  std::vector<MappedAudioFile> soundfiles;
  SpeakerDistanceGainAdjustmentProcessor gainAdjustment;
  DownMixer mDownMixer;
};

int main(int argc, char *argv[]) {
  AudioPlayerApp app;

  /* Load configuration from text file. Config file should look like:

rootDir = "files/"
[[file]]
name = "test.wav"
outChannels = [0, 1]
gain = 0.9
[[file]]
name = "test_mono.wav"
outChannels = [1]
gain = 1.2
    */

  std::string configFile;
  if (argc > 1) {
    configFile = argv[1];
  } else {
    configFile = "multichannel_playback.toml";
  }

  TomlLoader appConfig(configFile);

  if (appConfig.hasKey<std::string>("rootDir")) {
    app.rootDir = appConfig.gets("rootDir");
  }
  if (appConfig.hasKey<double>("globalGain")) {
    assert(app.audioDomain()->parameters()[0]->getName() == "gain");
    app.audioDomain()->parameters()[0]->fromFloat(appConfig.getd("globalGain"));
  }
  auto nodesTable = appConfig.root->get_table_array("file");
  std::vector<std::string> filesToLoad;
  if (nodesTable) {
    for (const auto &table : *nodesTable) {
      std::string name = *table->get_as<std::string>("name");
      auto outChannelsToml = *table->get_array_of<int64_t>("outChannels");
      std::vector<size_t> outChannels;
      float gain = 1.0f;
      bool loop = false;
      if (table->contains("gain")) {
        gain = *table->get_as<double>("gain");
      }
      if (table->contains("loop")) {
        loop = *table->get_as<bool>("loop");
      }
      for (auto channel : outChannelsToml) {
        outChannels.push_back(channel);
      }
      // Load requested file into app. If any file fails, abort.
      if (!app.loadFile(name, outChannels, gain, loop)) {
        return -1;
      }
    }
  } else {
    std::cout << "Error loading file. Aborting" << std::endl;
    return -1;
  }

  app.start();
  return 0;
}
