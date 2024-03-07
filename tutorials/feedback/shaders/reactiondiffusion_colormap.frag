#version 330 core

in vec2 vuv;
out vec4 fragColor;

uniform sampler2D tex;

void main() {
  vec4 value = texture(tex, vuv);
  float v = value.g;
  float a = 0.0;
  vec3 col = vec3(0.0,0.0,0.0);

  vec4 color1 = vec4(0.0,0.0,0.0,0.0);
  vec4 color2 = vec4(1.0,1.0,1.0,0.3);
  vec4 color3 = vec4(0.0,1.0,1.0,0.35);
  vec4 color4 = vec4(0.0,0.0,1.0,0.5);
  vec4 color5 = vec4(0.0,0.0,0.0,0.6);

  if(v <= color1.a){
    col = color1.rgb;
  } else if(v <= color2.a){
    a = (v - color1.a)/(color2.a - color1.a);
    col = mix(color1.rgb, color2.rgb, a);
  } else if(v <= color3.a){
    a = (v - color2.a)/(color3.a - color2.a);
    col = mix(color2.rgb, color3.rgb, a);
  } else if(v <= color4.a){
    a = (v - color3.a)/(color4.a - color3.a);
    col = mix(color3.rgb, color4.rgb, a);
  } else if(v <= color5.a){
    a = (v - color4.a)/(color5.a - color4.a);
    col = mix(color4.rgb, color5.rgb, a);
  } else {
    col = color5.rgb;
  }

  fragColor = vec4(col, 1.0);
}