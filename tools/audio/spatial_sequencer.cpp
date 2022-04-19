#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/io/al_Toml.hpp"

#include "al/graphics/al_Shapes.hpp"
#include "al/io/al_PersistentConfig.hpp"
#include "al/scene/al_DistributedScene.hpp"
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

struct AudioObjectData {
  std::string rootPath;
  uint16_t audioSampleRate;
  uint16_t audioBlockSize;
  Mesh *mesh;
};

class AudioObject : public PositionedVoice {
public:
  // Trigger Params
  ParameterString file{"audioFile", ""};            // in seconds
  ParameterString automation{"automationFile", ""}; // in seconds

  // Variable params
  Parameter gain{"gain", "", 1.0, 0.0, 4.0};
  ParameterBool mute{"mute", "", 0.0};

  // Internal
  Parameter azimuth{"azimuth", "", 0, -M_PI, M_PI};
  Parameter elevation{"elevation", "", 0, -M_PI_2, M_PI_2};
  Parameter distance{"distance", "", 1.0, 0.00001, 10};

  void init() override {
    registerTriggerParameters(file, automation, gain);
    registerParameters(azimuth, elevation, distance);
    mSequencer << azimuth << elevation << distance;

    mSequencer << mPresetHandler; // For morphing
    mPresetHandler << azimuth << elevation << distance;

    mSequencer.setSequencerStepTime(
        (float)static_cast<AudioObjectData *>(userData())->audioBlockSize /
        static_cast<AudioObjectData *>(userData())->audioSampleRate);

    azimuth.setSynchronousCallbacks(false);
    elevation.setSynchronousCallbacks(false);
    distance.setSynchronousCallbacks(false);
  }

  void onProcess(AudioIOData &io) override {
    { // update position from azimuth, elevation and distance parameters
      bool changed = false;
      if (azimuth.hasChange()) {
        azimuth.processChange();
        changed = true;
      }
      if (elevation.hasChange()) {
        elevation.processChange();
        changed = true;
      }
      if (distance.hasChange()) {
        distance.processChange();
        changed = true;
      }
      if (changed) {
        float d = distance.get();
        float az = azimuth.get();
        float el = elevation.get();
        float x, y, z;
        x = d * std::sin(az) * std::cos(el);
        y = d * std::sin(el);
        z = d * -std::cos(az) * std::cos(el);
        std::cout << x << "," << y << "," << z << std::endl;
        mPose.setPos({x, y, z});
      }
    }
    float buffer[2048 * 60];
    int numChannels = soundfile.channels();
    assert(io.framesPerBuffer() < INT32_MAX);
    auto framesRead =
        soundfile.read(buffer, static_cast<int>(io.framesPerBuffer()));
    int outIndex = 0;
    size_t inChannel = 0;
    if (!mute) {
      for (size_t sample = 0; sample < framesRead; sample++) {
        io.outBuffer(outIndex)[sample] +=
            gain * buffer[sample * numChannels + inChannel];
      }
    }
  }

  void onProcess(Graphics &g) override {
    auto &mesh = *static_cast<AudioObjectData *>(userData())->mesh;
    g.draw(mesh);
  }

  void onTriggerOn() override {

    auto &rootPath = static_cast<AudioObjectData *>(userData())->rootPath;
    soundfile.open(File::conformPathToOS(rootPath) + file.get());
    if (!soundfile.opened()) {
      std::cerr << "ERROR: opening audio file: "
                << File::conformPathToOS(rootPath) + file.get() << std::endl;
    }
    mSequencer.playSequence(File::conformPathToOS(rootPath) + automation.get());
  }

  void onFree() override { soundfile.close(); }

private:
  PresetSequencer mSequencer;
  PresetHandler mPresetHandler{""};
  SoundFileBuffered soundfile;
};

class SpatialSequencer : public DistributedApp {
public:
  std::string rootDir{""};

  DistributedScene scene{"spatial_sequencer"};

  Trigger play{"play", ""};
  PersistentConfig config;

  void setPath(std::string path) {
    rootDir = path;
    mSequencer.setDirectory(path);
  }

  void onInit() override {
    // Prepare scene shared data
    mObjectData.mesh = &this->mObjectMesh;
    mObjectData.rootPath = rootDir;
    mObjectData.audioSampleRate = audioIO().framesPerSecond();
    mObjectData.audioBlockSize = audioIO().framesPerBuffer();
    scene.setDefaultUserData(&mObjectData);

    mSequencer << scene;
    scene.registerSynthClass<AudioObject>(); // Allow AudioObject in sequences
    scene.allocatePolyphony<AudioObject>(16);

    // Prepare GUI
    auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = guiDomain->newGUI();
    gui << play << mSequencer << audioDomain()->parameters()[0];
    registerDynamicScene(scene);

    // Parameter callbacks
    play.registerChangeCallback([&](auto v) {
      if (v == 1.0) {
        mSequencer.playSequence("session");
      }
    });
  }

  void onCreate() override {
    // Prepare mesh
    addSphere(mObjectMesh);
    mObjectMesh.scale(0.1f);
    mObjectMesh.update();
  }
  void onAnimate(double dt) override { mSequencer.update(dt); }

  void onDraw(Graphics &g) override {
    g.clear(0, 0, 0);
    g.pushMatrix();
    g.translate(0, 0, -4);
    g.scale(3);
    mSequencer.render(g);
    g.popMatrix();
  }

  void onSound(AudioIOData &io) override { mSequencer.render(io); }

  void onExit() override {}

private:
  VAOMesh mObjectMesh;

  SynthSequencer mSequencer{TimeMasterMode::TIME_MASTER_CPU};
  AudioObjectData mObjectData;
  SpeakerDistanceGainAdjustmentProcessor gainAdjustment;
};

int main() {
  SpatialSequencer app;

  app.setPath("Morris Allosphere piece");
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

  //  TomlLoader appConfig("multichannel_playback.toml");
  //  //  appConfig.writeFile();

  //  if (appConfig.hasKey<std::string>("rootDir")) {
  //    app.rootDir = appConfig.gets("rootDir");
  //  }
  //  if (appConfig.hasKey<double>("globalGain")) {
  //    assert(app.audioDomain()->parameters()[0]->getName() == "gain");
  //    app.audioDomain()->parameters()[0]->fromFloat(appConfig.getd("globalGain"));
  //  }
  //  auto nodesTable = appConfig.root->get_table_array("file");
  //  std::vector<std::string> filesToLoad;
  //  if (nodesTable) {
  //    for (const auto &table : *nodesTable) {
  //      std::string name = *table->get_as<std::string>("name");
  //      auto outChannelsToml = *table->get_array_of<int64_t>("outChannels");
  //      std::vector<size_t> outChannels;
  //      float gain = 1.0f;
  //      bool loop = false;
  //      if (table->contains("gain")) {
  //        gain = *table->get_as<double>("gain");
  //      }
  //      if (table->contains("loop")) {
  //        loop = *table->get_as<bool>("loop");
  //      }
  //      for (auto channel : outChannelsToml) {
  //        outChannels.push_back(channel);
  //      }
  //      // Load requested file into app. If any file fails, abort.
  //      if (!app.loadFile(name, outChannels, gain, loop)) {
  //        return -1;
  //      }
  //    }
  //  }

  app.start();
  return 0;
}
