#include "al/app/al_DistributedApp.hpp"

using namespace al;

static const al::Color backgroundColor(1, 1, 1);
static const al::Color cageColor(RGB(0.3, 0.3, 0.3));

static const double sphereRadius = 10.0;
static const double circleRadius = 1.0;

struct MyApp : DistributedApp {
public:
  MyApp() : DistributedApp() {

    CreateCircles(sphereRadius, circleRadius);

    CreateSphericalCage(1.001 * sphereRadius, mInnerCage, cageColor);
  }

  virtual ~MyApp() {}

  // Stacks are circles cut perpendicular to the z axis while slices are circles
  // cut through the z axis.
  // The top is (0,0,radius) and the bottom is (0,0,-radius).
  int addSphereWithLine(Mesh &m, double radius, int slices, int stacks) {

    struct CSin {
      CSin(double frq, double radius = 1.)
          : r(radius), i(0.), dr(cos(frq)), di(sin(frq)) {}
      void operator()() {
        double r_ = r * dr - i * di;
        i = r * di + i * dr;
        r = r_;
      }
      double r, i, dr, di;
    };

    int Nv = m.vertices().size();

    CSin P(M_PI / stacks);
    P.r = P.dr * radius;
    P.i = P.di * radius;
    CSin T(M_2PI / slices);

    // Add top cap
    // Triangles have one vertex at the north pole and the others on the first
    // ring down.
    m.vertex(0, 0, radius);
    for (int i = 0; i < slices; ++i) {
      m.index(Nv + 1 + i);
      m.index(Nv + 1 + ((i + 1) % slices));
      m.index(Nv); // the north pole
    }

    // Add rings
    for (int j = 0; j < stacks - 2; ++j) {
      int jp1 = j + 1;

      for (int i = 0; i < slices; ++i) {
        int ip1 = (i + 1) % slices;

        int i00 = Nv + 1 + j * slices + i;
        int i10 = Nv + 1 + j * slices + ip1;
        int i01 = Nv + 1 + jp1 * slices + i;
        int i11 = Nv + 1 + jp1 * slices + ip1;

        m.vertex(T.r * P.i, T.i * P.i, P.r);
        m.index(i00);
        m.index(i01);
        m.index(i10);
        m.index(i10);
        m.index(i01);
        m.index(i11);
        T();
      }
      P();
    }

    // Add bottom ring and cap
    int icap = m.vertices().size() + slices;
    for (int i = 0; i < slices; ++i) {
      m.vertex(T.r * P.i, T.i * P.i, P.r);
      m.index(icap - slices + ((i + 1) % slices));
      m.index(icap - slices + i);
      m.index(icap);
      T();
    }
    m.vertex(0, 0, -radius);

    return m.vertices().size() - Nv;
  }

  // equal latitude and longitude
  void CreateSphericalCage(double sphereRadius, Mesh &mesh,
                           al::Color const &color) {
    // slices should be divisible by 4 and stacks by 2
    mesh.primitive(Mesh::LINES);
    addSphereWithLine(mesh, sphereRadius, 128,
                      64); // mesh,sphereRadius,slices,stacks
    mesh.color(color);
    mesh.generateNormals();
  }

  void CreateCircles(double sphereRadius, double circleRadius) {

    {
      // Create the 3 major circles
      const int segments = 256;

      mXZCircle.primitive(Mesh::LINE_LOOP); // equator
      mXYCircle.primitive(Mesh::LINE_LOOP);
      mYZCircle.primitive(Mesh::LINE_LOOP);

      for (int i = 0; i < segments; ++i) {
        double t = 2.0 * M_PI * i / segments;
        mXZCircle.vertex(sphereRadius * sin(t), 0.0, sphereRadius * cos(t));
        mXYCircle.vertex(sphereRadius * cos(t), sphereRadius * sin(t), 0.0);
        mYZCircle.vertex(0.0, sphereRadius * cos(t), sphereRadius * sin(t));
      }

      mXZCircle.generateNormals();
      mXYCircle.generateNormals();
      mYZCircle.generateNormals();
    }

    {
      // Create the 3 minor circles = direction beacons
      // projected directly in front and ahead.  will appear elliptical if space
      // is scaled non-uniformly
      const int segments = 128;

      mBackCircle.primitive(Mesh::LINE_LOOP);
      mTopCircle.primitive(Mesh::LINE_LOOP);
      mRightCircle.primitive(Mesh::LINE_LOOP);

      double rightCircleEnlargeFactor = 1.5;

      for (int i = 0; i < segments; ++i) {
        double t = 2.0 * M_PI * i / segments;

        mBackCircle.vertex(
            circleRadius * cos(t), circleRadius * sin(t),
            sqrt(sphereRadius * sphereRadius - circleRadius * circleRadius));
        mTopCircle.vertex(
            circleRadius * cos(t),
            sqrt(sphereRadius * sphereRadius - circleRadius * circleRadius),
            circleRadius * sin(t));

        // Make Right Circle bigger - doorway blocks image
        mRightCircle.vertex(
            sqrt(sphereRadius * sphereRadius - circleRadius * circleRadius *
                                                   rightCircleEnlargeFactor *
                                                   rightCircleEnlargeFactor),
            rightCircleEnlargeFactor * circleRadius * sin(t),
            rightCircleEnlargeFactor * circleRadius * cos(t));
      }

      mRightCircle.color(RGB(1, 0, 0)); // In positive X = R color
      mTopCircle.color(RGB(0, 1, 0));   // In positive Y = G color
      mBackCircle.color(RGB(0, 0, 1));  // In positive Z = B color

      mRightCircle.generateNormals();
      mTopCircle.generateNormals();
      mBackCircle.generateNormals();
    }
  }

  virtual void onDraw(Graphics &g) {
    g.clear(backgroundColor);
    g.polygonLine();
    g.meshColor();
    g.draw(mBackCircle);

    g.draw(mTopCircle);
    g.draw(mRightCircle);
    g.draw(mInnerCage);

    g.color(1, 0, 0);
    g.draw(mXZCircle);
    g.color(0, 1, 0);
    g.draw(mXYCircle);
    g.color(0, 0, 1);
    g.draw(mYZCircle);
  }

  //  virtual void onAnimate(al_sec dt) { pose = nav(); }

private:
  Mesh mXZCircle, mXYCircle, mYZCircle; // major circles
  Mesh mBackCircle, mTopCircle, mRightCircle;

  Mesh mInnerCage;
  Mesh mOuterCage;

  Light mLight;
};

int main(int argc, char *argv[]) {
  MyApp().start();
  return 0;
}
