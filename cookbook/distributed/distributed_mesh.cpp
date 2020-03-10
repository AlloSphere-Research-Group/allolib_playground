//#include <string>

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

const size_t maxMeshDataSize = 128;

struct SharedState {
  float meshData[maxMeshDataSize];
  size_t meshVertices = 0;
  size_t meshIndeces = 0;
  size_t meshColors = 0;
};

void meshSerialize(Mesh &mesh, float *meshData, size_t &numVertices, size_t &numIndices, size_t &numColors) {
    if (mesh.vertices().size() * 3 + mesh.indices().size() +mesh.colors().size() * 4 > maxMeshDataSize) {
        numVertices = numIndices = numColors = 0;
    return;
  }
  numVertices = mesh.vertices().size();
  for (auto vertex : mesh.vertices()) {
    *meshData++ = vertex.x;
    *meshData++ = vertex.y;
    *meshData++ = vertex.z;
  }
  numIndices = mesh.indices().size();
  for (auto index : mesh.indices()) {
      *meshData++ = index;
  }
  numColors = mesh.colors().size();
  for (auto color : mesh.colors()) {
      *meshData++ = color.r;
      *meshData++ = color.g;
      *meshData++ = color.b;
      *meshData++ = color.a;
  }
}

void meshDeserialize(Mesh &mesh, float *meshData, size_t numVertices, size_t numIndices, size_t numColors) {
  mesh.vertices().resize(numVertices); // Allocate upfront if needed
  for (auto &vertex : mesh.vertices()) {
    vertex.x = *meshData++;
    vertex.y = *meshData++;
    vertex.z = *meshData++;
  }
  mesh.indices().resize(numIndices); // Allocate upfront if needed
  for (auto &index : mesh.indices()) {
      index = *meshData++;
  }
  mesh.colors().resize(numColors); // Allocate upfront if needed
  for (auto &color : mesh.colors()) {
      color.r = *meshData++;
      color.g = *meshData++;
      color.b = *meshData++;
      color.a = *meshData++;
  }


}

// Inherit from DistributedApp and template it on the shared
// state data struct
class MyApp : public DistributedAppWithState<SharedState> {
public:
  void onCreate() override {
    // Set the camera to view the scene
    nav().pos(Vec3d(0, 0, 8));
    // Prepare mesh to draw a cone
    addCone(mesh);
    mesh.primitive(Mesh::TRIANGLE_STRIP);
    navControl().active(false);
  }

  void onAnimate(double /*dt*/) override {
    if (isPrimary()) {
      meshSerialize(mesh, state().meshData, state().meshVertices, state().meshIndeces, state().meshColors);
      //      navControl().active(!isImguiUsingInput());
    } else {
      meshDeserialize(mesh, state().meshData, state().meshVertices, state().meshIndeces, state().meshColors);
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0);
    gl::polygonFill();
    g.meshColor();
    g.draw(mesh); // Draw the mesh

  }

  bool onKeyDown(Keyboard const &k) override {
    if (k.key() == ' ') {
      // The space bar will turn off omni rendering
      if (omniRendering) {
        omniRendering->drawOmni = !omniRendering->drawOmni;
      } else {
        std::cout << "Not doing omni rendering" << std::endl;
      }
    } else {
      mesh.reset();

      for (int i = 0 ; i < 4; i++) {
          mesh.vertex(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
          mesh.color(rnd::uniform(), rnd::uniform(), rnd::uniform());
      }
    }
    return true;
  }

private:
  Mesh mesh;
};

int main() {
  MyApp app;
  app.start();
  return 0;
}
