#include "al/app/al_App.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/io/al_Toml.hpp"
#include "al/sound/al_SpeakerAdjustment.hpp"
#include "al/sphere/al_AlloSphereSpeakerLayout.hpp"
#include "al/sphere/al_SphereUtils.hpp"
#include "al/ui/al_FileSelector.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al_ext/soundfile/al_SoundfileBuffered.hpp"

// From: https://github.com/hideakitai/MTCParser
// Under MIT license
//    Copyright (c) 2018 Hideaki Tai

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include "MTCParser.h"

using namespace al;

class MTCReceiver : public MIDIMessageHandler {
public:
  MTCParser mtc;
  uint8_t hour{0};
  uint8_t minute{0};
  uint8_t second{0};
  uint8_t frame{0};
  virtual ~MTCReceiver() {}

  /// Called when a MIDI message is received
  virtual void onMIDIMessage(const MIDIMessage &m) {
    if (m.type() == MIDIByte::SYSTEM_MSG && m.status() == MIDIByte::TIME_CODE) {
      mtc.feed(m.bytes, m.dataSize());
      if (mtc.available()) {
        hour = mtc.hour();
        minute = mtc.minute();
        second = mtc.second();
        frame = mtc.frame();
        mtc.pop();
      }
    }
  };

  /// Bind handler to a MIDI input
  //    void bindTo(RtMidiIn &RtMidiIn, unsigned port = 0);
};

class MTCApp : public App {
public:
  RtMidiIn midiIn;
  MTCReceiver mtcReceiver;
  ParameterMenu TCframes{"TC_fps"};
  ParameterInt frameOffset{"frame_offset", "", 0, -25, 25};
  std::vector<float> frameValues = {24, 25, 30, 30};
  // App callbacks
  void onInit() override {
    mtcReceiver.bindTo(midiIn);
    midiIn.ignoreTypes(false, false, false);
    std::vector<std::string> fps = {"24", "25", "29.97", "30"};

    TCframes.setElements(fps);
  }

  void onCreate() override { imguiInit(); }
  void onDraw(Graphics &g) override {
    imguiBeginFrame();

    ImGui::Begin("MIDI Time Code");
    ParameterGUI::draw(&TCframes);
    ParameterGUI::draw(&frameOffset);
    ParameterGUI::drawMIDIIn(&midiIn);

    ImGui::Text("%02i:%02i:%02i:%02i", mtcReceiver.hour, mtcReceiver.minute,
                mtcReceiver.second, mtcReceiver.frame);
    int fps = frameValues[TCframes.get()];
    int frameNum = mtcReceiver.hour * 60 * 60 * fps +
                   mtcReceiver.minute * 60 * fps + mtcReceiver.second * fps +
                   mtcReceiver.frame;
    ImGui::Text("Frame num : %i", frameNum);

    ImGui::End();
    imguiEndFrame();
    g.clear(0, 0, 0);
    imguiDraw();
  }

  //  void onSound(AudioIOData &io) override {}

  void onExit() override { imguiShutdown(); }

private:
};

int main() {
  MTCApp app;

  app.start();
  return 0;
}
