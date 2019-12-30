#include <string>

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/al_FontModule.hpp"

using namespace al;

/* Distributed App provides a simple way to share states and
 * parameters between a simulator and renderer apps using a
 * single source file.
 * You must define a state to be broadcast to all listeners
 * This state is synchronous but unreliable information
 * i.e. missed data should not affect the overall state of the
 * application
*/

struct SharedState {
    int frameCount;
};

// Inherit from DistributedApp and template it on the shared
// state data struct
class MyApp : public DistributedApp<SharedState>
{
public:

    void onCreate() override {
        // Set the camera to view the scene
        nav().pos(Vec3d(0,0,8));
        // Prepare mesh to draw a cone
        addCone(mesh);
        mesh.primitive(Mesh::LINES);

        // Register the parameters with the GUI
        gui << X << Y << Size;
        gui.init(); // Initialize GUI. Don't forget this!

        // DistributedApp provides a parameter server.
        // This links the parameters between "simulator" and "renderers" automatically
        parameterServer() << X << Y << Size;

        font.loadDefault(24);
        navControl().active(false);
    }

    // The simulate function is only called on the "simulator" instance
    void simulate(double dt) override {
        // You should write to the state on this function
        // Then the state will be available anywhere else in the application
        // (even for instances that are running on a different node on the
        // network)
        state().frameCount++;
    }

    void onAnimate(double dt) override {
    }

    void onDraw(Graphics &g) override
    {
        g.clear();

        g.pushMatrix();
        // You can get a parameter's value using the get() member function
        g.translate(X.get(), Y.get(), 0);
        g.scale(Size.get());
        g.draw(mesh); // Draw the mesh
        g.color(1);
        g.blendAdd();
        g.blending(true);
        font.render(g, std::to_string(state().frameCount));
        g.popMatrix();

        // Draw th GUI on the simulator only
        if (role() == ROLE_SIMULATOR || role() == ROLE_DESKTOP) {
            gui.draw(g);
        }
    }

private:
    Mesh mesh;
    FontModule font;

    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};


    /* DistributedApp provides a parameter server. In fact it will
     * crash if you have a parameter server with the same port,
     * as it will raise an exception when it is unable to acquire
     * the port
    */
//    ParameterServer paramServer {"127.0.0.1", 9010};

    ControlGUI gui;
};


int main()
{
    MyApp app;
    app.start();
    return 0;
}
