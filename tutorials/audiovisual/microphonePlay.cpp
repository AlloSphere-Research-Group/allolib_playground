// AlloLib Example 
// Microphone input to Waveform & Spectrum 
// Author: Myungin Lee 2022

#include <memory>
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "Gamma/DFT.h"

using namespace al;
using namespace std;
#define FFT_SIZE 1024
#define BLOCK_SIZE 512
#define CHANNEL_COUNT 2
#define SAMPLE_RATE 48000.0f

struct MyApp : public App
{
  // STFT variables
  // Window size
  // Hop size; number of samples between transforms
  // Pad size; number of zero-valued samples appended to window
  // Window type: HAMMING, HANN, WELCH, NYQUIST, or etc
  // Format of frequency samples: COMPLEX, MAG_PHASE, or MAG_FREQ
  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);
  Mesh mSpectrogram;
  vector<float> spectrum;
  float i_waveformData[BLOCK_SIZE * CHANNEL_COUNT]{0}; // Waveform variables
  float o_waveformData[BLOCK_SIZE * CHANNEL_COUNT]{0}; // Waveform variables
  Mesh i_waveformMesh[2]{Mesh::LINE_STRIP, Mesh::LINE_STRIP};
  Mesh o_waveformMesh[2]{Mesh::POINTS, Mesh::POINTS};

  void onInit() override
  {
    if (audioIO().channelsIn() == 0)
    {
      cout << "**** ERROR: Could not open audio input. Quitting."
              << endl;
      quit();
      return;
    }
    // Declare the size of the spectrum
    spectrum.resize(FFT_SIZE / 2 + 1);
    mSpectrogram.primitive(Mesh::LINE_STRIP);
    nav().pos(Vec3f(0, 0, 0));
  }

  void onAnimate(double dt)
  {
    // Waveform i/o
    for(int ch = 0; ch < CHANNEL_COUNT; ch++) {
      i_waveformMesh[ch].reset();
      o_waveformMesh[ch].reset();
      for(int i = 0; i < BLOCK_SIZE; i++) {
        float x = i / float(BLOCK_SIZE);
        float iy = (i_waveformData[ch * BLOCK_SIZE + i]);
        float oy = (o_waveformData[ch * BLOCK_SIZE + i]);
        i_waveformMesh[ch].vertex(10 * x, 100 * iy, 0);
        i_waveformMesh[ch].color(HSV(iy*10, 1., 1.));
        o_waveformMesh[ch].vertex(10 * x, 10 * oy, 0);
        o_waveformMesh[ch].color(HSV(al::rnd::uniform(oy*1000), 1., 1.));
      }
    }
    // Spectrogram
    mSpectrogram.reset();
    for (int i = 0; i < FFT_SIZE / 2; i++)
    {
      mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
      mSpectrogram.vertex(i, spectrum[i], 0.0);
    }
  }
  void onSound(AudioIOData &io) override
  {
    while (io())
    {
      if (stft(io.in(0)))
      { // Loop through all the frequency bins
        for (unsigned k = 0; k < stft.numBins(); ++k)
        {
          // Here we simply scale the complex sample
          spectrum[k] = stft.bin(k).real();
        }
      }
      // // Process the outputs - Randomized
      io.out(0) = al::rnd::uniform(io.in(0)*10);
      io.out(1) = al::rnd::uniform(io.in(1)*10);
    }
    memcpy(&i_waveformData, io.inBuffer(), BLOCK_SIZE * CHANNEL_COUNT * sizeof(float));
    memcpy(&o_waveformData, io.outBuffer(), BLOCK_SIZE * CHANNEL_COUNT * sizeof(float));
  }
  // The graphics callback function.
  void onDraw(Graphics &g) override
  {
    g.clear();
    // Draw Spectrum
    g.meshColor(); // Use the color in the mesh
    g.pushMatrix();
    g.translate(-5, 0, -20);
    g.scale(100.0 / FFT_SIZE, 1000, 1.0);
    g.draw(mSpectrogram);
    g.popMatrix();
    // Input Waveform
    for(int ch = 0; ch < CHANNEL_COUNT; ch++) { 
      g.pushMatrix();
      g.translate(-5, 3 * (ch+1), -20);
      g.draw(i_waveformMesh[ch]);
      g.popMatrix();
    }
    // Output Waveform
    for(int ch = 0; ch < CHANNEL_COUNT; ch++) { 
      g.pushMatrix();
      g.pointSize(3);
      g.translate(-5, - 3 * (ch+1), -20);
      g.draw(o_waveformMesh[ch]);
      g.popMatrix();
    }    
  }
};

int main(int argc, char *argv[])
{
  MyApp app;
  app.start();
  return 0;
}
