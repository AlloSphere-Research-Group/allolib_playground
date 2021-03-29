
#include "Gamma/Domain.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/math/al_Random.hpp"
#include "al/scene/al_DynamicScene.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/sound/al_Ambisonics.hpp"
#include "al/sound/al_Dbap.hpp"
#include "al/sound/al_StereoPanner.hpp"
#include "al/sound/al_Vbap.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetSequencer.hpp"

//#include "al/util/sound/al_OutputMaster.hpp"

using namespace al;

/*
 * This tutorial shows how to use the DynamicScene class. It is similar to a
 * PolySynth in that it handles the allocation, insertion and removal of
 * nodes in an audio-visual processing chain, and it supports spatial
 * information in the sources that informs both the graphics rendering and
 * the audio spatialization.
 */

// Choose the spatializer type here:

#define SpatializerType StereoPanner
//#define SpatializerType Vbap
//#define SpatializerType Dbap
//#define SpatializerType AmbisonicsSpatializer

//
class MyAgent : public PositionedVoice {
public:
  MyAgent() {
    mEnvelope.lengths(5.0f, 5.0f);
    mEnvelope.levels(0, 1, 0);
    mEnvelope.sustainPoint(1);
    mModulator.freq(1.9);
  }

  void onProcess(AudioIOData &io) override {
    while (io()) {
      mModulatorValue = mModulator();
      io.out(0) +=
          mEnvelope() * mSource() * mModulatorValue * 0.05; // compute sample
    }

    if (mEnvelope.done()) {
      free();
    }
  }

  void onProcess(Graphics &g) override {
    // Get shared Mesh
    Mesh *sharedMesh = static_cast<Mesh *>(userData());
    mLifeSpan--;
    if (mLifeSpan == 0) { // If it's time to die, start die off
      mEnvelope.release();
    }
    g.pushMatrix();
    gl::polygonMode(GL_LINE);
    g.color(0.1, 0.9, 0.3);
    g.scale(mSize * mEnvelope.value() + mModulatorValue * 0.1);
    g.draw(*sharedMesh); // Draw the mesh
    g.popMatrix();
  }

  void set(float x, float y, float z, float size, float frequency,
           float lifeSpanFrames) {
    setPose(Pose({x, y, z}));
    mSize = size;
    mSource.freq(frequency);
    mLifeSpan = lifeSpanFrames;
  }

  // No get or set ParamFields functions as this system is not made to
  // be recordable.

  virtual void onTriggerOn() override {
    // We want to reset the envelope:
    mEnvelope.reset();
    mModulator.phase(-0.1); // reset the phase
  }

  // No need for onTriggerOff() function as duration of agent's life is fixed

  // Make everything public so we can query it
  // private:
  gam::Sine<> mSource;   // Sine wave oscillator source
  gam::Saw<> mModulator; // Saw wave modulator
  gam::AD<> mEnvelope;

  unsigned int mLifeSpan; // life span counter
  float mModulatorValue;  // To share modulator value from audio to graphics
};

struct MyApp : public App {
  Mesh mesh;

  rnd::Random<> randomGenerator; // Random number generator

  DynamicScene scene;
  virtual void onInit() override {
    // Configure spatializer for the scene
    auto speakers = StereoSpeakerLayout();
    scene.setSpatializer<SpatializerType>(speakers);

    // You can set how distance attenuattion for audio is handled
    //    scene.distanceAttenuation().law(ATTEN_NONE);

    addDodecahedron(mesh); // Prepare mesh to draw a dodecahedron
    // Set pointer to mesh as default user data for the scene.
    // This pointer will be passed to all voices allocated from now on.
    // Voices can access this data through their userData() function.
    scene.setDefaultUserData(&mesh);

    // Prepare the scene buffers according to audioIO buffers
    scene.prepare(audioIO());
  }

  virtual void onCreate() override {
    navControl().active(true);
    imguiInit();
  }

  void onAnimate(double dt) override {
    SynthVoice *voices = scene.getActiveVoices();
    int count = 0;
    while (voices) {
      count++;
      voices = voices->next;
    }
    // We prepare the GUI in onAnimate()
    imguiBeginFrame();

    ImGui::SetNextWindowPos(ImVec2(5, 5));
    ImGui::Begin("Info");
    ImGui::Text("Press space to create agent. Navigate scene with keyboard.");
    ImGui::Text("%i Active Agents", count);
    voices = scene.getActiveVoices();
    count = 0;
    while (voices) {
      MyAgent *agent = static_cast<MyAgent *>(voices);
      Pose pose = agent->pose();
      ImGui::Text("-- Voice %i: id: %i freq: %f life: %i", count, voices->id(),
                  agent->mSource.freq(), agent->mLifeSpan);
      ImGui::Text("         %f,%f,%f", pose.x(), pose.y(), pose.z());

      Vec3d direction = pose.vec() - scene.listenerPose().vec();

      // Rotate vector according to listener-rotation
      Quatd srcRot = scene.listenerPose().quat();
      direction = srcRot.rotate(direction);
      //            direction = Vec4d(direction.x, direction.z, direction.y);

      ImGui::Text("         %f,%f,%f", direction.x, direction.y, direction.z);
      ImGui::Text("Attenuation: %i", agent->useDistanceAttenuation());

      count++;
      voices = voices->next;
    }
    //        ImGui::SliderFloat2("Levels", values, 0.0, 0.03);
    ImGui::End();
    imguiEndFrame();

    navControl().active(!isImguiUsingInput());
  }

  void onDraw(Graphics &g) override {
    g.clear();
    scene.listenerPose(nav()); // Update listener pose to current nav
    scene.render(g);

    imguiDraw();
  }

  virtual void onSound(AudioIOData &io) override {
    // The spatializer must be "prepared" and "finalized" on every block.
    // We do it here once, independently of the number of voices.
    scene.render(io);
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      // get a free voice from the scene
      MyAgent *voice = scene.getVoice<MyAgent>();

      Vec3d position = nav().pos();
      float size = randomGenerator.uniform(0.8, 1.2);
      float frequency = randomGenerator.uniform(440.0, 880.0);
      int lifespan =
          graphicsDomain()->fps() * randomGenerator.uniform(8.0, 20.0);
      voice->set(position.x, position.y, position.z - 1, // Place it in front
                 size, frequency, lifespan);
      scene.triggerOn(voice);
    }
    return true;
  }
};

int main() {
  MyApp app;
  app.dimensions(800, 600);

  app.configureAudio(44100, 256, 2, 0);
  gam::sampleRate(app.audioIO().framesPerSecond());

  app.start();
  return 0;
}
