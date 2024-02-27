#version 330 core

in vec2 vuv;
out vec4 fragColor;

uniform vec2 size;
uniform vec2 brush;
uniform sampler2D tex;
uniform float dx;
uniform float dy;

void main() {
  float ddx = 1.0/size.x * dx;
  float ddy = 1.0/size.y * dy;
  float dt = 0.1;

  // vec2 alpha = vec2(da/(dx*dx), db/(dy*dy));
  vec2 alpha = vec2(0.2097, 0.105);


  vec2 V = texture(tex, vuv).rg;
  vec4 L = texture(tex, vuv + vec2(0,-ddy))
    +  texture(tex, vuv + vec2(-ddx,0)) 
    -  4.0 * texture(tex,  vuv )
    +  texture(tex, vuv + vec2(ddx,0)) 
    +  texture(tex, vuv + vec2(0,ddy));

  V += L.rg * alpha * dt;


  // float F = 0.034; //mitosis
  // float K = 0.063;
  float F = 0.025; //pulse
  float K = 0.06;
  // float F = 0.014; //waves
  // float K = 0.045;
  // float F = 0.026; //brains
  // float K = 0.055;
  // float F = 0.082; //worms
  // float K = 0.061;
  // float F = 0.082; //worm channels
  // float K = 0.059;
  
  // grey scott
  float ABB = V.r*V.g*V.g;
  float rA = -ABB + F*(1.0 - V.r);
  float rB = ABB - (F+K)*V.g;

  vec2 R = vec2(rA,rB) * dt;

  // output diffusion + reaction
  vec2 RD = V + R;

  // mouse input
  if(brush.x > 0.0){
    vec2 brsh = brush;
    //brsh.y = 1.0 - brsh.y;
    vec2 diff = (vuv - brsh)/vec2(ddx,ddy);
    float dist = dot(diff, diff);
    if(dist < 100.0){
        RD.r = 0.0;
        RD.g = 0.9;
    }
  }
  
  fragColor = vec4(RD, 0.0, 1.0);
}