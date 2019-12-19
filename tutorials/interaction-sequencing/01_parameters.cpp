
#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Shapes.hpp"
#include "al/util/ui/al_Parameter.hpp"
#include "al/util/ui/al_ControlGUI.hpp"

using namespace al;

/* A simple App that draws a cone on the screen */
class MyApp : public App
{
public:

    virtual void onCreate() override {
        // Set the camera to view the scene
        nav().pos(Vec3d(0,0,8));
        // Prepare mesh to draw a cone
        addCone(mesh);
        mesh.primitive(Mesh::LINE_STRIP);

        // Register the parameters with the GUI
        gui << X << Y << Size;
        gui.init(); // Initialize GUI. Don't forget this!
    }

    virtual void onAnimate(double dt) override {
        // You will want to disable navigation and text if the mouse is within
        // the gui window. You need to do this within the onAnimate callback
        navControl().active(!gui.usingInput());
    }

    virtual void onDraw(Graphics &g) override
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
private:
    Mesh mesh;

    /*
        The Parameter class encapsulates a single data value that can be shared
        safely between contexts (e.g. the graphics and audio threads) and that
        can be exposed to external control.
        The parameter constructor takes the following:

        Parameter(std::string parameterName, std::string Group,
              float defaultValue,
              std::string prefix = "",
              float min = -99999.0,
              float max = 99999.0
            );

        The parameter's name is the first argument, followed by the name of the
        group it belongs to. The "prefix" is similar to group, but we'll discuss
        differences in the next tutorial.

        You can constrain the values of the parameter using the min and max arguments.
        It is useful to set this values as they also determine the range for GUI
        elements.
    */
    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};

    ControlGUI gui;
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);
    app.start();

    return 0;
}
