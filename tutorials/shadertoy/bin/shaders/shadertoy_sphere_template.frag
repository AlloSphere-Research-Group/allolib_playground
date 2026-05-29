#version 330 core

// Equirect UV template (matches addTexSphere skybox):
//   u: longitude 0..1  -> 0..360 deg (east), v: 0 = north pole, 1 = south pole

uniform float iTime;
uniform vec3 iResolution;
uniform vec4 iMouse;

in vec2 vUV;
out vec4 fragColor;

const float PI = 3.14159265;
const float DEG10 = 10.0;
const float DEG30 = 30.0;

vec3 directionFromEquirectUV(vec2 uv) {
  float theta = uv.y * PI;
  float phi = uv.x * 2.0 * PI;
  float sinT = sin(theta);
  return normalize(vec3(sin(phi) * sinT, cos(theta), cos(phi) * sinT));
}

vec3 hsv2rgb(vec3 c) {
  vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
  return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}

// 1 on line center, 0 off; coord in degrees, spacing in degrees
float degreeLine(float coordDeg, float spacingDeg, float lineWidthDeg) {
  float f = fract(coordDeg / spacingDeg);
  f = min(f, 1.0 - f) * spacingDeg;
  float w = lineWidthDeg * fwidth(coordDeg);
  return 1.0 - smoothstep(0.0, w, f);
}

// Dot marker when both lon/lat land on spacing (within small window)
float degreeMarker(float lonDeg, float latDeg, float spacingDeg, float radiusDeg) {
  float fl = fract(lonDeg / spacingDeg);
  float ft = fract((latDeg + 90.0) / spacingDeg);
  fl = min(fl, 1.0 - fl) * spacingDeg;
  ft = min(ft, 1.0 - ft) * spacingDeg;
  float dl = fl / fwidth(lonDeg);
  float dt = ft / fwidth(latDeg);
  float d = sqrt(dl * dl + dt * dt);
  float r = radiusDeg / spacingDeg * 8.0;
  return 1.0 - smoothstep(r, r + 1.5, d);
}

void main() {
  vec2 uv = vUV;
  float lonDeg = uv.x * 360.0;           // 0..360, u=0 prime meridian
  float latDeg = 90.0 - uv.y * 180.0;  // +90 north .. -90 south

  vec3 dir = directionFromEquirectUV(uv);

  // Slowly morphing test pattern (direction + time)
  float t = iTime * 0.12;
  float hue = fract(atan(dir.z, dir.x) / (2.0 * PI) + 0.5 + t * 0.15);
  float sat = 0.45 + 0.35 * sin(t * 1.7 + dir.y * 4.0 + length(dir.xz) * 2.0);
  float val = 0.35 + 0.25 * cos(t * 2.3 + dir.x * 3.0 - dir.z * 2.0);
  vec3 base = hsv2rgb(vec3(hue, clamp(sat, 0.2, 1.0), clamp(val, 0.15, 0.85)));

  // Checker tint every 30 deg cells (subtle)
  float cellLon = floor(lonDeg / DEG30);
  float cellLat = floor((latDeg + 90.0) / DEG30);
  float checker = mod(cellLon + cellLat, 2.0);
  base *= 0.92 + 0.08 * checker;

  // Latitude / longitude lines (equirect = straight in UV)
  float line10 =
      max(degreeLine(lonDeg, DEG10, 0.35), degreeLine(latDeg + 90.0, DEG10, 0.35));
  float line30 =
      max(degreeLine(lonDeg, DEG30, 0.55), degreeLine(latDeg + 90.0, DEG30, 0.55));

  vec3 lineCol10 = vec3(0.85, 0.9, 0.95);
  vec3 lineCol30 = vec3(1.0, 0.95, 0.75);

  // Prime meridian (lon=0/360) and equator (lat=0)
  float prime = degreeLine(lonDeg, 360.0, 0.9) + degreeLine(lonDeg, 360.0, 0.9);
  prime = max(prime, degreeLine(abs(latDeg), 180.0, 0.9));
  vec3 primeCol = vec3(1.0, 0.35, 0.35);
  vec3 equatorCol = vec3(0.35, 1.0, 0.45);

  float equator = degreeLine(abs(latDeg), 180.0, 0.7);

  // Poles (singular in equirect — highlight caps)
  float poleBand = smoothstep(3.0, 0.0, abs(latDeg - 90.0)) +
                   smoothstep(3.0, 0.0, abs(latDeg + 90.0));
  vec3 poleCol = vec3(0.5, 0.7, 1.0);

  // Degree markers every 30° (except too close to poles)
  float markers = 0.0;
  if (abs(latDeg) < 85.0) {
    markers = degreeMarker(lonDeg, latDeg, DEG30, 2.2);
  }
  vec3 markerCol = vec3(1.0, 1.0, 1.0);

  vec3 color = base;
  color = mix(color, lineCol10, line10 * 0.75);
  color = mix(color, lineCol30, line30 * 0.9);
  color = mix(color, equatorCol, equator * 0.85);
  color = mix(color, primeCol, prime * 0.5);
  color = mix(color, poleCol, poleBand * 0.4);
  color = mix(color, markerCol, markers * 0.95);

  // Mouse: highlight nearest 10° intersection (debug reticle)
  float mLon = iMouse.x * 360.0;
  float mLat = 90.0 - iMouse.y * 180.0;
  float snapLon = round(mLon / DEG10) * DEG10;
  float snapLat = round(mLat / DEG10) * DEG10;
  float reticle = exp(-pow((lonDeg - snapLon) / 4.0, 2.0)) *
                  exp(-pow((latDeg - snapLat) / 4.0, 2.0));
  color += vec3(1.0, 0.9, 0.2) * reticle * 0.35;

  fragColor = vec4(color, 1.0);
}
