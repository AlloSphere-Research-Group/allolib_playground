
#include "al/app/al_DistributedApp.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_PersistentConfig.hpp"
#include "al/math/al_Random.hpp"
#include "al/scene/al_DistributedScene.hpp"

#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"

#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#include <Gamma/Noise.h>

using namespace al;

#include <iostream> // cout
#include <vector> // vector

const size_t numPictures = 8;

struct State {
  Pose pose; // for navigation
};

#ifdef AL_WINDOWS
// Damn you Windows!
#undef near
#undef far
#endif

class PictureAgent : public PositionedVoice {
public:
  ParameterString file{"file"};
  Parameter alpha{"alpha", "", 1.0, 0.0, 1.0};
  Texture tex;
  float aspectRatio{1.0f};

  ParameterBundle bundle{"pictureParams"};
  ParameterBool billboard{"billboard", "", false};
  std::string currentlyLoadedFile;

  virtual void init() {
    registerParameters(parameterPose(), parameterSize(), file, billboard,
                       alpha);

    parameterSize().min(0.1f);
    parameterSize().max(2.0f);

    file.registerChangeCallback([&](std::string value) {
      if (value != currentlyLoadedFile) {
        std::string *rootPath = static_cast<std::string *>(userData());

        std::string filename = *rootPath + "CFM_SensoriumImages_HiRes/" + value;

        auto imageData = Image(filename);

        if (imageData.array().size() == 0) {
          std::cout << "failed to load image " << filename << std::endl;
        }
        std::cout << "loaded image size: " << imageData.width() << ", "
                  << imageData.height() << std::endl;

        tex.create2D(imageData.width(), imageData.height());
        tex.submit(imageData.array().data(), GL_RGBA, GL_UNSIGNED_BYTE);

        tex.filter(Texture::LINEAR);
        aspectRatio = imageData.width() / (float)imageData.height();
        currentlyLoadedFile = value;
      }
    });
    file.setSynchronousCallbacks(
        false); // texture must always be loaded in graphics context

    bundle.addParameter(parameterPose());
    bundle.addParameter(parameterSize());
    bundle.addParameter(alpha);
    bundle.addParameter(file);
    bundle.addParameter(billboard);
  }
  virtual void update(double dt = 0) { (void)dt; }
  virtual void onProcess(Graphics &g) {
    //    g.tint(1.0, alpha);
    file.processChange();
    if (billboard.get() == 1) {
      Vec3f forward = pose().pos();
      forward.normalize();
      Quatf rot = Quatf::getBillboardRotation(-forward, Vec3f{0.0, 1.0f, 0.0f});
      g.rotate(rot);
    }
    g.tint(1.0, alpha);
    g.quad(tex, -0.5 * aspectRatio, -0.5, aspectRatio, 1, true);
  }

  virtual void onProcess(AudioIOData & /*io*/) {}
};

struct PictureViewer : DistributedAppWithState<State> {

  // a mesh we use to do graphics rendering in this app
  Mesh mesh;

  gam::NoisePink<> pinkNoise;

  ParameterColor bgColor{"bgColor", "", Color(0)};

  DistributedScene scene{TimeMasterMode::TIME_MASTER_CPU};
  FileList imageFiles;
  PersistentConfig config;
  PictureAgent pictures[numPictures];

  int8_t currentImage[numPictures]{0};
  PresetHandler presets;
  ControlGUI *gui;

  void onInit() override {

    if (!sphere::isSphereMachine()) {
      dataRoot = "c:/Users/Andres/Downloads/";
    }

    // Enable cuttlebone for state distribution
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    registerDynamicScene(scene);
    scene.registerSynthClass<PictureAgent>(); // Needed to propagate changes
    scene.verbose(true);
    // Picture voices
    for (size_t i = 0; i < numPictures; i++) {
      // DynamicScene allows injecting voices that are allocated outside,
      // but init() must be called for them explicitly
      pictures[i].init();
      pictures[i].id(i);
      scene.registerVoiceParameters(&pictures[i]);
      scene.triggerOn(&pictures[i], 0, i, &dataRoot);
      if (!isPrimary()) {
        pictures[i].markAsReplica();
      }
    }

    if (isPrimary()) {
      // Persistent configuration
      config.registerParameter(bgColor);
      config.read();

      // GUI
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();
      *gui << bgColor;
      *gui << presets;

      presets << bgColor;

      for (size_t i = 0; i < numPictures; i++) {
        *gui << pictures[i].bundle;
        presets.registerParameterBundle(pictures[i].bundle);
      }
    } else {
    }
    parameterServer() << bgColor;

    imageFiles = fileListFromDir(dataRoot + "CFM_SensoriumImages_HiRes");
  }

  //  void onCreate() override {}

  void onAnimate(double dt) override {

    scene.update(dt);
    if (isPrimary()) {

    } else {
    }
  }

  void onDraw(Graphics &g) override {
    g.pushMatrix();
    g.clear(bgColor);
    g.blending(true);
    g.blendTrans();
    scene.render(g);
    g.popMatrix();
  }

  void onSound(AudioIOData &io) override {

    if (isPrimary()) {
      while (io()) {
      }
    }
  }
  void onExit() override {
    if (isPrimary()) {
      config.write();
    }
  }

  bool onKeyDown(const Keyboard &k) override {
    if (isPrimary()) {
      auto currentPanel = gui->getBundleCurrent("pictureParams");
      if (k.key() == '[') {
        auto i = currentImage[currentPanel] - 1;
        if (i == -1) {
          i = imageFiles.count() - 1;
        }
        currentImage[currentPanel] = i;
        std::cout << "Loading " << (int)currentPanel << " -> "
                  << imageFiles[i].filepath() << std::endl;
        pictures[currentPanel].file.set(imageFiles[i].file());
      } else if (k.key() == ']') {

        auto i = currentImage[currentPanel] + 1;
        if (i == imageFiles.count()) {
          i = 0;
        }
        currentImage[currentPanel] = i;
        std::cout << "Loading " << (int)currentPanel << " -> "
                  << imageFiles[i].filepath() << std::endl;
        pictures[currentPanel].file.set(imageFiles[i].file());
      }
    } else { // Renderer
      if (k.key() == 'o') {
        omniRendering->drawOmni = !omniRendering->drawOmni;
      }
    }
    return false;
  }

  bool onMouseDown(const Mouse &m) override { return false; }
};

int main() {
  PictureViewer viewer;
  viewer.start();
  return 0;
}
