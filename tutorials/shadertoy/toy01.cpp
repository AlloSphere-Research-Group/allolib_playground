/*
 * Shadertoy-style example using allolib's ShaderManager
 *
 * This example demonstrates:
 * - Using ShaderManager to automatically reload shaders on file changes
 * - Passing shadertoy.com-style uniforms (iTime, iResolution, iMouse, etc.)
 * - Rendering a fullscreen quad with a fragment shader
 */

#include "al/app/al_App.hpp"
#include "al/graphics/al_ShaderManager.hpp"
#include "al/io/al_File.hpp"
#include "al/system/al_Time.hpp"

using namespace al;

struct ShadertoyApp : App {
  ShaderManager shaderManager;
  VAOMesh quad;
  SearchPaths searchPaths;

  // Time tracking
  al_sec startTime;
  al_sec currentTime;
  al_sec deltaTime;

  // Mouse tracking (for iMouse uniform)
  Vec2f mousePos{0.0f, 0.0f};
  Vec2f mouseClick{0.0f, 0.0f};

  void onInit() override {
    // Set up search paths for shader files
    searchPaths.addSearchPath(".", false);
    searchPaths.addAppPaths();
    searchPaths.addRelativePath("shaders", true);
    searchPaths.print();

    // Set search paths for shader manager
    shaderManager.setSearchPaths(searchPaths);

    // Set polling interval for file watching (check every 0.1 seconds)
    shaderManager.setPollInterval(0.1);
  }

  void onCreate() override {
    // Initialize time tracking
    startTime = al_steady_time();
    currentTime = 0.0;
    deltaTime = 0.0;

    // Create a fullscreen quad
    quad.primitive(Mesh::TRIANGLE_STRIP);
    quad.vertex(-1, -1, 0); // Bottom-left
    quad.vertex(1, -1, 0);  // Bottom-right
    quad.vertex(-1, 1, 0);  // Top-left
    quad.vertex(1, 1, 0);   // Top-right

    // Add UV coordinates for texture sampling
    quad.texCoord(0, 0); // Bottom-left
    quad.texCoord(1, 0); // Bottom-right
    quad.texCoord(0, 1); // Top-left
    quad.texCoord(1, 1); // Top-right

    quad.update();

    // Add shader program to manager
    // The shader files will be watched for changes
    shaderManager.add("shadertoy", "shadertoy.vert", "shadertoy.frag");

    // Print managed shaders
    shaderManager.print();

    // Set window title
    defaultWindow().title("Shadertoy");
  }

  void onAnimate(double dt) override {
    // Update shader manager (checks for file changes and reloads if needed)
    if (shaderManager.update()) {
      std::cout << "Shader reloaded!" << std::endl;
    }

    // Update time
    al_sec now = al_steady_time();
    currentTime = now - startTime;
    deltaTime = dt;
  }

  void onDraw(Graphics &g) override {
    g.clear(0);

    // Get window dimensions
    int w = width();
    int h = height();
    float aspect = (float)w / (float)h;

    // Use the shader
    shaderManager.use("shadertoy");
    ShaderProgram &shader = shaderManager.get("shadertoy");

    // Set shadertoy.com-style uniforms
    shader
        .uniform("iTime", (float)currentTime)        // Time since start
        .uniform("iTimeDelta", (float)deltaTime)     // Time since last frame
        .uniform("iResolution", Vec3f(w, h, aspect)) // Viewport resolution
        .uniform("iMouse", Vec4f(mousePos.x, mousePos.y, mouseClick.x,
                                 mouseClick.y)) // Mouse position and click
        .uniform("iFrame",
                 (int)(currentTime * 60.0)); // Approximate frame number

    // Draw fullscreen quad
    quad.draw();
  }

  bool onMouseMove(const Mouse &m) override {
    // Update mouse position (normalized to [0, 1])
    mousePos.x = (float)m.x() / width();
    mousePos.y = 1.0f - (float)m.y() / height(); // Flip Y axis
    return true;
  }

  bool onMouseDown(const Mouse &m) override {
    // Store click position
    mouseClick.x = (float)m.x() / width();
    mouseClick.y = 1.0f - (float)m.y() / height(); // Flip Y axis
    return true;
  }
};

int main() {
  ShadertoyApp app;

  // Configure window dimensions
  app.dimensions(800, 600);

  app.start();

  return 0;
}
