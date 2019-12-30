
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"

#include "al/sound/al_StereoPanner.hpp"
#include "al/sound/al_Vbap.hpp"
#include "al/sound/al_Dbap.hpp"
#include "al/sound/al_Ambisonics.hpp"


#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetSequencer.hpp"

#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"

#include "Gamma/Oscillator.h"
#include "Gamma/Envelope.h"
#include "Gamma/Domain.h"

using namespace al;

/*
 * This tutorial shows how to use audio spatialization for SynthVoices and
 * PolySynth. It shows how each voice can have its own position in space
 * handled by a single spatializer object owned by the PolySynth.
*/

// Choose the spatializer type here:

//#define SpatializerType StereoPanner
//#define SpatializerType Vbap
//#define SpatializerType Dbap
#define SpatializerType AmbisonicsSpatializer

class MyVoice : public SynthVoice {
public:
    MyVoice() {
        addCone(mesh); // Prepare mesh to draw a cone
        mesh.primitive(LINE_STRIP); 

        mEnvelope.lengths(0.1f,  0.5f);
        mEnvelope.levels(0, 1, 0);
        mEnvelope.sustainPoint(1);
    }

    virtual void onProcess(AudioIOData &io) override {
        // First we will render the audio into bus 0
        // Note that we have allocated the bus on initialization by calling
        // channelsBus() for the AudioIO object in the main() function.
        // We could run the spatializer in sample by sample mode here and
        // avoid using the bus altogether but this will be significantly
        // slower, so for efficiency, we render the output first to a bus
        // and then we use the spatializer on that buffer.

        while(io()) {
            io.bus(0) = mEnvelope() * mSource() * 0.05; // compute sample
        }
        // Then we will pass the bus buffer to the spatializer's
        // renderBuffer function. The spatializer will map the signal to
        // the loudspeaker setup according to the position and the type
        // of spatializer. Note that all voices share a spatializer that is
        // owned by the PolySynth. The user data queried here can be set when
        // triggering the voice or by setting a default user data in the
        // PolySynth using setDefaultUserData()
        SpatializerType *spatializer = static_cast<SpatializerType *>(userData());
        spatializer->renderBuffer(io, mPose, io.busBuffer(0), io.framesPerBuffer());

        if (mEnvelope.done()) {
            free();
        }
    }

    virtual void onProcess(Graphics &g) {
        g.pushMatrix();
        g.translate(mPose.x(), mPose.y(), mPose.z());
        g.scale(mSize * mEnvelope.value());
        g.draw(mesh); // Draw the mesh
        g.popMatrix();
    }

    void set(float x, float y, float size, float frequency, float attackTime, float releaseTime) {
        mPose.pos(x, y, 0);
        mSize = size;
        mSource.freq(frequency);
        mEnvelope.lengths()[0] = attackTime;
        mEnvelope.lengths()[2] = releaseTime;
    }

    virtual bool setParamFields(float *pFields, int numFields) override {
        if (numFields != 6) { // Sanity check to make sure we are getting the right number of p-fields
            return false;
        }
        set(pFields[0], pFields[1], pFields[2], pFields[3], pFields[4], pFields[5]);
        return true;
    }

    virtual int getParamFields(float *pFields, int maxParams = -1) override {
        if (maxParams < 6) { return 0;} // Sanity check
        // For getParamFields, we will copy the internal parameters into the pointer recieved.
        pFields[0] = mPose.x();
        pFields[1] = mPose.y();
        pFields[2] = mSize;
        pFields[3] = mSource.freq();
        pFields[4] = mEnvelope.lengths()[0];
        pFields[5] = mEnvelope.lengths()[2];

        return 6;
    }

    virtual void onTriggerOn() override {
        // We want to reset the envelope:
        mEnvelope.reset();
    }

