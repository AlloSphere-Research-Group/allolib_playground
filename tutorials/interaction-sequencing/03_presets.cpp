
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_PresetHandler.hpp"
#include "al/ui/al_PresetServer.hpp"
#include "al/math/al_Random.hpp"
#include "al/ui/al_ControlGUI.hpp"

using namespace al;

/* A simple App that draws a cone on the screen
 * Parameters are exposed via OSC.
*/

class MyApp : public App
{
public:

    void onCreate() override {
        nav().pos(Vec3d(0,0,8)); // Set the camera to view the scene
        addCone(mesh); // Prepare mesh to draw a cone
        mesh.primitive(Mesh::LINE_STRIP);

        gui << X << Y << Size; // Register the parameters with the GUI
        gui << presetHandler; // Register the preset handler with GUI
        gui.init(); // Initialize GUI. Don't forget this!

        /*
            To register Parameters with a PresetHandler, you use the streaming
            operator, just as you did for the ParameterServer.
        */
        presetHandler << X << Y << Size;
        presetHandler.setMorphTime(2.0); // Presets will take 2 seconds to "morph"

        /*
            You need to register the PresetHandler object into the PresetServer
            in order to expose it via OSC.

            By default, a PresetServer will listen on OSC path "/preset", although this
            can be changed calling the setAddress() function. The OSC message must
            contain a float or an int value providing the preset index.

            You can also change the morph time by sending a float value to OSC address
            "/preset/morphTime".
        */
        presetServer << presetHandler;
        /*
            Adding a listener to a preset server makes the listener receive notification
            of any preset or morph time changes.
        */
        presetServer.addListener("127.0.0.1", 9050);

        /*
            The PresetServer print() function gives details about the PresetServer
            configuration. It will print something like:

        Preset server listening on: 127.0.0.1:9011
        Communicating on path: /preset
        Registered listeners:
        127.0.0.1:9050
        */
        presetServer.print();
        /* Alternatively, you can use the parameter server for the app to share the port
        */
        parameterServer().registerOSCListener(&presetServer);

    }

    void onAnimate(double dt) override {
        // You will want to disable navigation and text if the mouse is within
        // the gui window. You need to do this within the onAnimate callback
        navControl().active(!gui.usingInput());
    }

    void onDraw(Graphics &g) override
    {
        g.clear();

        g.pushMatrix();
        // You can get a parameter's value using the get() member function
        g.translate(X.get(), Y.get(), 0);
        g.scale(Size.get());
        g.draw(mesh); // Draw the mesh
        g.popMatrix();

        // Draw th GUI
        gui.draw(g);
    }

    /*
        The keyboard is used here to store and recall presets, and also to
        randomize the parameter values. See instructions below.

        The storePreset() function can be used passing only a string, but you can
        also assign a number index to each particular preset. The number index will
        become useful in the next example. For simplicity, the preset name and the
        preset index will be the same (although one is an int and the other a
        string).
    */
    bool onKeyDown(const Keyboard& k) override
    {
        std::string presetName = std::to_string(k.keyAsNumber());
        if (k.alt()) {
            if (k.isNumber()) { // Use alt + any number key to store preset
                presetHandler.storePreset(k.keyAsNumber(), presetName);
                std::cout << "Storing preset:" << presetName << std::endl;
            }
        } else {
            if (k.isNumber()) { // Recall preset using the number keys
                presetHandler.recallPreset(k.keyAsNumber());
                std::cout << "Recalling preset:" << presetName << std::endl;
            } else if (k.key() == ' ') { // Randomize parameters
                X = randomGenerator.uniformS();
                Y = randomGenerator.uniformS();
                Size = 0.1 + randomGenerator.uniform() * 2.0;
            }

        }
        return true;
    }

private:
    Light light;
    Mesh mesh;

    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};

    /*
        A PresetHandler object groups parameters and stores presets for them.

        Parameters are registered using the streaming operator.

        You need to specify the path where presets will be stored as the first
        argument to the constructor.

        A PresetHandler can store and recall presets using the storePreset() and
        recallPreset() functions. When a preset is recalled, the values are
        gradually "morphed" (i.e. interpolated linearly) until they reach their
        destination. The time of this morph is set using the setMorphTime()
        function.
    */
    PresetHandler presetHandler {"sequencerPresets"};
    /*
        A PresetServer exposes the preset handler via OSC. You provide the
        address and port on which to listen.
    */
    PresetServer presetServer {"127.0.0.1", 9011};

    al::rnd::Random<> randomGenerator; // Random number generator

    ControlGUI gui;
};


int main()
{
    MyApp app;
    app.start();
    return 0;
}
