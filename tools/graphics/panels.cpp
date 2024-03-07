
#include "al/app/al_DistributedApp.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_PersistentConfig.hpp"
#include "al/math/al_Random.hpp"
#include "al/scene/al_DistributedScene.hpp"

#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"

#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#ifdef AL_EXT_LIBAV
#include "al_ext/video/al_VideoDecoder.hpp"
#endif

#include <Gamma/Noise.h>

using namespace al;

#include <iostream> // cout
#include <vector> // vector

const size_t numPictures = 7;
const size_t numVideos = 1;

struct State {
  Pose pose; // for navigation
};

#ifdef AL_WINDOWS
// Damn you Windows!
#undef near
#undef far
#endif

static const char *imagePath = "Sensorium/images/";
static const char *videoPath = "Sensorium/videos/";

struct VoiceSharedData {
  std::string *dataRoot{nullptr};
};

class Panel : public PositionedVoice {
public:
  ParameterString file{"file"};
  Parameter alpha{"alpha", "", 1.0, 0.0, 1.0};
  Texture tex;
  float aspectRatio{1.0f};
  ParameterBool billboard{"billboard", "", true};
  std::string currentlyLoadedFile;

  Parameter lat{"lat", "", 0.0, -90.0, 90.0};
  Parameter lon{"lon", "", 0.0, -180.0, 180.0};
  Parameter radius{"radius", "", 10.0, 0.0, 50.0};

  ParameterBundle bundle{"pictureParams"};

  virtual void init() {

    registerParameters(parameterPose(), parameterSize(), file, billboard,
                       alpha);

    parameterSize().min(0.0f);
    parameterSize().max(10.0f);
    parameterSize().set(0.0f);

    file.setSynchronousCallbacks(
        false); // texture must always be loaded in graphics context

    lat.registerChangeCallback([&](float value) {
      parameterPose().setPos(Vec3d(radius.get() * cos(value / 180.0 * M_PI) *
                                       sin(lon.get() / 180.0 * M_PI),
                                   radius.get() * sin(value / 180.0 * M_PI),
                                   -radius.get() * cos(value / 180.0 * M_PI) *
                                       cos(lon.get() / 180.0 * M_PI)));
    });

    lon.registerChangeCallback([&](float value) {
      parameterPose().setPos(
          Vec3d(radius.get() * cos(lat.get() / 180.0 * M_PI) *
                    sin(value / 180.0 * M_PI),
                radius.get() * sin(lat.get() / 180.0 * M_PI),
                -radius.get() * cos(lat.get() / 180.0 * M_PI) *
                    cos(value / 180.0 * M_PI)));
    });

    radius.registerChangeCallback([&](float value) {
      parameterPose().setPos(Vec3d(value * cos(lat.get() / 180.0 * M_PI) *
                                       sin(lon.get() / 180.0 * M_PI),
                                   value * sin(lat.get() / 180.0 * M_PI),
                                   -value * cos(lat.get() / 180.0 * M_PI) *
                                       cos(lon.get() / 180.0 * M_PI)));
    });

    bundle.addParameter(parameterPose());
    bundle.addParameter(lon);
    bundle.addParameter(lat);
    bundle.addParameter(radius);
    bundle.addParameter(parameterSize());
    bundle.addParameter(alpha);
    bundle.addParameter(file);
    bundle.addParameter(billboard);
  }

  virtual void onProcess(Graphics &g) {
    file.processChange();
    g.pushMatrix();
    if (billboard.get() == 1) {
      Vec3f forward = pose().pos();
      forward.normalize();
      Quatf rot = Quatf::getBillboardRotation(-forward, Vec3f{0.0, 1.0f, 0.0f});
      g.rotate(rot);
    }
    g.tint(1.0, alpha);
    g.quad(tex, -0.5 * aspectRatio, 0.5, aspectRatio, -1, false);
    g.popMatrix();
  }
};

