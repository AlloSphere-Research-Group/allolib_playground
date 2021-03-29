/*
Allolib Tutorial: Vector field 2a

Description:
Tutorial to render vector fields.
Various methods to handle computation and sophisticated rendering techniques.

Example of using newton's method on the vector field
and rendering using a texture

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

  // std::vector to store the color of the vector field
  std::vector<Color> field;

  // Texture to store the image
  Texture tex;

  // Rectangle mesh to apply the texture
  VAOMesh quad;

  // variables for the algorithm's artistic manipulation
  float coef;
  bool goUp;
  Color baseColor;

  FieldApp() {
    // initialize variables
    xRes = 512;
    yRes = 512;
    scale = 2.f;

    coef = -0.2f;
    goUp = true;

    baseColor = Color(0.4f, 0.5f, 0.3f, 1.f);
  }

  void onCreate() {
    // pull the nav back a bit, so we can see the field being
    // rendered at the origin.
    // Some elements like nav need to be modified after being created
    nav().pos(0, 0, 4);

    // resize the field container
    field.resize(xRes * yRes);

    // set the filters for the texture. Default: NEAREST
    tex.filterMag(Texture::LINEAR);
    tex.filterMin(Texture::LINEAR);

    // create a texture unit on the GPU
    tex.create2D(xRes, yRes, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);

    // create the quad mesh to apply texture on
    quad.primitive(Mesh::TRIANGLE_STRIP);
    quad.vertex(-1, 1);
    quad.vertex(-1, -1);
    quad.vertex(1, 1);
    quad.vertex(1, -1);

    quad.texCoord(0, 1);
    quad.texCoord(0, 0);
    quad.texCoord(1, 1);
    quad.texCoord(1, 0);

    quad.update();
  }

  // square of complex number
  Vec2f p2(Vec2f p) {
    return Vec2f(p[0] * p[0] - p[1] * p[1], 2.f * p[0] * p[1]);
  }

  // cube of complex number
  Vec2f p3(Vec2f p) {
    return Vec2f(p[0] * p[0] * p[0] - 3.f * p[0] * p[1] * p[1],
                 3.f * p[0] * p[0] * p[1] - p[1] * p[1] * p[1]);
  }

  // function to apply newton's method
  // here we use p_next = p^9 + coef * p - i
  // with coef as the artistic manipulation
  Vec2f myF(Vec2f p) { return p3(p3(p)) + coef * p - Vec2f(0.f, 1.f); }

  // derivative of the function above
  Vec2f myFPrime(Vec2f p) { return 9.f * p2(p2(p2(p))) + Vec2f(coef, 0.f); }

  void onAnimate(double dt) {
    // reset the field
    for (auto &f : field) {
      f.set(0);
    }

    for (int j = 0; j < yRes; ++j) {
      for (int i = 0; i < xRes; ++i) {
        // get the middle of the pixel in a vector field -0.5~0.5 x -0.5~0.5
        Vec2f p((i + 0.5f) / (float)xRes - 0.5f,
                (j + 0.5f) / (float)yRes - 0.5f);

        // scale the vector field size
        p *= scale;

        // apply newton's method to find the root of the function
        // on each iteration add a bit of the base color
        // to the corresponding vector field
        int t = 0;
        while (myF(p).mag() > 1E-3 && t < 100) {
          Vec2f f = myF(p);
          Vec2f fP = myFPrime(p);

          p = p - Vec2f((f[0] * fP[0] + f[1] * fP[1]) /
                            (fP[0] * fP[0] + fP[1] * fP[1]),
                        (f[1] * fP[0] - f[0] * fP[1]) /
                            (fP[0] * fP[0] + fP[1] * fP[1]));

          field[i + j * xRes] += 0.02f * baseColor;

          ++t;
        }
      }
    }

    // increment the parameters based on the time elapsed from last frame
    // this allows animation to look smooth regardless of fps
    if (goUp) {
      coef += 0.0002f * dt;
      if (coef > -0.2f)
        goUp = false;
    } else {
      coef -= 0.0002f * dt;
      if (coef < -0.3f)
        goUp = true;
    }

    // update the texture with the modified vector field
    tex.submit(field.data());
  }

  void onDraw(Graphics &g) {
    g.clear();
    // use textures to color meshes
    g.texture();
    // bind the texture we want to use
    tex.bind();
    // render the quad to apply texture
    g.draw(quad);
    // unbind the texture after use
    tex.unbind();
  }
};

int main() {
  FieldApp app;
  app.start();
}
