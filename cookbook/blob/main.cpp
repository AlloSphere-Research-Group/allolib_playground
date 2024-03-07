
#include "al/app/al_DistributedApp.hpp"
#include "al/math/al_Random.hpp"

#include "al/app/al_GUIDomain.hpp"

#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#include <Gamma/Noise.h>

using namespace al;

#include <iostream> // cout
#include <vector> // vector

// This example demonstrates how to write a distributed application that
// shares mesh vertices through cuttlebone.
// Original by Karl Yerkes, adapted by Andres Cabrera

// State --------------------------
#define N 162
//#define N 642
//#define N 2562
//#define N 10242
//#define N 40962
//#define N 163842
//#define N 655362

struct State {
  Pose pose; // for navigation

  // this is how you might control renderering settings.
  double eyeSeparation;
  Color backgroundColor{0};
  bool wireFrame; // alternative is shaded

  // all of the above data could be distributed using unicast OSC to each
  // renderering host, but this scheme could suffer
  // from tearing due to out-of-sync sending and receiving of packets. the win
  // above is that all this data appears at each renderering host
  // simultaneously because we're using UDP broadcast, which does not use a
  // foreach to send N identical messages to N renderering hosts.
  //
  // below is another, different win. here we have an array of vertices that
  // could get as big as the network will bear (~10MB). this data is calculated
  // on the server/simulator, so it does not need to be calculated on the
  // renderer, only interpreted.
  //

  Vec3f p[N];
};

// Load file into mesh
bool load(std::string fileName, Mesh &mesh, std::vector<std::vector<int>> &nn) {
  std::ifstream file(fileName);
  if (!file.is_open())
    return false;

  std::string line;
  int state = 0;
  while (getline(file, line)) {
    if (line == "|") {
      state++;
      continue;
    }
    switch (state) {
    case 0: {
      std::vector<float> v;
      std::stringstream ss(line);
      float f;
      while (ss >> f) {
        v.push_back(f);
        if (ss.peek() == ',')
          ss.ignore();
      }
      mesh.vertex(v[0], v[1], v[2]);
      // cout << v[0] << "|" << v[1] << "|" << v[2] << endl;
    } break;

    case 1: {
      std::stringstream ss(line);
      int i;
      if (ss >> i)
        mesh.index(i);
      else
        return false;
      // cout << i << endl;
    } break;

    case 2: {
      std::vector<int> v;
      std::stringstream ss(line);
      int i;
      while (ss >> i) {
        v.push_back(i);
        if (ss.peek() == ',')
          ss.ignore();
      }
      if ((v.size() != 5) && (v.size() != 6))
        return false;
      nn.push_back(v);
      // cout << nn[nn.size() - 1].size() << endl;
    } break;
    }
  }
  file.close();

  return true;
}

#ifdef AL_WINDOWS
// Damn you Windows!
#undef near
#undef far
#endif

// Create a new DistributedAppWithState, templated on the state data structure
// that will be shared on the network

struct Blob : DistributedAppWithState<State> {

  // simulation parameters
  Parameter SK{"SK", "", 0.06f, 0.01f,
               0.3f}; // spring constant for anchor points
  Parameter NK{"NK", "", 0.1f, 0.01f,
               0.3f};                       // spring constant between neighbors
  Parameter D{"D", "", 0.08f, 0.01f, 0.3f}; // damping factor

  // Display parameters
  ParameterColor bgColor{"BackgroundColor", "", Color(0)};
  ParameterBool wireFrame{"wireFrame", "", true};

  // Internal computation data
  // This data will not be shared to remote nodes, so you should only use it on
  // the simulator machine
  vector<vector<int>> nn;
  vector<Vec3f> velocity;
  vector<Vec3f> original;

  // a boolean value that is read and reset (false) by the simulation step and
  // written (true) by audio, keyboard and mouse callbacks.
  bool shouldPoke;
  unsigned pokedVertex;
  Vec3f pokedVertexRest;

  // a mesh we use to do graphics rendering in this app
  Mesh mesh;

  gam::NoisePink<> pinkNoise;