class PicturePanel : public Panel {
public:
  virtual void init() {
    Panel::init();

    file.registerChangeCallback([&](std::string value) {
      if (value != currentlyLoadedFile) {
        auto data = static_cast<VoiceSharedData *>(userData());
        std::string rootPath = *(data->dataRoot);

        std::string filename = rootPath + imagePath + value;

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
  }
};

class VideoPanel : public Panel {
public:
  //  double wallTime{0.0};
  double previousTime{0.0};

  ParameterBool playing{"playing", "", false};
  Parameter currentTime{"currentTime", "", 0.0};

  virtual void init() {
    Panel::init();

    bundle.addParameter(playing);
    bundle.addParameter(currentTime);
    registerParameter(playing);
    registerParameter(currentTime); // Current time will be updated on replicas
    currentTime.min(0.0f);

    file.registerChangeCallback([&](std::string value) {
      if (value != currentlyLoadedFile) {
#ifdef AL_EXT_LIBAV
        videoDecoder.stop();

        videoDecoder.init();
        auto data = static_cast<VoiceSharedData *>(userData());
        std::string rootPath = *(data->dataRoot);

        std::string filename = rootPath + videoPath + value;

        if (!videoDecoder.load(filename.c_str())) {
          std::cerr << "Error loading video file: " << filename << std::endl;
          //            quit();
        }
        // generate texture
        tex.filter(Texture::LINEAR);
        tex.wrap(Texture::REPEAT, Texture::CLAMP_TO_EDGE,
                 Texture::CLAMP_TO_EDGE);
        tex.create2D(videoDecoder.width(), videoDecoder.height(),
                     Texture::RGBA8, Texture::RGBA, Texture::UBYTE);
        videoDecoder.enableAudio(false);
        aspectRatio = videoDecoder.width() / (double)videoDecoder.height();
        currentTime = 0.0;
        currentTime.max(3000); // TODO set from video length
        playing = 1.0;
        videoDecoder.start();

#else
        std::cerr << "ERROR: video extension al_ext/video not built. Video "
                     "will not play back"
                  << std::endl;
#endif
      }
    });
  }

  void update(double dt) {
#ifdef AL_EXT_LIBAV
    bool timeChanged = false;
    if (isPrimary()) {
      if (playing.get() == 1.0f) {
        currentTime = currentTime.get() + dt;
        timeChanged = true;
      }
    }
    if (currentTime - previousTime < -3.0 / 30.0) {
      videoDecoder.stream_seek((int64_t)(currentTime * AV_TIME_BASE), -10);
      timeChanged = true;
    } else if (currentTime - previousTime > 3.0 / 30.0) {
      videoDecoder.stream_seek((int64_t)(currentTime * AV_TIME_BASE), 10);
      timeChanged = true;
    } else if (currentTime != previousTime) {
      timeChanged = true;
    }
    if (timeChanged) {
      uint8_t *frame = videoDecoder.getVideoFrame(currentTime);
      if (frame) {
        tex.submit(frame);
      }
      previousTime = currentTime;
    }
#endif
  }

  virtual void onProcess(Graphics &g) {
    file.processChange();
    g.pushMatrix();
    if (billboard.get() == 1) {
      Vec3f forward = pose().pos();
      forward.normalize();
      Quatf rot = Quatf::getBillboardRotation(-forward, Vec3f{0.f, 1.f, 0.f});
      g.rotate(rot);
    }
    g.tint(1.0, alpha);
    g.quad(tex, -0.5 * aspectRatio, 0.5, aspectRatio, -1, false);
    g.popMatrix();
  }

private:
#ifdef AL_EXT_LIBAV
  VideoDecoder videoDecoder;
#endif
};

class PanelViewer : public DistributedAppWithState<State> {
public:
  // a mesh we use to do graphics rendering in this app
  Mesh mesh;

  gam::NoisePink<> pinkNoise;

  ParameterColor bgColor{"bgColor", "", Color(0)};
  ParameterBool stereo{"stereo"};
  Parameter rotateSpeed{"rotateSpeed", 0.f, -5.f, 5.f};
  Parameter rotatePhase{"rotatePhase"};

  ParameterBool skybox{"skybox"};
  int8_t currentSkybox{0};
  VAOMesh sphereMesh;
  ParameterString skyboxFile{"skyboxFile"};
  ParameterPose skyboxPose{"skyboxPose"};
  Texture skyboxTexture;
  std::string currentSkyboxFile;

  DistributedScene scene{TimeMasterMode::TIME_MASTER_CPU};
  FileList imageFiles;
  FileList videoFiles;
  PersistentConfig config;
  PicturePanel pictures[numPictures];
  VideoPanel videos[numVideos];

  int8_t currentImage[numPictures + numVideos]{0};
  PresetHandler presets;
  ControlGUI *gui;

  VoiceSharedData voiceData;

  void onInit() override {
    voiceData.dataRoot = &this->dataRoot;
    assert(voiceData.dataRoot);

    // Enable cuttlebone for state distribution
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    registerDynamicScene(scene);
    scene.registerSynthClass<PicturePanel>(); // Needed to propagate changes
    scene.verbose(true);
    // Picture voices
    for (size_t i = 0; i < numPictures; i++) {
      // DynamicScene allows injecting voices that are allocated outside,
      // but init() must be called for them explicitly
      pictures[i].init();
      pictures[i].id(i);
      scene.registerVoiceParameters(&pictures[i]);
      scene.triggerOn(&pictures[i], 0, i, &voiceData);
      if (!isPrimary()) {
        pictures[i].markAsReplica();
      }
    }

    for (size_t i = 0; i < numVideos; i++) {
      videos[i].init();
      videos[i].id(i);
      scene.registerVoiceParameters(&videos[i]);
      scene.triggerOn(&videos[i], 0, 100 + i, &voiceData);
      if (!isPrimary()) {
        videos[i].markAsReplica();
      }
    }

    // Skybox
    {
      skyboxFile.setSynchronousCallbacks(false);

      skyboxFile.registerChangeCallback([&](std::string value) {
        if (value != currentSkyboxFile) {

          std::string filename = dataRoot + imagePath + value;

          auto imageData = Image(filename);

          if (imageData.array().size() == 0) {
            std::cout << "failed to load image " << filename << std::endl;
          }
          std::cout << "SKYBOX loaded image size: " << imageData.width() << ", "
                    << imageData.height() << std::endl;

          skyboxTexture.create2D(imageData.width(), imageData.height());
          skyboxTexture.submit(imageData.array().data(), GL_RGBA,
                               GL_UNSIGNED_BYTE);

          skyboxTexture.filter(Texture::LINEAR);
          currentSkyboxFile = value;
        }
      });
      parameterServer() << skyboxFile << skybox << skyboxPose << rotateSpeed
                        << rotatePhase;
    }

    if (isPrimary()) {
      // Persistent configuration
      config.registerParameter(bgColor);
      config.read();

      presets << bgColor << skyboxFile << skybox << skyboxPose << rotateSpeed;
    } else {
      lens().eyeSep(0);
      stereo.setSynchronousCallbacks(false);
      stereo.registerChangeCallback([&](auto value) {
        if (value == 1.0) {
          lens().eyeSep(0.02);
        } else {
          lens().eyeSep(0);
        }
        // if (omniRendering) {
        //   omniRendering->stereo(value == 1.0);
        // }
      });
    }
    parameterServer() << bgColor << stereo;

    imageFiles = fileListFromDir(dataRoot + imagePath);
    videoFiles = fileListFromDir(dataRoot + videoPath);
  }

  void onCreate() override {
    addSphereWithTexcoords(sphereMesh, 50, 50, true);
    sphereMesh.update();

    // GUI
    auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
    gui = &guiDomain->newGUI();
    *gui << bgColor;
    *gui << presets;

    *gui << skybox << skyboxFile << skyboxPose << rotateSpeed;
    *gui << stereo;

    for (size_t i = 0; i < numPictures; i++) {
      *gui << pictures[i].bundle;
      presets.registerParameterBundle(pictures[i].bundle);
    }
    for (size_t i = 0; i < numVideos; i++) {
      *gui << videos[i].bundle;
      presets.registerParameterBundle(videos[i].bundle);
    }
  }

  void onAnimate(double dt) override {
    skyboxFile.processChange();
    stereo.processChange();

    scene.update(dt);
    if (isPrimary()) {
      rotatePhase.set(rotatePhase.get() + rotateSpeed.get() * dt);
      if (rotatePhase.get() > 360.f)
        rotatePhase.set(rotatePhase.get() - 360.f);
      else if (rotatePhase.get() < -360.f)
        rotatePhase.set(rotatePhase.get() + 360.f);
    } else {
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(bgColor);
    g.blending(true);
    g.blendTrans();

    g.pushMatrix();
    g.rotate(rotatePhase, 0, 1, 0);

    if (skybox.get() == 1.0) {
      g.pushMatrix();
      g.texture();
      g.tint(1.f, 1.f);
      g.translate(skyboxPose.get().pos());
      g.rotate(skyboxPose.get().quat());
      skyboxTexture.bind();
      g.draw(sphereMesh);
      skyboxTexture.unbind();
      g.popMatrix();
    }

    g.popMatrix();

    g.pushMatrix();
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
        if (currentPanel < numPictures) {
          if (imageFiles.count() == 0) {
            return true;
          }
          auto i = currentImage[currentPanel] - 1;
          if (i == -1) {
            i = imageFiles.count() - 1;
          }
          currentImage[currentPanel] = i;
          std::cout << "Loading " << (int)currentPanel << " -> "
                    << imageFiles[i].filepath() << std::endl;
          pictures[currentPanel].file.set(imageFiles[i].file());
        } else {
          // Video Panel
          if (videoFiles.count() == 0) {
            return true;
          }
          auto i = currentImage[currentPanel] - 1;
          if (i == -1) {
            i = videoFiles.count() - 1;
          }
          currentImage[currentPanel] = i;
          std::cout << "Loading " << (int)currentPanel << " -> "
                    << videoFiles[i].filepath() << std::endl;
          videos[currentPanel - numPictures].file.set(videoFiles[i].file());
        }
      } else if (k.key() == ']') {

        if (currentPanel < numPictures) {
          if (imageFiles.count() == 0) {
            return true;
          }
          auto i = currentImage[currentPanel] + 1;
          if (i == imageFiles.count()) {
            i = 0;
          }
          currentImage[currentPanel] = i;
          std::cout << "Loading " << (int)currentPanel << " -> "
                    << imageFiles[i].filepath() << std::endl;
          pictures[currentPanel].file.set(imageFiles[i].file());
        } else {
          // Video Panel
          if (videoFiles.count() == 0) {
            return true;
          }
          auto i = currentImage[currentPanel] + 1;
          if (i == videoFiles.count()) {
            i = 0;
          }
          currentImage[currentPanel] = i;
          std::cout << "Loading " << (int)currentPanel << " -> "
                    << videoFiles[i].filepath() << std::endl;
          videos[currentPanel - numPictures].file.set(videoFiles[i].file());
        }
      } else if (k.key() == ',') {
        if (imageFiles.count() == 0) {
          return true;
        }
        auto i = currentSkybox - 1;
        if (i == -1) {
          i = imageFiles.count() - 1;
        }
        currentSkybox = i;
        std::cout << "Loading " << (int)currentPanel << " -> "
                  << imageFiles[i].filepath() << std::endl;
        skyboxFile.set(imageFiles[i].file());
      } else if (k.key() == '.') {

        if (imageFiles.count() == 0) {
          return true;
        }
        auto i = currentSkybox + 1;
        if (i == imageFiles.count()) {
          i = 0;
        }
        currentSkybox = i;
        std::cout << "Loading " << (int)currentPanel << " -> "
                  << imageFiles[i].filepath() << std::endl;
        skyboxFile.set(imageFiles[i].file());
      } else if (k.key() == 'r') {
        nav().home();
        rotateSpeed.set(0.f);
        rotatePhase.set(0.f);
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
  PanelViewer viewer;

  if (!sphere::isSphereMachine()) {
    viewer.dataRoot = "c:/Users/Andres/Downloads/";
    //      dataRoot = "/Users/cannedstar/code/allolib_playground/";
  } else {
    if (sphere::isRendererMachine()) {
      viewer.dataRoot += "/data/";
    } else {
      viewer.dataRoot = "/Volumes/Data/";
    }
  }
  viewer.start();
  return 0;
}
