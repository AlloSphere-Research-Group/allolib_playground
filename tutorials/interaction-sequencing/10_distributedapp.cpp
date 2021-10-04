#include <string>

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Font.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

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
  uint16_t frameCount{0};
};

// Inherit from DistributedApp and template it on the shared
// state data struct
class MyApp : public DistributedAppWithState<SharedState> {
public:
  void onCreate() override {
    // Set the camera to view the scene
    nav().pos(Vec3d(0, 0, 8));
    // Prepare mesh to draw a cone
    addCone(mesh);
    mesh.primitive(Mesh::LINES);

    // Register the parameters with the GUI
    gui << X << Y << Size;
    gui.init(); // Initialize GUI. Don't forget this!

    // DistributedApp provides a parameter server.
    // This links the parameters between "simulator" and "renderers"
    // automatically
    parameterServer() << X << Y << Size;

    //    font.loadDefault(24);

    if (!font.load(Font::defaultFont().c_str(), 54, 1024)) {
      std::cerr << "Could not load font! Aborting." << std::endl;
      quit();
    }
    font.alignCenter();
  }

  void onAnimate(double /*dt*/) override {
    if (isPrimary()) {
      state().frameCount++;
      navControl().active(!isImguiUsingInput());
    }

    font.write(fontMesh, std::to_string(state().frameCount).c_str(), 1.0f);
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    g.pushMatrix();
    // You can get a parameter's value using the get() member function
    g.translate(X.get(), Y.get(), 0);
    g.scale(Size.get());
    g.color(1);
    g.draw(mesh); // Draw the mesh
    g.blendAdd();
    g.texture();
    font.tex.bind();
    g.draw(fontMesh);
    font.tex.unbind();

    g.popMatrix();

    // Draw the GUI on the simulator only
    if (isPrimary()) {
      gui.draw(g);
    }
  }

  bool onKeyDown(Keyboard const &k) override {
    if (k.key() == ' ') {
      // The space bar will turn off omni rendering
      if (omniRendering) {
        omniRendering->drawOmni = !omniRendering->drawOmni;
      } else {
        std::cout << "Not doing omni rendering" << std::endl;
      }
    }
    return true;
  }

private:
  Mesh mesh;
  Font font;
  Mesh fontMesh;

  Parameter X{"X", "Position", 0.0, -1.0f, 1.0f};
  Parameter Y{"Y", "Position", 0.0, -1.0f, 1.0f};
  Parameter Size{"Scale", "Size", 1.0, 0.1f, 3.0f};

  /* DistributedApp provides a parameter server. In fact it will
   * crash if you have a parameter server with the same port,
   * as it will raise an exception when it is unable to acquire
   * the port
   */
  //    ParameterServer paramServer {"127.0.0.1", 9010};

  ControlGUI gui;
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
