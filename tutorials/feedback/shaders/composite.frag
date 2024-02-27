#version 330 core

in vec2 vuv;
layout (location = 0) out vec4 fragColor;

uniform sampler2D tex0;
// uniform sampler2D tex1;

uniform float blend0;
uniform float blend1;

void main(){

  vec4 color0 = texture(tex0, vuv) * blend0;
  // vec4 color1 = texture(tex1, vuv) * blend1;

  fragColor = color0; // + color1;

}

