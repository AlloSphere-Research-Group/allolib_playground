
/*
Because files and how to find them will be an issue when moving content to the
Allosphere, this tutorial shows common techniques to manage audio files
in the distributed Allosphere environment.

*/

#include "Gamma/Oscillator.h"
#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Mesh.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

struct CommonState {
  Nav nav;
};

class MyApp : public DistributedAppWithState<CommonState> {
public:
  // When the file name is fixed, we can keep it in a string:
  std::string file1;
  // We can use a ParameterString when we want to load the file when the value
  // changes
  ParameterString file2{"file"};
  // For files like textures that must be loaded in the graphics context,
  //  we will use asynchronous callbacks:
  ParameterString texture{"texture"};

  void loadFile(std::string path) {
    std::cout << "loading: " << path << std::endl;
  }

  void onInit() override {
    if (file1.size() == 0) {
      // set a default value if none is set on the command line
      file1 = "file.png";
    }
    // In most cases the filename for an asset will be the same for all nodes,
    // but what will change is the path.
    // We can use a different path according to the node where this application
    // is running.
    if (!sphere::isSphereMachine()) {
      // Your desktop machine
    } else if (isPrimary()) {
      // the primary node in the Allosphere is the audio machine (ar01) that
      // often manages simulation as well.
    } else {
      // The renderers will run this branch. It is rare that other machines
      // will run the application, but if they do they will run this too.
    }

    //    DistributedApp provides a mechanism to manage data root path
    // through dataRoot. The data root will be read from the
    // distributed_app.toml file, but it can be overriden at runtime.
    // You can also test for capabilities specifically:
    if (!sphere::isSphereMachine()) {
      // Your desktop machine
      dataRoot = "C:/Users/Andres/source/repos/allolib_data";
    } else if (hasCapability(Capability::CAP_STATE_SEND)) {
      // The default location for data in the audio machine is /Volumes/Data
      dataRoot = "/Volumes/Data";
    } else if (hasCapability(Capability::CAP_STATE_RECEIVE)) {
      // The default location for data for renderers is /data
      dataRoot = "/data";
    }
    loadFile(File::conformDirectory(dataRoot) + file1);

    // Whenever file2 changes, it will reigger an immediate load:
    file2.registerChangeCallback([&](auto value) {
      loadFile(File::conformDirectory(dataRoot) + value);
    });
    // If you are loading a texture you must ensure that it is loaded in the
    // graphics context. This is not problem if you are triggering the change
    // from the GUI on the simulator, but when that message is sent to the
    // renderers, the loading will happen on the network thread, as it is being
    // triggered by OSC, causing a crash.
    // To avoid this, we disable synchronous callbacks on the parameter. When
    // this is done, the callbacks will not be triggered automatically, but must
    // be explicitly called in the right context. Look at onDraw() below.

    texture.setSynchronousCallbacks(false);
    texture.registerChangeCallback([&](auto value) {
      loadFile(File::conformDirectory(dataRoot) + value);
    });
  }

  void onAnimate(double dt) override {

    if (isPrimary()) {
      state().nav = nav();
    } else {
      nav() = state().nav;
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0);
    // If there have been changes to the parameter value since last call to
    // processChange(), this triggers updating the parameter value and triggers
    // parameter callbacks. This only makes sense for parameters that have
    // disabled synchronous callbacks.
    texture.processChange();
    g.pushMatrix();
    g.popMatrix();
  }
};

int main(int argc, char *argv[]) {
  MyApp app;
  if (argc > 1) {
    // Set file from command line argument.
    app.file1 = argv[1];
  }
  app.start();
  return 0;
}
