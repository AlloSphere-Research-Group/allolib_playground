
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"

#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetSequencer.hpp"

#include "al/scene/al_SynthSequencer.hpp"
#include "al/scene/al_DynamicScene.hpp"
#include "al/graphics/al_Imgui.hpp"

#include "al/sound/al_StereoPanner.hpp"
#include "al/sound/al_Vbap.hpp"
#include "al/sound/al_Dbap.hpp"
#include "al/sound/al_Ambisonics.hpp"

#include "Gamma/Oscillator.h"
#include "Gamma/Envelope.h"
#include "Gamma/Domain.h"

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
        addDodecahedron(mesh); // Prepare mesh to draw a dodecahedron

        mEnvelope.lengths(5.0f,  5.0f);
        mEnvelope.levels(0, 1, 0);
        mEnvelope.sustainPoint(1);
        mModulator.freq(1.9);
    }

    virtual void onProcess(AudioIOData &io) override {

        while(io()) {
            mModulatorValue = mModulator();
            io.out(0) += mEnvelope() * mSource() * mModulatorValue * 0.05; // compute sample
        }

        if (mEnvelope.done()) {
            free();
        }
    }

    virtual void onProcess(Graphics &g) {
        mLifeSpan--;
        if (mLifeSpan == 0) { // If it's time to die, start die off
            mEnvelope.release();
        }
        g.pushMatrix();
        g.polygonLine();
        g.color(0.1, 0.9, 0.3);
        g.scale(mSize * mEnvelope.value()+ mModulatorValue* 0.1);
        g.draw(mesh); // Draw the mesh
        g.popMatrix();
    }

    void set(float x, float y, float z, float size, float frequency, float lifeSpanFrames) {
        pose().pos(x, y, z);
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
//private:
    gam::Sine<> mSource; // Sine wave oscillator source
    gam::Saw<> mModulator; // Saw wave modulator
    gam::AD<> mEnvelope;

    unsigned int mLifeSpan; // life span counter
    float mModulatorValue; // To share modulator value from audio to graphics

    Mesh mesh; // The mesh now belongs to the voice
};


class MyApp : public App
{
public:

    virtual void onInit() override {
        // Configure spatializer for the scene
        SpeakerLayout sl = StereoSpeakerLayout();
        scene.setSpatializer<SpatializerType>(sl);

//        mOutputMaster.setMeterOn(true);
//        mOutputMaster.setMeterUpdateFreq(1);
    }

    virtual void onCreate() override {
        nav().pos(Vec3d(0,0,8)); // Set the camera to view the scene
        Light::globalAmbient({0.2, 1, 0.2});

        navControl().active(true); // Disable nav control (because we are using the control to drive the synth

        initIMGUI();
    }

//    virtual void onAnimate(double dt) override {
//        navControl().active(!gui.usingInput());
//    }

    virtual void onDraw(Graphics &g) override
    {
//        float values[2];
//        mOutputMaster.getCurrentValues(values);

        g.clear();
        scene.listenerPose(nav()); // Update listener pose to current nav
        scene.render(g);

        // Since we are not using parameters in this case we will use IMGUI
        // more directly to display information

        SynthVoice *voices = scene.getActiveVoices();
        int count = 0;
        while (voices) {
            count++;
            voices = voices->next;
        }
        beginIMGUI_minimal(true, "Info", 5, 5);
        ImGui::Text("Press space to create agent. Navigate scene with keyboard.");
        ImGui::Text("%i Active Agents", count);
        voices = scene.getActiveVoices();
        count = 0;
        while (voices) {
            MyAgent *agent = static_cast<MyAgent *>(voices);
            Pose &pose = agent->pose();
            ImGui::Text("-- Voice %i: id: %i freq: %f life: %i", count, voices->id(), agent->mSource.freq(), agent->mLifeSpan);
            ImGui::Text("         %f,%f,%f", pose.x(), pose.y(), pose.z());

            Vec3d direction = pose.vec() - scene.listenerPose().vec();

            //Rotate vector according to listener-rotation
            Quatd srcRot = scene.listenerPose().quat();
            direction = srcRot.rotate(direction);
//            direction = Vec4d(direction.x, direction.z, direction.y);

            ImGui::Text("         %f,%f,%f", direction.x, direction.y, direction.z);
            ImGui::Text("Attenuation: %i", agent->useDistanceAttenuation());

            count++;
            voices = voices->next;
        }
//        ImGui::SliderFloat2("Levels", values, 0.0, 0.03);
        endIMGUI_minimal(true);
    }

    virtual void onSound(AudioIOData &io) override {
         // The spatializer must be "prepared" and "finalized" on every block.
        // We do it here once, independently of the number of voices.

        scene.prepare(audioIO());
        scene.render(io);
//        mOutputMaster.onAudioCB(io);
    }

    virtual void onKeyDown(const Keyboard& k) override
    {
        if (k.key() == ' ') {
            // get a free voice from the scene
            MyAgent *voice = scene.getVoice<MyAgent>();

            Vec3d position = nav().pos();
            float size = randomGenerator.uniform(0.8, 1.2);;
            float frequency = randomGenerator.uniform(440.0, 880.0);
            int lifespan = this->fps() * randomGenerator.uniform(8.0, 20.0);
            voice->set(position.x, position.y, position.z - 1, // Place it in front
                       size, frequency,
                       lifespan);
            scene.triggerOn(voice);
        }
    }

private:
    Light light;

    rnd::Random<> randomGenerator; // Random number generator

    DynamicScene scene;
//    OutputMaster mOutputMaster {2, 44100};
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);

    app.initAudio(44100, 256, 2, 0);
    gam::sampleRate(app.audioIO().framesPerSecond());

    app.start();
    return 0;
}

