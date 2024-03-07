#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/ui/al_ParameterGUI.hpp"


using namespace al;


// In this example we will demonstrate using fbos and texture feedback to create a diffusion shader
// fbo0 will be used to capture diffusion shader output
// tex0 and tex1 will be used to alternate between shader fbo output and shader input to create feedback

struct DiffusionApp : public App {

  Texture *tex0, *tex1;
  RBO rbo0;
  FBO fbo0;

  ShaderProgram diffusionShader;

  // we will watch and auto reload shader files on change
  SearchPaths searchPaths;
  struct WatchedFile { 
    File file;
    al_sec modified;
  };
  std::map<std::string, WatchedFile> watchedFiles;
  al_sec watchCheckTime;

  // For simple Gui to control parameters
  ControlGUI *gui;

  // Parameters for interactive control of uniforms
  ParameterInt steps{"double_steps_per_frame", "", 1, 1, 30};
  Parameter dx{"dx", "", 1, 0, 10};
  Parameter dy{"dy", "", 1, 0, 10};
  

  void updateFBO(int w, int h) {
    // Note: all attachments (textures, RBOs, etc.) to the FBO must have the same width and height.

    // Configure texture on GPU
    tex0->create2D(w, h, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);
    tex1->create2D(w, h, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);

    // Configure render buffer object on GPU
    rbo0.resize(w, h);

    // Finally, attach color texture and depth RBO to FBO
    fbo0.bind();
    // fbo0.attachTexture2D(tex0);
    fbo0.attachRBO(rbo0);
    fbo0.unbind();
    
    // init diffusion input tex0 with values to diffuse
    // Loop through the pixels to generate an image
    vector<float> pixels;
    pixels.resize(4 * w * h);

    for (int j = 0; j < h; ++j) {
      int y = j - h/2;
      for (int i = 0; i < w; ++i) {
        int x = i - w/2;

        if( abs(x % 200) + abs(y % 200) < 200){
          int idx = j * w + i;
          pixels[idx * 4 + 0] = 1.0;
          pixels[idx * 4 + 1] = 1.0;
          pixels[idx * 4 + 2] = 1.0;
          pixels[idx * 4 + 3] = 1.0;
        }
      }
    }

    tex0->submit(pixels.data());

  }

  void onInit() override {
		searchPaths.addAppPaths();
    searchPaths.addRelativePath("../shaders", true);
    searchPaths.print();
  }



  void onCreate() override {

    // initialize fbo, rbo, texture for rendering using default framebuffer dimensions
    tex0 = new Texture();
    tex1 = new Texture();
    updateFBO(fbWidth(), fbHeight());

    // Initialize GUI and Parameter callbacks
    auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
    gui = &guiDomain->newGUI();
    *gui << steps << dx << dy;

    nav().pos(0,0,5);

    reloadShaders();
  }

  void reloadShaders() {
    loadShader(diffusionShader, "uv.vert", "diffusion.frag");
  }

  void onAnimate(double dt) override {

    if (watchCheck()) {
      printf("shader files changed, reloading..\n");
      reloadShaders();
    }
  }


  void onDraw(Graphics &g) override {

    g.framebuffer(fbo0);

    for(int i = 0; i < steps * 2; i++){
      fbo0.attachTexture2D(*tex1); // tex1 will act as fbo0 render target
      g.viewport(0, 0, fbWidth(), fbHeight());
      g.camera(Viewpoint::IDENTITY);

      diffusionShader.use();
      diffusionShader.uniform("tex", 0);
      diffusionShader.uniform("size", Vec2f(fbWidth(),fbHeight()));
      diffusionShader.uniform("dx", dx);
      diffusionShader.uniform("dy", dy);
      g.quad(*tex0,-1,-1,2,2);


      // swap textures
      Texture *tmp = tex0;
      tex0 = tex1;
      tex1 = tmp;
    }

    // draw to screen
    g.framebuffer(FBO::DEFAULT);
    g.clear(0, 0, 0.5);
    g.viewport(0, 0, fbWidth(), fbHeight());
    g.camera(Viewpoint::IDENTITY);
    g.quad(*tex1,-1,-1,2,2);
  }

  bool onKeyDown(const Keyboard &k) override {}

  void watchFile(std::string path) {
    File file(searchPaths.find(path).filepath());
    watchedFiles[path] = WatchedFile{ file, file.modified() };
  }

  bool watchCheck() {
    bool changed = false;
    if (floor(al_system_time()) > watchCheckTime) {
      watchCheckTime = floor(al_system_time());
      for (std::map <std::string, WatchedFile>::iterator i = watchedFiles.begin(); i != watchedFiles.end(); i++) {
        WatchedFile& watchedFile = (*i).second;
        if (watchedFile.modified != watchedFile.file.modified()) {
          watchedFile.modified = watchedFile.file.modified();
          changed = true;
        }
      }
    }
    return changed;
  }

  // read in a .glsl file
  // add it to the watch list of files
  // replace shader #include directives with corresponding file code
  std::string loadGlsl(std::string filename) {
    watchFile(filename);
		std::string code = File::read(searchPaths.find(filename).filepath());
		size_t from = code.find("#include \"");
		if (from != std::string::npos) {
			size_t capture = from + strlen("#include \"");
			size_t to = code.find("\"", capture);
			std::string include_filename = code.substr(capture, to-capture);
			std::string replacement = File::read(searchPaths.find(include_filename).filepath());
			code = code.replace(from, to-from+2, replacement);
			//printf("code: %s\n", code.data());
		}
		return code;
	}

  void loadShader(ShaderProgram& program, std::string vp_filename, std::string fp_filename) {
		std::string vp = loadGlsl(vp_filename);
		std::string fp = loadGlsl(fp_filename);
		program.compile(vp, fp);
	}

};




int main() {
  DiffusionApp app;

  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1200, 800);
  app.start();
}
