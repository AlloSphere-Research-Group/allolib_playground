

#include "al/app/al_DistributedApp.hpp"
#include "al/io/al_Imgui.hpp"

using namespace al;

// This example shows how to make a GUI with tabs using ParameterMenu.
// Connecing gui tabs across application instances through
// the use of ParameterMenu

struct MyApp : public DistributedApp {
  void onInit() override {
    rank = 0; // Force application to draw regular window
    imguiInit();

    // Set the avaialable tab names
    mTabs.setElements({"first", "second", "third"});

    // Using tabs as a parameter allows keeping the selected tab synchronized
    // across instances of the app
    parameterServer() << mTabs;
  }

  void onAnimate(double dt) override { prepareGui(); }

  void onDraw(Graphics &g) override {
    g.clear(0);
    imguiDraw();
  }

  void onExit() override { imguiShutdown(); }

  int drawTabBar(ParameterMenu &menu) {
    auto tabNames = menu.getElements();

    // Draw tab headers
    ImGui::Columns(tabNames.size(), nullptr, true);
    for (size_t i = 0; i < tabNames.size(); i++) {
      if (ImGui::Selectable(tabNames[i].c_str(), menu.get() == i)) {
        menu.set(i);
      }
      ImGui::NextColumn();
    }
    // Draw tab contents
    ImGui::Columns(1);
    return menu.get();
  }

  void prepareGui() {
    imguiBeginFrame();
    ImGui::SetNextWindowBgAlpha(0.9);

    ImGui::Begin("Tabs example");

    if (ImGui::Button("Add Tab")) {
      auto elements = mTabs.getElements();
      elements.push_back("new " + std::to_string(mTabs.getElements().size()));
      mTabs.setElements(elements);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Tab")) {
      auto elements = mTabs.getElements();
      elements.pop_back();
      mTabs.setElements(elements);
    }
    // Draw tab bar
    int currentTab = drawTabBar(mTabs);
    // Draw tab contents according to currently selected tab
    switch (currentTab) {
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
      ImGui::Text(
          "%s",
          std::string("Dynamic Tab: " + std::to_string(mTabs.get())).c_str());
      break;
    }
    ImGui::End();
    imguiEndFrame();
  }

  ParameterMenu mTabs{"Tabs", "", 0};
};

int main() {
  MyApp().start();
  return 0;
}