    virtual void onTriggerOff() override {
        // We want to force the envelope to release:

        mEnvelope.release();
    }


private:
    gam::Sine<> mSource; // Sine wave oscillator source
    gam::AD<> mEnvelope;

    Mesh mesh; // The mesh now belongs to the voice

    Pose mPose;
    float mSize {1.0}; // This are the internal parameters

};


class MyApp : public App
{
public:

    void onInit() override {
        // We must call compile() once to prepare the spatializer
        // This must be done in onInit() to make sure it is called before
        // audio starts procesing
        mSpatializer.setSpeakerLayout(StereoSpeakerLayout());
        mSpatializer.compile();

        // We need to pass the spatializer to each of the voices to spatialize
        // each one differently. We do this by setting the spatializer as
        // the default user data. This will get passed to voices before they are
        // triggered.
        mSequencer.synth().setDefaultUserData(&mSpatializer);
        
        // Before starting the application we need to register our voice in
        // the PolySynth (that is inside the sequencer). This allows
        // triggering the class from a text file.
        mSequencer.synth().registerSynthClass<MyVoice>("MyVoice");

        // We also need to pre-allocate some voices
        mSequencer.synth().allocatePolyphony<MyVoice>(10);
    }


    void onCreate() override {
        nav().pos(Vec3d(0,0,8)); // Set the camera to view the scene

        gui << X << Y << Size << AttackTime << ReleaseTime; // Register the parameters with the GUI

        gui << mRecorder;
        gui << mSequencer;

        gui.init(); // Initialize GUI. Don't forget this!

        navControl().active(false); // Disable nav control (because we are using the control to drive the synth
        mRecorder << mSequencer.synth();
    }


    void onDraw(Graphics &g) override
    {
        g.clear();

        // We call the render method for the sequencer. This renders its
        // internal PolySynth
        mSequencer.render(g);

        gui.draw(g);
    }

    void onSound(AudioIOData &io) override {
         // The spatializer must be "prepared" and "finalized" on every block.
        // We do it here once, independently of the number of voices.
        mSpatializer.prepare(io);
        mSequencer.render(io);
        mSpatializer.finalize(io);
    }

    bool onKeyDown(const Keyboard& k) override
    {
        MyVoice *voice = mSequencer.synth().getVoice<MyVoice>();
        int midiNote = asciiToMIDI(k.key());
        float freq = 440.0f * powf(2, (midiNote - 69)/12.0f);
        voice->set(X, Y, Size, freq, AttackTime, ReleaseTime);

        mSequencer.synth().triggerOn(voice, 0, midiNote);
        // We can pass the spatializer as the "user data" to the synth voice
        // This is not necessary as we have already set it as the default user
        // data. But you can pass per instance user data here if needed.
        // Note that for text sequences to work, you need to set a default
        // user data as you can't pass user data from the text file.
//        mSequencer.synth().triggerOn(voice, 0, midiNote, &mSpatializer);
        return  true;
    }

    bool onKeyUp(const Keyboard &k) override {
        int midiNote = asciiToMIDI(k.key());
        mSequencer.synth().triggerOff(midiNote);
        return  true;
    }

private:
    Light light;

    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};
    Parameter AttackTime {"AttackTime", "Sound", 0.1, "", 0.001f, 2.0f};
    Parameter ReleaseTime {"ReleaseTime", "Sound", 1.0, "", 0.001f, 5.0f};

    rnd::Random<> randomGenerator; // Random number generator

    ControlGUI gui;

    SynthRecorder mRecorder;
    SynthSequencer mSequencer;

    // A speaker layout and spatializer
    SpatializerType mSpatializer {};
};


int main(int argc, char *argv[])
{
    MyApp app;

    // We will render each voice's output to an internal bus within the
    // AudioIO object. We need to allocate this bus here, before audio
    // is opened by initAudio.
    app.audioIO().channelsBus(1);
    gam::sampleRate(app.audioIO().framesPerSecond());

    app.start();
    return 0;
}

