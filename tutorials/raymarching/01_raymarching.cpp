#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"


using namespace al;


// In this example we will demonstrate using a custom shader to render signed distance functions using raymarching

struct State {
  Pose pose;
};

struct RayApp : public DistributedAppWithState<State> {

  // we will render our raymarching shader to a fullscreen quad
  VAOMesh quad;

  // our raymarching shader program
  ShaderProgram rayShader;

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
  ParameterInt maxSteps{"maxSteps", "Raymarching", 1024, 16, 2048};
  Parameter stepSize{"stepSize", "Raymarching", 0.02, 0.01, 0.1};
  Parameter eyeSep{"eyeSep", "Raymarching", 0.02, 0., 0.5};
  Parameter translucent{"translucent", "Raymarching", 0.5, 0.0, 1.0};
  



  // for distributed App state synchronization
  std::shared_ptr<CuttleboneStateSimulationDomain<State>> cuttleboneDomain;


  void onInit() override {
    searchPaths.addSearchPath(".", false);
		searchPaths.addAppPaths();
    searchPaths.addRelativePath("../shaders", true);
    searchPaths.print();
  }

  void onCreate() override {
    cuttleboneDomain = CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone" << std::endl;
      quit();
    }

    // initialize quad mesh
    quad.primitive(Mesh::TRIANGLE_STRIP);
    quad.vertex(-1, -1, 0);
    quad.vertex(1, -1, 0);
    quad.vertex(-1, 1, 0);
    quad.vertex(1, 1, 0);
    quad.update();


    // Initialize GUI and Parameter callbacks
    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      gui = &guiDomain->newGUI();

      *gui << maxSteps << stepSize << translucent;

    }

    // parameters to be available via osc as well as distribute to all nodes
    parameterServer() << maxSteps << stepSize << translucent; 

    nav().pos(0,0,5);

    reloadShaders();
  }

  void reloadShaders() {
    loadShader(rayShader, "raymarch.vert", "raymarch.frag");
  }

  void onAnimate(double dt) override {

    if (watchCheck()) {
      printf("shader files changed, reloading..\n");
      reloadShaders();
    }

    if(isPrimary()){
      state().pose.set(nav());
    } else {
      nav().set(state().pose);
    }

  }


  void onDraw(Graphics &g) override {

    g.clear(0);

    rayShader.use();
    rayShader.uniform("max_steps", maxSteps)
    .uniform("step_size", stepSize)
    .uniform("translucent", translucent)
    .uniform("eye_sep", g.lens().eyeSep() * g.eye() / 2.0f)
    .uniform("foc_len", g.lens().focalLength())
    .uniform("al_ProjMatrixInv", Matrix4f::inverse(g.projMatrix()))
    .uniform("al_ViewMatrixInv", Matrix4f::inverse(g.viewMatrix()))
    .uniform("al_ModelMatrixInv", Matrix4f::inverse(g.modelMatrix()))
    .uniform("box_min", Vec3f(-1))  // use a bounding box volume to limit raycasting within this area
    .uniform("box_max", Vec3f(1));

    quad.draw();

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
  RayApp app;

  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1200, 800);
  app.start();
}
