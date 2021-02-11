/*
Allolib Tutorial: Vector field 3

Description:
Tutorial to render vector fields.
Various methods to handle computation and sophisticated rendering techniques.

Example of uploading the vector field to a Pixel buffer object(PBO)

More in-depth explanation of PBO can be found here
http://www.songho.ca/opengl/gl_pbo.html

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

  // parameters of the data type in the vector field
  int channels;
  int dataBytes;
  int dataSize;

  // std::vector to store the color of the vector field
  std::vector<Color> field;

  // PBO objects to upload the vector field
  BufferObject buffer[2];

  // Texture to store the image
  Texture tex;

  // Rectangle mesh to apply the texture
  VAOMesh quad;

  // phase of the sine waves in the algorithm used in the example
  float theta;

  FieldApp() {
    // initialize variables
    xRes = 512;
    yRes = 512;
    theta = 0.f;
    scale = 2.f;
    channels = 4;  // RGBA
    dataBytes = 4; // Floats are 32 bit = 4 bytes
    dataSize = xRes * yRes * channels * dataBytes;
  }

  void onCreate() {
    // pull the nav back a bit, so we can see the field being
    // rendered at the origin.
    // Some elements like nav need to be modified after being created
    nav().pos(0, 0, 4);

    // resize the field container
    field.resize(xRes * yRes);

    // set the buffer object as a PBO and configure
    // GL_PIXEL_UNPACK_BUFFER: uploading pixel data to OpenGL
    // GL_STREAM_DRAW: streaming texture upload
    buffer[0].bufferType(GL_PIXEL_UNPACK_BUFFER);
    buffer[0].usage(GL_STREAM_DRAW);
    buffer[0].create();
    buffer[0].bind();
    buffer[0].data(dataSize, nullptr); // reserve memory space
    buffer[0].unbind();

    buffer[1].bufferType(GL_PIXEL_UNPACK_BUFFER);
    buffer[1].usage(GL_STREAM_DRAW);
    buffer[1].create();
    buffer[1].bind();
    buffer[1].data(dataSize, nullptr);
    buffer[1].unbind();

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

  void onAnimate(double dt) {
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

        // store the color in the container
        field[xRes * j + i] = color;
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
    // use textures to color meshes
    g.texture();

    // static index to switch between 0 and 1 every frame
    static int index = 0;
    int nextIndex = index;   // index for the next frame
    index = (index + 1) % 2; // switch between 0 and 1

    // bind the texture we want to use
    tex.bind();
    buffer[index].bind(); // bind the PBO to use

    // Transfer pixel data from the bound PBO to the texture
    // 0 at the end acts as the offset instead of a pointer if there's
    // a PBO is bound
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, xRes, yRes, GL_RGBA, GL_FLOAT, 0);

    // bind the next PBO to use in the next frame
    buffer[nextIndex].bind();

    // discard previous data and create a new data store
    buffer[nextIndex].data(dataSize, nullptr);

    // map the buffer's memory to a client's memory space
    void *ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

    if (ptr) {
      // copy the vector field to the mapped memory space
      std::memcpy(ptr, field.data(), dataSize);

      // release the mapping
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }

    // unbind the PBO
    buffer[nextIndex].unbind();

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
