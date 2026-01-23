#version 330 core

// Shadertoy.com-style uniforms
uniform float iTime;           // Time since start
uniform float iTimeDelta;       // Time since last frame
uniform vec3 iResolution;       // Viewport resolution (width, height, aspect)
uniform vec4 iMouse;            // Mouse position and click (x, y, clickX, clickY)
uniform int iFrame;             // Frame number

in vec2 fragCoord;
out vec4 fragColor;

void main() {
  // Normalize coordinates to [0, 1] range
  vec2 uv = fragCoord;
  
  // Center coordinates around origin
  vec2 p = (uv - 0.5) * 2.0;
  p.x *= iResolution.z; // Adjust for aspect ratio
  
  // Example: Animated gradient with mouse interaction
  vec3 color = vec3(0.0);
  
  // Create a rotating color pattern
  float angle = iTime * 0.5;
  vec2 rotated = vec2(
    p.x * cos(angle) - p.y * sin(angle),
    p.x * sin(angle) + p.y * cos(angle)
  );
  
  // Base color based on position
  color.r = 0.5 + 0.5 * sin(rotated.x * 3.0 + iTime);
  color.g = 0.5 + 0.5 * sin(rotated.y * 3.0 + iTime + 2.0);
  color.b = 0.5 + 0.5 * sin(length(p) * 5.0 - iTime * 2.0);
  
  // Add mouse interaction - create a circle at mouse position
  vec2 mouseUV = iMouse.xy;
  vec2 mouseP = (mouseUV - 0.5) * 2.0;
  mouseP.x *= iResolution.z;
  
  float dist = length(p - mouseP);
  float circle = smoothstep(0.3, 0.25, dist);
  color += vec3(circle * 0.5);
  
  // Add click effect
  if (iMouse.z > 0.0 || iMouse.w > 0.0) {
    vec2 clickP = (iMouse.zw - 0.5) * 2.0;
    clickP.x *= iResolution.z;
    float clickDist = length(p - clickP);
    float clickCircle = smoothstep(0.2, 0.0, clickDist);
    color += vec3(clickCircle);
  }
  
  fragColor = vec4(color, 1.0);
}
