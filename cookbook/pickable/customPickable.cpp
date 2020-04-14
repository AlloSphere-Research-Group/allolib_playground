/*
 Custom Pickable

Description:
This example demonstrates how to create a custom pickable that inherits from PickableBB for its
default functionality and adds a custom child Pickable to interact with an internal dataset

Author:
Timothy Wood, April 2020
*/

#include "al/app/al_App.hpp"

#include "al/graphics/al_Shapes.hpp"

#include "al/math/al_Ray.hpp"
#include "al/ui/al_BoundingBox.hpp"
#include "al/ui/al_Pickable.hpp"
#include "al/ui/al_PickableManager.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

// inherit from PickableBB
struct CustomPickableBB : PickableBB {

  std::vector<Vec3f> data;
  Mesh mesh;
  const float meshSize{ 0.02f };
  int hoverIndex, selectIndex;

  // Child pickable to interact with some data
  struct DataPickable : Pickable {
    CustomPickableBB *p;
    
    DataPickable(CustomPickableBB *p_) : p(p_){};
    
    Hit intersect(Rayd r){
      auto ray = transformRayLocal(r);
      double minT = std::numeric_limits<double>::max();
      int minIndex = -1;
      for (int i = 0; i < p->data.size(); i++) {
        Vec3f& v = p->data[i];
        auto t = ray.intersectSphere(v, p->meshSize);
        if (t < minT && t > 0) {
          minT = t;
          minIndex = i;
        }
      }
      if (minIndex != -1) {
        return Hit(true, r, minIndex, this);
      } else
        return Hit(false, r, 0, this);
    };
    
    bool onEvent(PickEvent e, Hit h){
      switch (e.type) {
        case Point:
          if (h.hit) p->hoverIndex = (int)h.t;
          return h.hit;

        case Pick:
          if(h.hit) p->selectIndex = (int)h.t;
          return false;

        default:
          return false;
      }
    };

  } datasetPickable{this};

  CustomPickableBB(){ 
    bb.set(Vec3f(0), Vec3f(1));  //initialize bounding box for pickableBB
    addChild(datasetPickable); // add our DataSetPickable as child of this
    
    addSphere(mesh, meshSize); // initialize mesh we will use to draw data points

    // generate random data points
    for(int i=0; i < 100; i++){
      data.push_back(Vec3f(rnd::uniform(), rnd::uniform(), rnd::uniform()));
    }
  }

  void draw(Graphics& g){
    g.lighting(false);
    drawBB(g);

    // draw each data point as sphere, change color if hover or selected
    for(int i=0; i < data.size(); i++){
      pushMatrix(g);
      g.translate(data[i]);
      g.color(0.75);
      if(i == hoverIndex) g.color(0,1,1);
      if(i == selectIndex) g.color(1,0,0);
      g.draw(mesh);
      popMatrix(g);
    }
  }
};

struct MyApp : public App {

  CustomPickableBB pickable;
  PickableManager pm;

  void onCreate() override {
    nav().pos(0.5, 0.5, 4);
    pm << pickable;
  }

  void onAnimate(double dt) override {}

  void onDraw(Graphics &g) override {
    g.clear(0);
    gl::depthTesting(true);

    pickable.draw(g);
  }


  virtual bool onMouseMove(const Mouse &m) override {
    pm.onMouseMove(graphics(), m, width(), height());
    return true;
  }
  virtual bool onMouseDown(const Mouse &m) override {
    pm.onMouseDown(graphics(), m, width(), height());
    return true;
  }
  virtual bool onMouseDrag(const Mouse &m) override {
    pm.onMouseDrag(graphics(), m, width(), height());
    return true;
  }
  virtual bool onMouseUp(const Mouse &m) override {
    pm.onMouseUp(graphics(), m, width(), height());
    return true;
  }
};

int main() { MyApp().start(); }
