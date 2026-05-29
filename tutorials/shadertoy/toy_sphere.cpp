/*
 * Shadertoy-style surround: one pass on an inner sphere. Fragment shader uses
 * mesh equirect UVs (from addTexSphere) to build a world direction and shade.
 *
 * DistributedApp; pose sync like omnirendering.cpp.
 */

#include <string>

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_ShaderManager.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Matrix4.hpp"
#include "al/system/al_Time.hpp"
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp"

using namespace al;

// Per frame state synchronized with render nodes via DistributedAppWithState
struct State {
  Pose pose;
  double time;
};

struct ShadertoySphereApp : public DistributedAppWithState<State> {
  ShaderManager shaderManager;
  VAOMesh sphere;
  SearchPaths searchPaths;

  std::string activeShader{"shadertoy_sphere"};

  // Parameters are synchronized to render nodes over OSC via parameterServer()
  ParameterInt shaderIndex{"shaderIndex"};
  ParameterVec3 cursorPos{"cursorPos"};

  void onInit() override {
    searchPaths.addSearchPath(".", false);
    searchPaths.addAppPaths();
    searchPaths.addRelativePath("shaders", true);
    searchPaths.print();

    shaderManager.setSearchPaths(searchPaths);
    shaderManager.setPollInterval(0.1);
  }

  void onCreate() override {

    CuttleboneDomain<State>::enableCuttlebone(this);

    parameterServer() << shaderIndex << cursorPos;

    addTexSphere(sphere, 50.0, 64, true);
    sphere.update();

    shaderManager.add("shadertoy_sphere", "shadertoy_sphere.vert",
                      "shadertoy_sphere.frag");
    shaderManager.add("sphere_template", "shadertoy_sphere.vert",
                      "shadertoy_sphere_template.frag");
    shaderManager.print();

    shaderIndex.registerChangeCallback([&](int index) {
      switch (index) {
      case 1:
        activeShader = "shadertoy_sphere";
        break;
      case 2:
        activeShader = "sphere_template";
        break;
      }
    });
  }

  void onAnimate(double dt) override {
    if (shaderManager.update()) {
      std::cout << "Shader reloaded!" << std::endl;
    }

    // handle state data on primary vs non-primary
    if (isPrimary()) {
      state().pose = pose();
      state().time += dt;
    } else {
      pose().set(state().pose);
    }
  }

  void setShadertoyUniforms(ShaderProgram &shader) {}

  void onDraw(Graphics &g) override {
    // g.viewport(0, 0, fbWidth(), fbHeight());
    g.depthTesting(false);
    g.lighting(false);
    g.blending(false);
    g.culling(false);
    g.clear(0., 0., 0.);

    ShaderProgram &shader = shaderManager.get(activeShader);
    g.shader(shader);

    shader.uniform("iTime", static_cast<float>(state().time));
    shader.uniform("iMouse", Vec4f(cursorPos.get().x, cursorPos.get().y,
                                   cursorPos.get().z, 0.));
    shader.uniform("eye_sep", 0.f);
    shader.uniform("foc_len", lens().focalLength());
    g.draw(sphere);
  }

  bool onMouseMove(const Mouse &m) override {
    if (width() <= 0 || height() <= 0) {
      return true;
    }
    Vec3f p;
    p.x = static_cast<float>(m.x()) / static_cast<float>(width());
    p.y = static_cast<float>(m.y()) / static_cast<float>(height());
    cursorPos.set(p);
    return true;
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == '1') {
      shaderIndex = 1;
    } else if (k.key() == '2') {
      shaderIndex = 2;
    }
    return false;
  }

  bool onMouseDown(const Mouse &m) override {
    if (width() <= 0 || height() <= 0) {
      return true;
    }
    Vec3f p;
    p.x = static_cast<float>(m.x()) / static_cast<float>(width());
    p.y = static_cast<float>(m.y()) / static_cast<float>(height());
    //
    return true;
  }
};

int main() {
  ShadertoySphereApp app;
  app.dimensions(1024, 512);
  app.start();
  return 0;
}
