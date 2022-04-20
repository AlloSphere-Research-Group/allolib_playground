#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/io/al_Toml.hpp"

#include "al/graphics/al_Shapes.hpp"
#include "al/io/al_PersistentConfig.hpp"
#include "al/scene/al_DistributedScene.hpp"
#include "al/sound/al_Lbap.hpp"
#include "al/sound/al_SpeakerAdjustment.hpp"
#include "al/sphere/al_AlloSphereSpeakerLayout.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al/ui/al_FileSelector.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al_ext/soundfile/al_SoundfileBuffered.hpp"

#include "Gamma/scl.h"

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

    azimuth.setSynchronousCallbacks(false);
    elevation.setSynchronousCallbacks(false);
    distance.setSynchronousCallbacks(false);
    registerParameter(parameterPose());
    parameterPose().registerChangeCallback([&](auto val) {
      if (val.z() == 0) {
        azimuth = val.x() > 0 ? M_PI : -M_PI;
      } else {
        azimuth = gam::scl::wrap(atan(val.x() / -val.z()), M_PI, -M_PI);
      }
      std::cout << azimuth.get() << std::endl;
    });
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

    mSequencer.setSequencerStepTime(
        (float)static_cast<AudioObjectData *>(userData())->audioBlockSize /
        static_cast<AudioObjectData *>(userData())->audioSampleRate);

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
    gui.drawFunction = [&]() {
      if (ParameterGUI::drawAudioIO(audioIO())) {
        scene.prepare(audioIO());
        mObjectData.audioSampleRate = audioIO().framesPerSecond();
        mObjectData.audioBlockSize = audioIO().framesPerBuffer();
      }
    };
    registerDynamicScene(scene);

    // Parameter callbacks
    play.registerChangeCallback([&](auto v) {
      if (v == 1.0) {
        mSequencer.playSequence("session");
      }
    });

    if (al::sphere::isSimulatorMachine()) {
      auto sl = al::AlloSphereSpeakerLayout();
      scene.setSpatializer<Lbap>(sl);
    }

    audioIO().print();
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

  app.start();
  return 0;
}
