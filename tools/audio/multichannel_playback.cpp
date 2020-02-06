#include "al/app/al_App.hpp"
#include "al/io/al_File.hpp"
#include "al_ext/soundfile/al_SoundfileBuffered.hpp"

using namespace al;

class AudioPlayerApp : public App {
 public:
  std::string
      rootDir;  // Set this variable to set root directory before loading files

  std::map<size_t, std::vector<size_t>> channelOutMap;

  bool loadFiles(std::vector<std::string> files) {
    for (auto file : files) {
      soundfiles.push_back(std::make_unique<SoundFileBuffered>(
          File::conformPathToOS(rootDir) + file));
      if (!soundfiles.back()->opened()) {
        std::cerr << "ERROR: opening " << File::conformPathToOS(rootDir) + file
                  << std::endl;
        return false;
      }
    }
    return true;
  }

  void onSound(AudioIOData &io) override {
    float buffer[512 * 60];
    for (auto outMap : channelOutMap) {
      int numChannels = soundfiles[outMap.first]->channels();
      int framesRead =
          soundfiles[outMap.first]->read(buffer, io.framesPerBuffer());
      for (size_t i = 0; i < outMap.second.size(); i++) {
        size_t outIndex = outMap.second[i];
        for (uint64_t sample = 0; sample < framesRead; sample++) {
          io.outBuffer(outIndex)[sample] += buffer[sample * numChannels + i];
        }
      }
    }
  }

  void onExit() {
    for (auto &sf : soundfiles) {
      sf->close();
    }
  }

 private:
  std::vector<std::unique_ptr<SoundFileBuffered>> soundfiles;
};

int main() {
  AudioPlayerApp app;
  app.rootDir = "";
  if (!app.loadFiles({"Allosphere_upper_test.wav"})) {
    return -1;
  }

  AudioDevice dev = AudioDevice::defaultOutput();
  if (dev.channelsOutMax() == 60) {
    app.channelOutMap = {
        {0, {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
             15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
             30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
             45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59}}};
  } else {
    std::cout << "Downmixing to two channels" << std::endl;
    std::vector<size_t> channelMap;
    for (int i = 0; i < 30; i++) {
      channelMap.push_back(0);
      channelMap.push_back(1);
    }
    app.channelOutMap = {{0, channelMap}};
  }

  app.configureAudio(dev, 48000, 256, 60);

  app.start();
  return 0;
}
