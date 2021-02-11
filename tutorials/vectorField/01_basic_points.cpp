/*
Allolib Tutorial: Vector field 1

Description:
Tutorial to render vector fields.
Various methods to handle computation and sophisticated rendering techniques.

Basic example of vector field and rending with point meshes

Author:
Kon Hyong Kim - Jan 2021
*/

#include "al/app/al_App.hpp"
#include <vector>
using namespace al;

class FieldApp : public App {
public:
  // resolution of the vector field
  int xRes;
  int yRes;

  // scale of the vector field
  float scale;

  // mesh to store the points we're rendering
  Mesh mesh;

  // phase of the sine waves in the algorithm used in the example
  float theta;

  FieldApp() {
    // initialize variables
    xRes = 512;
    yRes = 512;
    theta = 0.f;
    scale = 2.f;
  }

  void onCreate() {
    // pull the nav back a bit, so we can see the field being
    // rendered at the origin.
    // Some elements like nav need to be modified after being created
    nav().pos(0, 0, 4);
  }

  void onAnimate(double dt) {
    // reset the mesh
    mesh.reset();
    // configure the mesh to render using individual points
    mesh.primitive(Mesh::POINTS);

    for (int j = 0; j < yRes; ++j) {
      for (int i = 0; i < xRes; ++i) {
        // get the middle of the pixel in a vector field -0.5~0.5 x -0.5~0.5
        Vec3f p((i + 0.5f) / (float)xRes - 0.5f,
                (j + 0.5f) / (float)yRes - 0.5f, 0.f);

        // scale the vector field size
        p *= scale;

        // ** place to apply algorithms based on the vector field
        // here we're coloring the vector field based on the radius
        // and a sine wave as an example
        float radius = p.mag();
        // RGB that fluctuates from 0-1 based on radius and theta
        // with different periods
        Color color = Color(0.5f * sin(8.f * radius + theta) + 0.5f,
                            0.5f * sin(7.f * radius + theta) + 0.5f,
                            0.5f * sin(5.f * radius + theta) + 0.5f);

        // here we're rendering a point based on the vector field
        mesh.vertex(p);
        mesh.color(color);
      }
    }

    // increment the phase based on the time elapsed from last frame
    // this allows animation to look smooth regardless of fps
    theta += 1.5f * dt;
    if (theta > 2 * M_PI)
      theta -= 2 * M_PI;
  }

  void onDraw(Graphics &g) {
    g.clear();
    // use the color stored in the mesh
    g.meshColor();
    g.draw(mesh);
  }
};

int main() {
  FieldApp app;
  app.start();
}
