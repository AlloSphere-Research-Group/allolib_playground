#include "al/app/al_App.hpp"
#include "al/io/al_Imgui.hpp"
using namespace al;

using std::cout;
using std::endl;
using std::vector;

#include "libtcc.h"

const char* starterCode = R"(
double tanh(double);
double sin(double);
double exp(double);
double modf(double, double*);
double fmod(double, double);

double function(double x) {
  return 0.3;
  //return x;
  //return tanh(6 * x);
  //return x += .4, x *= 5, exp(-x * x);
  //return x *= 100, sin(x) / x;
  //return x += 1, sin(220 * x) * exp(-x * 5);
  //return x *= 2, x * x * x - x * x - x / 10;
  //return x + sin(x * 10) / 10;
  //return modf(3 * x, &x);
  //return fmod(x, .3333);
}
)";

void tcc_error_handler(void* tcc, const char* msg);

struct TCC {
  using FunctionPointer = double (*)(double);
  FunctionPointer function = nullptr;
  TCCState* instance = nullptr;
  std::string error;

  bool compile(std::string source) {
    if (instance) tcc_delete(instance);
    instance = tcc_new();
    assert(instance != nullptr);

    tcc_set_options(instance, "-nostdinc -nostdlib -Wall -Werror");
    tcc_set_error_func(instance, this, tcc_error_handler);
    tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);

    if (tcc_compile_string(instance, source.c_str()) == -1)  //
      return false;

    if (tcc_relocate(instance, TCC_RELOCATE_AUTO) < 0) {
      error = "failed to relocate code";
      return false;
    }

    FunctionPointer foo =
        (FunctionPointer)(tcc_get_symbol(instance, "function"));
    if (foo == nullptr) {
      error = "could not find the symbol 'foo'";
      return false;
    }

    error = "";
    function = foo;
    return true;
  }

  double operator()(double x) {
    if (function == nullptr) return 0;
    return function(x);
  }
};

void tcc_error_handler(void* tcc, const char* msg) { ((TCC*)tcc)->error = msg; }

const int N = 2000;

struct Appp : App {
  TCC tcc;
  char buffer[10000];
  char error[10000];
  Mesh mesh;

  Appp() { strcpy(buffer, starterCode); }
  void onExit() override { imguiShutdown(); }
  void onInit() override { imguiInit(); }

  void onCreate() override {
    mesh.primitive(Mesh::LINE_STRIP);
    for (int i = 0; i < N; i++) {
      float x = 2.0f * i / (N - 1) - 1;
      mesh.vertex(x, 0, 0);
    }

    if (tcc.compile(buffer)) {
      vector<Vec3f>& vertex(mesh.vertices());
      for (int i = 0; i < N; i++) {
        vertex[i].y = tcc(vertex[i].x);
      }
    }
  }

  void onAnimate(double dt) override {
    imguiBeginFrame();

    ImGui::Text(tcc.error.c_str());
    ImGui::Separator();

    bool update =
        ImGui::InputTextMultiline("", buffer, sizeof(buffer), ImVec2(420, 330));

    if (update) {
      if (tcc.compile(buffer)) {
        vector<Vec3f>& vertex(mesh.vertices());
        for (int i = 0; i < N; i++)  //
          vertex[i].y = tcc(vertex[i].x);
      }
    }

    imguiEndFrame();
  }

  void onDraw(Graphics& g) override {
    g.clear(Color(0.1));
    g.camera(Viewpoint::IDENTITY);
    g.draw(mesh);
    imguiDraw();
  }
};

int main() { Appp().start(); }
