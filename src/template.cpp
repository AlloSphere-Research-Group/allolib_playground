#include "al/app/al_App.hpp"
#include "Gamma/Oscillator.h"

using namespace al;

struct MyApp: public App {
    float color = 0.0;
    gam::Sine<> oscillator;

    void onInit() override { // Called on app start
        std::cout << "onInit()" << std::endl;
    }

    void onCreate() override { // Called when graphics context is available
        std::cout << "onCreate()" << std::endl;
    }

    void onAnimate(double dt) override { // Called once before drawing
        color += 0.01;
        if (color > 1.0) {
            color -= 1.0;
        }
        oscillator.freq(220 + 880 * color);
    } 

    void onDraw(Graphics &g) override { // Draw function
        g.clear(color);
    }

    void onSound(AudioIOData &io) override { // Audio callback
        while (io()) {
            io.out(0) = oscillator() * 0.1;
        }
    }

    void onMessage(osc::Message &m) override { // OSC message callback
        m.print();
    }
};


int main()
{
    MyApp app;
    auto dev = AudioDevice("devicename");
    app.configureAudio(dev, 44100, 256, 2, 2);

    app.start();
    return 0;
}
