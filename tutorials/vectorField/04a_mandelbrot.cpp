/*
Allolib Tutorial: Vector field 4a

Description:
Tutorial to render vector fields.
Various methods to handle computation and sophisticated rendering techniques.

Example of calculating mandelbrot fractal using shaders

Author:
Kon Hyong Kim - Jan 2021
*/

#include "al/app/al_App.hpp"
#include <vector>
using namespace al;

// vertex shader code stored as string
const std::string shader_vert = R"(
#version 330
// graphics class sends parameters essential in rendering
uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texcoord;

// variable to pass to the fragment shader
out vec2 T;

void main(void) {
  // normalize the texture coordinate to -1 ~ 1
  T = 2. * vec2(texcoord) - 1;

  // calculate vertex position to be used in rendering
  // position is converted to a homogeneous coordinate
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * vec4(position, 1.0);//vec4(T, 0, 1);
}
)";

//
const std::string shader_frag = R"(
#version 330
// variable sent from fragment shader
in vec2 T;
// output color of frament shader
layout (location = 0) out vec4 fragColor;

void main(){
  vec2 p = vec2(0);
  int count = 0;
  // basic mandelbrot algorithm with p_(n+1) = p_n^2 + c,
  // where c is the vector position in the complex plane
  // we assume vector escapes if magnitude exceeds 200 before 100 iterations
  while (length(p) < 200 && count < 100) {
    p = vec2(p.x * p.x - p.y * p.y, 2 * p.x * p.y);
    p += T;
    ++count;
  }

  // store the color differently for points that didn't escape
  if (length(p) < 200) {
    fragColor = vec4(1);
  } else {
    fragColor = vec4(0.1, 0.1, 0.1, 1);
  }
}
)";

class FieldApp : public App {
public:
  // resolution of the vector field
  int xRes;
  int yRes;

  // Texture to store the image
  Texture tex;

  // Rectangle mesh to apply the texture
  VAOMesh quad;

  // Shader program to hold glsl code
  ShaderProgram shaderProgram;

  FieldApp() {
    // initialize variables
    xRes = 512;
    yRes = 512;
  }

  void onCreate() {
    // pull the nav back a bit, so we can see the field being
    // rendered at the origin.
    // Some elements like nav need to be modified after being created
    nav().pos(0, 0, 4);

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

    // compile the shader program
    shaderProgram.compile(shader_vert, shader_frag);
  }

  void onDraw(Graphics &g) {
    g.clear();
    // use textures to color meshes
    g.texture();

    tex.bind();

    g.shader(shaderProgram);

    // render the quad to apply texture while using the shader program
    g.draw(quad);

    // unbind the texture after use
    tex.unbind();
  }
};

int main() {
  FieldApp app;
  app.start();
}
