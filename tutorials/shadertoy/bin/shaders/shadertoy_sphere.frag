#version 330 core

// vUV matches addTexSphere skybox layout (u = longitude, v = pole to pole)

uniform float iTime;
//uniform float iTimeDelta;
uniform vec3 iResolution;
uniform vec4 iMouse;
//uniform int iFrame;

in vec2 vUV;
out vec4 fragColor;

vec3 directionFromEquirectUV(vec2 uv) {
  float theta = uv.y * 3.14159265;
  float phi = uv.x * 6.2831853;
  float sinT = sin(theta);
  return normalize(vec3(sin(phi) * sinT, cos(theta), cos(phi) * sinT));
}

void main() {
  vec2 uv = vUV;
  vec3 dir = directionFromEquirectUV(uv);

  vec3 color = vec3(0.0);
  float t = iTime * 0.5;
  color.r = 0.5 + 0.5 * sin(dir.x * 4.0 + t + dir.z * 2.0);
  color.g = 0.5 + 0.5 * sin(dir.y * 5.0 + t * 1.1);
  color.b = 0.5 + 0.5 * cos(dir.z * 3.0 + length(dir.xy) * 2.0 - t);

  vec2 m = iMouse.xy * 2.0 - 1.0;
  m.x *= iResolution.z;
  vec2 dUv = uv * 2.0 - 1.0;
  dUv.x *= iResolution.z;
  float spot = smoothstep(0.25, 0.05, length(dUv - m));
  color += vec3(0.35) * spot;

  fragColor = vec4(color, 1.0);
}
