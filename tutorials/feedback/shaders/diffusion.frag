#version 330 core

in vec2 vuv;
layout (location = 0) out vec4 fragColor;
uniform sampler2D tex;
uniform vec2 size;
uniform float dx;
uniform float dy;

void main() {
  float ddx = 1.0/size.x * dx;
  float ddy = 1.0/size.y * dy;

  vec4 color = texture(tex, vuv);
  vec4 L = texture(tex, vuv + vec2(0,-ddy))
    +  texture(tex, vuv + vec2(-ddx,0)) 
    -  4.0 * texture(tex,  vuv )
    +  texture(tex, vuv + vec2(ddx,0)) 
    +  texture(tex, vuv + vec2(0,ddy));

  color += L * 0.1;
  fragColor = vec4(color);
}