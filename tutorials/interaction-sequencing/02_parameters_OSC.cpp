
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

/* A simple App that draws a cone on the screen
 * Parameters are exposed via OSC.
 */

class MyApp : public App {
public:
  void onCreate() override {
    // Set the camera to view the scene
    nav().pos(Vec3d(0, 0, 8));
    // Prepare mesh to draw a cone
    addCone(mesh);
    mesh.primitive(Mesh::LINE_STRIP);

    // Register the parameters with the GUI
    gui << X << Y << Size;
    gui.init(); // Initialize GUI. Don't forget this!

    /*
    Parameters need to be added to the ParameterServer by using the
    streaming operator <<.
    */
    parameterServer() << X << Y << Size; // Add parameters to parameter server

    /*
    You can add an OSC listener to a parameter server. Any change to any
    parameter will be broadcast to the OSC address and port registered using the
    addListener() function.
    */
    parameterServer().addListener("127.0.0.1", 13560);

    /*
    The print function of the ParameterServer object provides information
    on the server, including the paths to all registered parameters.

    You will see a cone on the screen. I will not move until you send OSC
    messages with values to any of these addresses:

    Parameter X : /Position/X
    Parameter Y : /Position/Y
    Parameter Scale : /Size/Scale
    */
    parameterServer().print();
  }

  void onAnimate(double dt) override {
    // You will want to disable navigation and text if the mouse is within
    // the gui window. You need to do this within the onAnimate callback
    navControl().active(!gui.usingInput());
  }

  void onDraw(Graphics &g) override {
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
      The parameter's name is the first argument, followed by the name of the
      group it belongs to. The group is used in particular by OSC to construct
      the access address.
  */
  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};

  ControlGUI gui;
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
