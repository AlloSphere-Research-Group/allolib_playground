// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 08. Subtractive synthesis-Visual (Mesh & Spectrum)
// Press '[' or ']' to turn on & off GUI
// Able to play with MIDI device
// Myungin Lee

#include <cstdio> // for printing to stdout
#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"

using namespace al;
using namespace std;
#define FFT_SIZE 4048

class Sub : public SynthVoice
{
public:
    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow; // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;
    double a = 0;
    double b = 0;
    double timepose = 0;
    Vec3f note_position;
    Vec3f note_direction;
    // Initialize voice. This function will nly be called once per voice
    void init() override
    {
        mAmpEnv.curve(0);               // linear segments
        mAmpEnv.levels(0, 1.0, 1.0, 0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2);        // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addSphere(mMesh, 1, 100, 100);
        mMesh.decompress();
        mMesh.generateNormals();

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.0, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 1, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cf2", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("bw1", 700.0, 10.0, 5000);
        createInternalTriggerParameter("bw2", 900.0, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("hmnum", 12.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.0, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    }

    //

    virtual void onProcess(AudioIOData &io) override
    {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while (io())
        {
            // mix oscillator with noise
            float s1 = mOsc() * (1 - noiseMix) + mNoise() * noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }

        if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f))
            free();
    }

    virtual void onProcess(Graphics &g)
    {
        a += 0.29;
        b += 0.23;
        timepose += 0.02;
        // Get the paramter values on every video frame, to apply changes to the
        // current instance
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");
        // Now draw
        g.pushMatrix();
        g.depthTesting(true);
        g.lighting(true);
        // g.translate(note_position);
        g.translate(note_position + note_direction * timepose);
        g.rotate(a, Vec3f(mCFEnv(), mBWEnv(), 0));
        g.rotate(b, Vec3f(mNoise()));
        g.scale(mCFEnv()/ 10000, mBWEnv()/ 10000,  0.3 + 0.1*mNoise());
        g.color(HSV(frequency / 1000, 0.5 + mOsc() * 0.1, 0.3 + 0.1*mNoise()));
        g.draw(mMesh);
        g.popMatrix();
    }
    virtual void onTriggerOn() override
    {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        a = al::rnd::uniform();
        b = al::rnd::uniform();
        timepose = 0;
        note_position = {0, 0, -15};
        float angle = getInternalParameterValue("frequency") / 200;
        note_direction = {sin(angle), cos(angle), 0};
    }

    virtual void onTriggerOff() override
    {
        mAmpEnv.triggerRelease();
        //        mCFEnv.triggerRelease();
        //        mBWEnv.triggerRelease();
    }

    void updateFromParameters()
    {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
        //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1] = getInternalParameterValue("sustain");
        mAmpEnv.levels()[2] = getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));

        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1 - getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

class MyApp : public App, public MIDIMessageHandler
{
public:
    SynthGUIManager<Sub> synthManager{"synth8"};
    //    ParameterMIDI parameterMIDI;
    RtMidiIn midiIn; // MIDI input carrier
    Mesh mSpectrogram;
    vector<float> spectrum;
    bool showGUI = true;
    bool showSpectro = true;
    bool navi = false;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

    virtual void onInit() override
    {
        imguiInit();
        navControl().active(false); // Disable navigation via keyboard, since we
                                    // will be using keyboard for note triggering
        // Set sampling rate for Gamma objects from app's audio
        gam::sampleRate(audioIO().framesPerSecond());
        // Check for connected MIDI devices
        if (midiIn.getPortCount() > 0)
        {
            // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
            // callback called whenever a MIDI message is received
            MIDIMessageHandler::bindTo(midiIn);

            // Open the last device found
            unsigned int port = midiIn.getPortCount() - 1;
            midiIn.openPort(port);
            printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
        }
        else
        {
            printf("Error: No MIDI devices found.\n");
        }
        // Declare the size of the spectrum
        spectrum.resize(FFT_SIZE / 2 + 1);
    }

    void onCreate() override
    {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth8.synthSequence");
        synthManager.synthRecorder().verbose(true);
    }

    void onSound(AudioIOData &io) override
    {
        synthManager.render(io); // Render audio
        // STFT
        while (io())
        {
            if (stft(io.out(0)))
            { // Loop through all the frequency bins
                for (unsigned k = 0; k < stft.numBins(); ++k)
                {
                    // Here we simply scale the complex sample
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // spectrum[k] = stft.bin(k).real();
                }
            }
        }
    }

    void onAnimate(double dt) override
    {
        navControl().active(navi); // Disable navigation via keyboard, since we
        imguiBeginFrame();
        synthManager.drawSynthControlPanel();
        imguiEndFrame();
    }

    void onDraw(Graphics &g) override
    {
        g.clear();
        synthManager.render(g);
        // // Draw Spectrum
        mSpectrogram.reset();
        mSpectrogram.primitive(Mesh::LINE_STRIP);
        if (showSpectro)
        {
            for (int i = 0; i < FFT_SIZE / 2; i++)
            {
                mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
                mSpectrogram.vertex(i, spectrum[i], 0.0);
            }
            g.meshColor(); // Use the color in the mesh
            g.pushMatrix();
            g.translate(-3.0, -3, -15);
            g.scale(10.0 / FFT_SIZE, 100, 1.0);
            g.draw(mSpectrogram);
            g.popMatrix();
        }
        // GUI is drawn here
        if (showGUI)
        {
            imguiDraw();
        }
    }
    void onMIDIMessage(const MIDIMessage &m)
    {
        switch (m.type())
        {
        case MIDIByte::NOTE_ON:
        {
            int midiNote = m.noteNumber();
            if (midiNote > 0 && m.velocity() > 0.001)
            {
                synthManager.voice()->setInternalParameterValue(
                    "freq", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
                synthManager.voice()->setInternalParameterValue(
                    "attackTime", 0.01 / m.velocity());
                synthManager.triggerOn(midiNote);
            }
            else
            {
                synthManager.triggerOff(midiNote);
            }
            break;
        }
        case MIDIByte::NOTE_OFF:
        {
            int midiNote = m.noteNumber();
            printf("Note OFF %u, Vel %f", m.noteNumber(), m.velocity());
            synthManager.triggerOff(midiNote);
            break;
        }
        default:;
        }
    }
    bool onKeyDown(Keyboard const &k) override
    {
        if (ParameterGUI::usingKeyboard())
        { // Ignore keys if GUI is using them
            return true;
        }
        if (!navi)
        {
            if (k.shift())
            {
                // If shift pressed then keyboard sets preset
                int presetNumber = asciiToIndex(k.key());
                synthManager.recallPreset(presetNumber);
            }
            else
            {
                // Otherwise trigger note for polyphonic synth
                int midiNote = asciiToMIDI(k.key());
                if (midiNote > 0)
                {
                    synthManager.voice()->setInternalParameterValue(
                        "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
                    synthManager.triggerOn(midiNote);
                }
            }
        }
        switch (k.key())
        {
        case ']':
            showGUI = !showGUI;
            break;
        case '[':
            showSpectro = !showSpectro;
            break;
        case '=':
            navi = !navi;
            break;
        }
        return true;
    }

    bool onKeyUp(Keyboard const &k) override
    {
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0)
        {
            synthManager.triggerOff(midiNote);
        }
        return true;
    }

    void onExit() override { imguiShutdown(); }
};

int main()
{
    MyApp app;

    // Set up audio
    app.configureAudio(48000., 512, 2, 0);

    app.start();
}