  void onInit() override {

    mesh.primitive(Mesh::TRIANGLES);

    SearchPaths searchPaths;
    searchPaths.addSearchPath(".", false);
    searchPaths.addSearchPath("/alloshare/blob", false);
    searchPaths.addAppPaths();

    std::string icoSphereFile = std::to_string(N) + ".ico";

    if (!load(searchPaths.find(icoSphereFile).filepath(), mesh, nn)) {
      std::cout << "cannot find " << icoSphereFile << std::endl;
      quit();
    }
    if (isPrimary()) {
      shouldPoke = true; // start with a poke

      // Initialize simulation data
      velocity.resize(mesh.vertices().size(), Vec3f(0, 0, 0));
      original.resize(mesh.vertices().size());
      for (int i = 0; i < mesh.vertices().size(); i++)
        original[i] = mesh.vertices()[i];

      for (int i = 0; i < N; i++)
        state().p[i] = original[i];
      state().eyeSeparation = 0.03;
      state().backgroundColor = Color(0.1f, 0.1f);
      state().wireFrame = true;
    }

    // Enable cuttlebone for state distribution
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }
    // GUI
    if (isPrimary()) {
      auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto &gui = guiDomain->newGUI();
      gui << SK << NK << D << wireFrame << bgColor;
    }
  }

  //  void onCreate() override {}

  void onAnimate(double dt) override {

    if (isPrimary()) {

      if (shouldPoke) {
        shouldPoke = false;
        int n = al::rnd::uniform(N);
        pokedVertex = n;
        pokedVertexRest = original[n];
        Vec3f v = Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
        for (unsigned k = 0; k < nn[n].size(); k++)
          state().p[nn[n][k]] += v * 0.5;
        state().p[n] += v;
      }

      // Compute new postions
      for (int i = 0; i < N; i++) {
        Vec3f &v = state().p[i];
        Vec3f force = (v - original[i]) * -SK;

        for (int k = 0; k < nn[i].size(); k++) {
          Vec3f &n = state().p[nn[i][k]];
          force += (v - n) * -NK;
        }

        force -= velocity[i] * D;
        velocity[i] += force;
      }

      for (int i = 0; i < N; i++) {
        state().p[i] += velocity[i];
      }

      // Update variables in state to send to nodes
      state().pose = nav();
      state().backgroundColor = bgColor;
      state().wireFrame = wireFrame;

    } else {
      // For remote nodes, update pose and color from state
      pose() = state().pose;
      bgColor = state().backgroundColor;
      wireFrame = state().wireFrame;
    }
    // Copy vertex positions from state to mesh
    memcpy(&mesh.vertices()[0], &state().p[0], sizeof(Vec3f) * N);
  }

  void onDraw(Graphics &g) override {
    g.pushMatrix();
    g.clear(state().backgroundColor);
    if (state().backgroundColor.luminance() > 0.5) {
      g.color(0.0);
    } else {
      g.color(1.0);
    }
    if (state().wireFrame) {
      g.polygonLine();
    } else {
      g.polygonFill();
    }
    g.draw(mesh);
    g.popMatrix();
  }

  void onSound(AudioIOData &io) override {

    if (isPrimary()) { // Only primary will produce audio
      float maxInputAmplitude = 0.0f;
      while (io()) {
        // find largest amplitude in block
        if (io.channelsIn() > 0) {
          float in = fabs(io.in(0));
          if (in > maxInputAmplitude) {
            maxInputAmplitude = in;
          }
        }

        float f = (state().p[pokedVertex] - pokedVertexRest).mag() - 0.45;

        if (f > 0.99) {
          f = 0.99;
        } else if (f < 0) {
          f = 0;
        }

        io.out(0) = io.out(1) = pinkNoise() * f * 0.3;
      }

      // poke the blob if the largest amplitude is above some threshold
      if (maxInputAmplitude > 0.707f)
        shouldPoke = true;
    }
  }

  bool onKeyDown(const Keyboard &k) override {
    shouldPoke = true;
    return false;
  }

  bool onMouseDown(const Mouse &m) override {
    shouldPoke = true;
    return false;
  }
};

int main() {
  Blob blob;
  blob.start();
  return 0;
}
