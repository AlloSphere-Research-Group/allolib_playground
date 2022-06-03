#include "al/app/al_DistributedApp.hpp"
using namespace al;

struct MyApp : DistributedApp {
  Mesh mesh;

  void onInit() override {
    mesh.primitive(Mesh::TRIANGLES);
    addCube(mesh);
    for (int i = 0; i < mesh.vertices().size(); ++i) {
      float f = (float)i / mesh.vertices().size();
      mesh.color(Color(HSV(f, 1 - f, 1), 1));
    }
    mesh.generateNormals();
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    // Put cubes throughout space

    float min = -20;
    float max = 20;
    float spacing = 4;

    g.meshColor();
    for (float x = min; x < max; x += spacing) {
      for (float y = min; y < max; y += spacing) {
        for (float z = min; z < max; z += spacing) {
          if (abs(x) < 1 && abs(y) < 1 && abs(z) < 1) {
            // Don't draw too near the origin
          } else {
            g.pushMatrix();
            g.translate(x, y, z);
            g.draw(mesh);
            g.popMatrix();
          }
        }
      }
    }
  }

  void changeEyeSep(float delta) {
    float before = lens().eyeSep();
    float after = before + delta;

    lens().eyeSep(after);

    std::cout << "Changed eye separation from " << before << " to " << after
              << std::endl;
  }

  void flipEyes() {
    float before = lens().eyeSep();
    float after = -before;

    lens().eyeSep(after);

    std::cout << "Flipping eye separation from " << before << " to " << after
              << std::endl;
  }

  bool onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case Keyboard::TAB:
      //        omniRendering->stereo((!omni().stereo());
      return false;
    case '-':
      changeEyeSep(-0.01);
      break;
    case '+':
      changeEyeSep(0.01);
      break;
    case '0':
      flipEyes();
      break;
    default:;
    }
    return true;
  }
};

int main(int argc, char *argv[]) {
  MyApp().start();
  return 0;
}
