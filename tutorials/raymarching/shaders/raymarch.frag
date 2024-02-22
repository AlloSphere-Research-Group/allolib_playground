#version 330

// uniform sampler3D tex_voxels;
uniform vec3 box_min, box_max;
uniform int max_steps;
uniform float step_size;
uniform float translucent;

uniform float time;

in vec3 ray_dir, ray_origin;

layout (location = 0) out vec4 frag_out0;

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rayBoxIntersect(const vec3 b_min, const vec3 b_max, const vec3 r_o, const vec3 r_d) {
  vec3 inv_dir = 1.0 / r_d;

  vec3 tbot = inv_dir * (b_min - r_o);
  vec3 ttop = inv_dir * (b_max - r_o);
  
  vec3 tmin = min(ttop, tbot);
  vec3 tmax = max(ttop, tbot);

  vec2 traverse = max(tmin.xx, tmin.yz);
  float traverse_low = max(traverse.x, traverse.y);
  
  traverse = min(tmax.xx, tmax.yz);
  float traverse_high = min(traverse.x, traverse.y);

  return vec3(float(traverse_high > max(traverse_low, 0.0)), traverse_low, traverse_high);
}

// distance to sphere surface
float sphereSDF(vec3 center, float radius, vec3 toPoint){
  return length(center - toPoint) - radius;
}

// distance to box surface
float boxSDF(vec3 center, vec3 size, vec3 toPoint) {
  vec3 d = abs(center - toPoint) - size;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

// distance to scene surface
float distfield(vec3 p){

  float d1 = sphereSDF(vec3(-0.25,0,0), 0.25, p);
  float d2 = sphereSDF(vec3(0.25,0,0), 0.25, p);

  float d = min(d1,d2); // min acts as union of surfaces
  return d;
}


void main() {
  vec3 ro = ray_origin;
  vec3 rd = ray_dir;

  vec3 slice_min = box_min;
  vec3 slice_max = box_max;
  
  vec3 boxHit = rayBoxIntersect(slice_min, slice_max, ro, rd);

  vec4 rayColor = vec4(0);

  if (boxHit.x > 0) {
    float dist = boxHit.y;
    float dist_max = boxHit.z;

    vec3 box_inverse = 1.0 / box_max;
    float sliceMultiplier = 1.;


    for (int i = 0; i < max_steps && dist < dist_max && rayColor.a < 1.; ++i) {
      vec3 ray_pos = ro + rd * dist;
      ray_pos *= box_inverse;

      // float amt = texture(tex_voxels, ray_pos).r;
      float d = distfield(ray_pos); // distance to surface within field

      float amt = 1.0 - smoothstep(0, 0.5, d);
      float alpha = amt;

      vec3 rgb = hsv2rgb(vec3(amt, 1, amt));


      rayColor.rgb += (1 - translucent + amt) * rgb * step_size;
      rayColor.a += (1 - translucent + amt) * alpha * step_size;
      
      dist += step_size;      
    }
  }


  frag_out0 = rayColor;
}
