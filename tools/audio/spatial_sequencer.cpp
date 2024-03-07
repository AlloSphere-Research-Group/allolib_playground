#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/io/al_PersistentConfig.hpp"
#include "al/io/al_Toml.hpp"
#include "al/math/al_Spherical.hpp"
#include "al/scene/al_DistributedScene.hpp"
#include "al/sound/al_DownMixer.hpp"
#include "al/sound/al_Lbap.hpp"
#include "al/sound/al_Speaker.hpp"
#include "al/sound/al_SpeakerAdjustment.hpp"
#include "al/sphere/al_AlloSphereSpeakerLayout.hpp"
#include "al/sphere/al_Meter.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al/ui/al_FileSelector.hpp"
#include "al/ui/al_ParameterGUI.hpp"

#include "al_ext/soundfile/al_SoundfileBuffered.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#include "Gamma/Analysis.h"
#include "Gamma/scl.h"

using namespace al;

struct SharedState {
  float meterValues[64] = {0};
};

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
  Parameter env{"env", "", 1.0, 0.00001, 10};

  void init() override {
    registerTriggerParameters(file, automation, gain);
    registerParameters(env);             // Propagate from audio rendering node
    registerParameters(parameterPose()); // Update position in secondary nodes

    mSequencer << parameterPose();
    mPresetHandler << parameterPose();
    mSequencer << mPresetHandler; // For morphing
  }

  void onProcess(AudioIOData &io) override {
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
        mEnvFollow(buffer[sample * numChannels + inChannel]);
      }
    }
  }

  void onProcess(Graphics &g) override {
    auto &mesh = *static_cast<AudioObjectData *>(userData())->mesh;
    if (isPrimary()) {
      env = mEnvFollow.value();
    }
    g.scale(0.5);
    g.scale(0.1 + gain + env * 10);
    g.color(c);
    g.polygonLine();
    g.draw(mesh);
  }

  void onTriggerOn() override {
    auto objData = static_cast<AudioObjectData *>(userData());

    if (isPrimary()) {
      auto &rootPath = objData->rootPath;
      soundfile.open(File::conformPathToOS(rootPath) + file.get());
      if (!soundfile.opened()) {
        std::cerr << "ERROR: opening audio file: "
                  << File::conformPathToOS(rootPath) + file.get() << std::endl;
      }

      float seqStep = (float)objData->audioBlockSize / objData->audioSampleRate;
      mSequencer.setSequencerStepTime(seqStep);

      mSequencer.playSequence(File::conformPathToOS(rootPath) +
                              automation.get());
    }
    auto colorIndex = automation.get()[0] - 'A';
    c = HSV(colorIndex / 6.0f, 1.0f, 1.0f);
  }

  void onTriggerOff() override {
    if (isPrimary()) {
      mPresetHandler.stopMorphing();
      mSequencer.stopSequence();
      soundfile.close();
    }
  }

  void onFree() override { soundfile.close(); }

private:
  PresetSequencer mSequencer;
  PresetHandler mPresetHandler{""};
  SoundFileBuffered soundfile{8192};
  Color c;

  gam::EnvFollow<> mEnvFollow;
};

class SpatialSequencer : public DistributedAppWithState<SharedState> {
public:
  std::string rootDir{""};

  DistributedScene scene{"spatial_sequencer", 0,
                         TimeMasterMode::TIME_MASTER_UPDATE};

  ParameterBool downMix{"downMix"};

  PersistentConfig config;
  DownMixer downMixer;

  void setPath(std::string path) {
    rootDir = al::File::conformDirectory(path);
    mSequencer.setDirectory(rootDir);
  }

  void onInit() override {
    // Prepare scene shared data
    mObjectData.mesh = &this->mObjectMesh;
    mObjectData.rootPath = rootDir;
    mObjectData.audioSampleRate = audioIO().framesPerSecond();
    mObjectData.audioBlockSize = audioIO().framesPerBuffer();
    scene.setDefaultUserData(&mObjectData);

    if (al::sphere::isSimulatorMachine()) {
    }
    auto sl = al::AlloSphereSpeakerLayoutCompensated();
    mSpatializer = scene.setSpatializer<Lbap>(sl);

    audioIO().channelsOut(60);
    audioIO().print();

    downMixer.layoutToStereo(sl, audioIO());
    downMixer.setStereoOutput();

    mSequencer << scene;

    registerDynamicScene(scene);
    scene.registerSynthClass<AudioObject>(); // Allow AudioObject in sequences
    scene.allocatePolyphony<AudioObject>(16);

    // Prepare GUI
    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto &gui = guiDomain->newGUI();
      gui << downMix << mSequencer << audioDomain()->parameters()[0];
      gui.drawFunction = [&]() {
        if (ParameterGUI::drawAudioIO(audioIO())) {
          scene.prepare(audioIO());
          mObjectData.audioSampleRate = audioIO().framesPerSecond();
          mObjectData.audioBlockSize = audioIO().framesPerBuffer();
        }
      };
    }
    CuttleboneStateSimulationDomain<SharedState>::enableCuttlebone(this);
  }

  void onCreate() override {
    // Prepare mesh
    addSphere(mSphereMesh, 0.1);
    mSphereMesh.update();
    addSphere(mObjectMesh, 0.1, 8, 4);
    mObjectMesh.update();
    mMeter.init(mSpatializer->speakerLayout());
  }

  void onAnimate(double dt) override {
    mSequencer.update(dt);
    if (isPrimary()) {
      auto &values = mMeter.getMeterValues();
      assert(values.size() < 65);
      memcpy(state().meterValues, values.data(), values.size() * sizeof(float));
    } else {
      mMeter.setMeterValues(state().meterValues, 64);
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0, 0, 0);
    g.pushMatrix();
    if (isPrimary()) {
      // For simulator view from outside
      g.translate(0.5, 0, -4);
    }
    {
      g.pushMatrix();
      g.polygonLine();
      g.scale(10);
      g.color(0.5);
      g.draw(mSphereMesh);
      g.popMatrix();
    }
    mMeter.draw(g);

    mSequencer.render(g);
    g.popMatrix();
  }

  void onSound(AudioIOData &io) override {
    mSequencer.render(io);
    mMeter.processSound(io);
    // downmix to stereo to bus 0 and 1
    downMixer.downMixToBus(io);
    // This can be used to create a global reverb
    while (io()) {
      float lfeLevel = 0.1;
      io.out(47) += io.bus(0) * lfeLevel;
      io.out(47) += io.bus(1) * lfeLevel;
    }
    if (downMix) {
      downMixer.copyBusToOuts(io);
    }
  }

  void onExit() override {}

private:
  VAOMesh mObjectMesh;
  VAOMesh mSphereMesh;

  SynthSequencer mSequencer{TimeMasterMode::TIME_MASTER_CPU};
  AudioObjectData mObjectData;
  SpeakerDistanceGainAdjustmentProcessor gainAdjustment;
  Meter mMeter;
  std::shared_ptr<Spatializer> mSpatializer;
};

int main(int argc, char *argv[]) {
  SpatialSequencer app;

  std::string folder;
  if (argc > 1) {
    folder = argv[1];
  } else {
    folder = "Morris Allosphere piece";
  }
  app.setPath(folder);

  app.start();
  return 0;
}
