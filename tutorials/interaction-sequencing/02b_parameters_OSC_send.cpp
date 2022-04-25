
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

/* This App sends OSC messages when the GUI controls are changed.
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
    gui.setTitle("GUI for sending OSC messages");
    gui.init(); // Initialize GUI. Don't forget this!

    // The simplest way to send OSC to another allolib app is to use the
    // parameterServer()
    parameterServer() << X << Y;

    // Send parameter changes to server.
    parameterServer().addListener("localhost", 9010);

    // If you want to have more control, you can send the OSC directly
    // whenever the value changes:
    Size.registerChangeCallback([&](float value) {
      uint16_t port = 9010;
      osc::Send s(port, "127.0.0.1");
      std::string oscAddr = "/Size/Scale";
      s.send(oscAddr, value);
      std::cout << "Sent " << value << " to " << oscAddr << " on port " << port
                << std::endl;
    });
  }

  void onDraw(Graphics &g) override {
    g.clear(0.5);

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
