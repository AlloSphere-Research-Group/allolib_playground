

#include "al/app/al_DistributedApp.hpp"
#include "al/io/al_Imgui.hpp"

using namespace al;

// This example shows connecing gui tabs across application instances through
// the use of ParameterMenu.

struct MyApp : public DistributedApp {
  void onInit() override {
    rank = 0;  // Force application to draw regular window
    imguiInit();

    // Set the avaialable tab names
    mTabs.setElements({"first", "second", "third"});

    // Using tabs as a parameter allows keeping the selected tab synchronized
    // across instances of the app
    parameterServer() << mTabs;
  }

  void onAnimate(double dt) override { prepareGui(); }

  void onDraw(Graphics &g) {
    g.clear(0);
    imguiDraw();
  }

  void onExit() { imguiShutdown(); }

  void prepareGui() {
    imguiBeginFrame();
    ImGui::SetNextWindowBgAlpha(0.9);

    ImGui::Begin("Tabs example");
    auto tabNames = mTabs.getElements();

    // Draw tab headers
    ImGui::Columns(tabNames.size(), nullptr, true);
    for (size_t i = 0; i < tabNames.size(); i++) {
      if (ImGui::Selectable(tabNames[i].c_str(), mTabs.get() == i)) {
        mTabs.set(i);
      }
      ImGui::NextColumn();
    }
    // Draw tab contents
    ImGui::Columns(1);
    switch (mTabs.get()) {
      case 0:
        ImGui::Text("First tab");
        break;
      case 1:
        ImGui::Text("Second tab");
        break;
      case 2:
        ImGui::Text("Third tab");
        break;
      default:
        break;
    }
    ImGui::End();
    imguiEndFrame();
  }

  ParameterMenu mTabs{"Tabs", "", 0, ""};
};

int main() {
  MyApp().start();
  return 0;
}
