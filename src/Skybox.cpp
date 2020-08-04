#include "Skybox.hpp"
#include <array>
#include <cmath>
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>

static const std::string SKYBOX_STARS_FRAG = R"(#version 330 core
out vec4 fragmentColor;

in float v_brightness;
in vec4 v_color;
in vec2 v_coords;

void main() {
    float dist = pow(clamp(1.0 - length(v_coords), 0.0, 1.0), 0.5);
    fragmentColor = v_color * dist * v_brightness;
}
)";

static const std::string SKYBOX_STARS_GEOM = R"(#version 330 core
layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

in float g_brightness[];
in vec4 g_color[];

out vec2 v_coords;
out float v_brightness;
out vec4 v_color;

uniform mat4 projectionMatrix;
uniform vec2 particleSize;

void main (void) {
    vec4 P = gl_in[0].gl_Position;

    v_brightness = g_brightness[0];
    v_color = g_color[0];

    // a: left-bottom 
    vec2 va = P.xy + vec2(-1.0, -1.0) * particleSize;
    gl_Position = projectionMatrix * vec4(va, P.zw);
    v_coords = vec2(-1.0, -1.0);
    EmitVertex();  
    
    // b: left-top
    vec2 vb = P.xy + vec2(-1.0, 1.0) * particleSize;
    gl_Position = projectionMatrix * vec4(vb, P.zw);
    v_coords = vec2(-1.0, 1.0);
    EmitVertex();  
    
    // d: right-bottom
    vec2 vd = P.xy + vec2(1.0, -1.0) * particleSize;
    gl_Position = projectionMatrix * vec4(vd, P.zw);
    v_coords = vec2(1.0, -1.0);
    EmitVertex();  

    // c: right-top
    vec2 vc = P.xy + vec2(1.0, 1.0) * particleSize;
    gl_Position = projectionMatrix * vec4(vc, P.zw);
    v_coords = vec2(1.0, 1.0);
    EmitVertex();  

    EndPrimitive();  
}   
)";

static const std::string SKYBOX_STARS_VERT = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in float brightness;
layout(location = 2) in vec4 color;

uniform mat4 viewMatrix;

out float g_brightness;
out vec4 g_color;

void main() {
    g_brightness = brightness;
    g_color = color;
    gl_Position = viewMatrix * vec4(position, 1.0);
}
)";

// The following shader is based on space-3d shader by wwwtyro from https://github.com/wwwtyro/space-3d
static const std::string SKYBOX_NEBULA_FRAG = R"(#version 330 core
// Source: https://github.com/wwwtyro/space-3d/blob/gh-pages/src/glsl/nebula.glsl
// created by: github.com/wwwtyro
// edited by: github.com/matusnovak

uniform vec4 uColor;
uniform vec3 uOffset;
uniform float uScale;
uniform float uIntensity;
uniform float uFalloff;

in vec3 v_position;

out vec4 fragmentColor;

//
// GLSL textureless classic 4D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-08-22
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/ashima/webgl-noise
//

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 fade(vec4 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec4 P)
{
  vec4 Pi0 = floor(P); // Integer part for indexing
  vec4 Pi1 = Pi0 + 1.0; // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec4 Pf0 = fract(P); // Fractional part for interpolation
  vec4 Pf1 = Pf0 - 1.0; // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = vec4(Pi0.zzzz);
  vec4 iz1 = vec4(Pi1.zzzz);
  vec4 iw0 = vec4(Pi0.wwww);
  vec4 iw1 = vec4(Pi1.wwww);

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);
  vec4 ixy00 = permute(ixy0 + iw0);
  vec4 ixy01 = permute(ixy0 + iw1);
  vec4 ixy10 = permute(ixy1 + iw0);
  vec4 ixy11 = permute(ixy1 + iw1);

  vec4 gx00 = ixy00 * (1.0 / 7.0);
  vec4 gy00 = floor(gx00) * (1.0 / 7.0);
  vec4 gz00 = floor(gy00) * (1.0 / 6.0);
  gx00 = fract(gx00) - 0.5;
  gy00 = fract(gy00) - 0.5;
  gz00 = fract(gz00) - 0.5;
  vec4 gw00 = vec4(0.75) - abs(gx00) - abs(gy00) - abs(gz00);
  vec4 sw00 = step(gw00, vec4(0.0));
  gx00 -= sw00 * (step(0.0, gx00) - 0.5);
  gy00 -= sw00 * (step(0.0, gy00) - 0.5);

  vec4 gx01 = ixy01 * (1.0 / 7.0);
  vec4 gy01 = floor(gx01) * (1.0 / 7.0);
  vec4 gz01 = floor(gy01) * (1.0 / 6.0);
  gx01 = fract(gx01) - 0.5;
  gy01 = fract(gy01) - 0.5;
  gz01 = fract(gz01) - 0.5;
  vec4 gw01 = vec4(0.75) - abs(gx01) - abs(gy01) - abs(gz01);
  vec4 sw01 = step(gw01, vec4(0.0));
  gx01 -= sw01 * (step(0.0, gx01) - 0.5);
  gy01 -= sw01 * (step(0.0, gy01) - 0.5);

  vec4 gx10 = ixy10 * (1.0 / 7.0);
  vec4 gy10 = floor(gx10) * (1.0 / 7.0);
  vec4 gz10 = floor(gy10) * (1.0 / 6.0);
  gx10 = fract(gx10) - 0.5;
  gy10 = fract(gy10) - 0.5;
  gz10 = fract(gz10) - 0.5;
  vec4 gw10 = vec4(0.75) - abs(gx10) - abs(gy10) - abs(gz10);
  vec4 sw10 = step(gw10, vec4(0.0));
  gx10 -= sw10 * (step(0.0, gx10) - 0.5);
  gy10 -= sw10 * (step(0.0, gy10) - 0.5);

  vec4 gx11 = ixy11 * (1.0 / 7.0);
  vec4 gy11 = floor(gx11) * (1.0 / 7.0);
  vec4 gz11 = floor(gy11) * (1.0 / 6.0);
  gx11 = fract(gx11) - 0.5;
  gy11 = fract(gy11) - 0.5;
  gz11 = fract(gz11) - 0.5;
  vec4 gw11 = vec4(0.75) - abs(gx11) - abs(gy11) - abs(gz11);
  vec4 sw11 = step(gw11, vec4(0.0));
  gx11 -= sw11 * (step(0.0, gx11) - 0.5);
  gy11 -= sw11 * (step(0.0, gy11) - 0.5);

  vec4 g0000 = vec4(gx00.x,gy00.x,gz00.x,gw00.x);
  vec4 g1000 = vec4(gx00.y,gy00.y,gz00.y,gw00.y);
  vec4 g0100 = vec4(gx00.z,gy00.z,gz00.z,gw00.z);
  vec4 g1100 = vec4(gx00.w,gy00.w,gz00.w,gw00.w);
  vec4 g0010 = vec4(gx10.x,gy10.x,gz10.x,gw10.x);
  vec4 g1010 = vec4(gx10.y,gy10.y,gz10.y,gw10.y);
  vec4 g0110 = vec4(gx10.z,gy10.z,gz10.z,gw10.z);
  vec4 g1110 = vec4(gx10.w,gy10.w,gz10.w,gw10.w);
  vec4 g0001 = vec4(gx01.x,gy01.x,gz01.x,gw01.x);
  vec4 g1001 = vec4(gx01.y,gy01.y,gz01.y,gw01.y);
  vec4 g0101 = vec4(gx01.z,gy01.z,gz01.z,gw01.z);
  vec4 g1101 = vec4(gx01.w,gy01.w,gz01.w,gw01.w);
  vec4 g0011 = vec4(gx11.x,gy11.x,gz11.x,gw11.x);
  vec4 g1011 = vec4(gx11.y,gy11.y,gz11.y,gw11.y);
  vec4 g0111 = vec4(gx11.z,gy11.z,gz11.z,gw11.z);
  vec4 g1111 = vec4(gx11.w,gy11.w,gz11.w,gw11.w);

  vec4 norm00 = taylorInvSqrt(vec4(dot(g0000, g0000), dot(g0100, g0100), dot(g1000, g1000), dot(g1100, g1100)));
  g0000 *= norm00.x;
  g0100 *= norm00.y;
  g1000 *= norm00.z;
  g1100 *= norm00.w;

  vec4 norm01 = taylorInvSqrt(vec4(dot(g0001, g0001), dot(g0101, g0101), dot(g1001, g1001), dot(g1101, g1101)));
  g0001 *= norm01.x;
  g0101 *= norm01.y;
  g1001 *= norm01.z;
  g1101 *= norm01.w;

  vec4 norm10 = taylorInvSqrt(vec4(dot(g0010, g0010), dot(g0110, g0110), dot(g1010, g1010), dot(g1110, g1110)));
  g0010 *= norm10.x;
  g0110 *= norm10.y;
  g1010 *= norm10.z;
  g1110 *= norm10.w;

  vec4 norm11 = taylorInvSqrt(vec4(dot(g0011, g0011), dot(g0111, g0111), dot(g1011, g1011), dot(g1111, g1111)));
  g0011 *= norm11.x;
  g0111 *= norm11.y;
  g1011 *= norm11.z;
  g1111 *= norm11.w;

  float n0000 = dot(g0000, Pf0);
  float n1000 = dot(g1000, vec4(Pf1.x, Pf0.yzw));
  float n0100 = dot(g0100, vec4(Pf0.x, Pf1.y, Pf0.zw));
  float n1100 = dot(g1100, vec4(Pf1.xy, Pf0.zw));
  float n0010 = dot(g0010, vec4(Pf0.xy, Pf1.z, Pf0.w));
  float n1010 = dot(g1010, vec4(Pf1.x, Pf0.y, Pf1.z, Pf0.w));
  float n0110 = dot(g0110, vec4(Pf0.x, Pf1.yz, Pf0.w));
  float n1110 = dot(g1110, vec4(Pf1.xyz, Pf0.w));
  float n0001 = dot(g0001, vec4(Pf0.xyz, Pf1.w));
  float n1001 = dot(g1001, vec4(Pf1.x, Pf0.yz, Pf1.w));
  float n0101 = dot(g0101, vec4(Pf0.x, Pf1.y, Pf0.z, Pf1.w));
  float n1101 = dot(g1101, vec4(Pf1.xy, Pf0.z, Pf1.w));
  float n0011 = dot(g0011, vec4(Pf0.xy, Pf1.zw));
  float n1011 = dot(g1011, vec4(Pf1.x, Pf0.y, Pf1.zw));
  float n0111 = dot(g0111, vec4(Pf0.x, Pf1.yzw));
  float n1111 = dot(g1111, Pf1);

  vec4 fade_xyzw = fade(Pf0);
  vec4 n_0w = mix(vec4(n0000, n1000, n0100, n1100), vec4(n0001, n1001, n0101, n1101), fade_xyzw.w);
  vec4 n_1w = mix(vec4(n0010, n1010, n0110, n1110), vec4(n0011, n1011, n0111, n1111), fade_xyzw.w);
  vec4 n_zw = mix(n_0w, n_1w, fade_xyzw.z);
  vec2 n_yzw = mix(n_zw.xy, n_zw.zw, fade_xyzw.y);
  float n_xyzw = mix(n_yzw.x, n_yzw.y, fade_xyzw.x);
  return 2.2 * n_xyzw;
}

// Classic Perlin noise, periodic version
float pnoise(vec4 P, vec4 rep)
{
  vec4 Pi0 = mod(floor(P), rep); // Integer part modulo rep
  vec4 Pi1 = mod(Pi0 + 1.0, rep); // Integer part + 1 mod rep
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec4 Pf0 = fract(P); // Fractional part for interpolation
  vec4 Pf1 = Pf0 - 1.0; // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = vec4(Pi0.zzzz);
  vec4 iz1 = vec4(Pi1.zzzz);
  vec4 iw0 = vec4(Pi0.wwww);
  vec4 iw1 = vec4(Pi1.wwww);

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);
  vec4 ixy00 = permute(ixy0 + iw0);
  vec4 ixy01 = permute(ixy0 + iw1);
  vec4 ixy10 = permute(ixy1 + iw0);
  vec4 ixy11 = permute(ixy1 + iw1);

  vec4 gx00 = ixy00 * (1.0 / 7.0);
  vec4 gy00 = floor(gx00) * (1.0 / 7.0);
  vec4 gz00 = floor(gy00) * (1.0 / 6.0);
  gx00 = fract(gx00) - 0.5;
  gy00 = fract(gy00) - 0.5;
  gz00 = fract(gz00) - 0.5;
  vec4 gw00 = vec4(0.75) - abs(gx00) - abs(gy00) - abs(gz00);
  vec4 sw00 = step(gw00, vec4(0.0));
  gx00 -= sw00 * (step(0.0, gx00) - 0.5);
  gy00 -= sw00 * (step(0.0, gy00) - 0.5);

  vec4 gx01 = ixy01 * (1.0 / 7.0);
  vec4 gy01 = floor(gx01) * (1.0 / 7.0);
  vec4 gz01 = floor(gy01) * (1.0 / 6.0);
  gx01 = fract(gx01) - 0.5;
  gy01 = fract(gy01) - 0.5;
  gz01 = fract(gz01) - 0.5;
  vec4 gw01 = vec4(0.75) - abs(gx01) - abs(gy01) - abs(gz01);
  vec4 sw01 = step(gw01, vec4(0.0));
  gx01 -= sw01 * (step(0.0, gx01) - 0.5);
  gy01 -= sw01 * (step(0.0, gy01) - 0.5);

  vec4 gx10 = ixy10 * (1.0 / 7.0);
  vec4 gy10 = floor(gx10) * (1.0 / 7.0);
  vec4 gz10 = floor(gy10) * (1.0 / 6.0);
  gx10 = fract(gx10) - 0.5;
  gy10 = fract(gy10) - 0.5;
  gz10 = fract(gz10) - 0.5;
  vec4 gw10 = vec4(0.75) - abs(gx10) - abs(gy10) - abs(gz10);
  vec4 sw10 = step(gw10, vec4(0.0));
  gx10 -= sw10 * (step(0.0, gx10) - 0.5);
  gy10 -= sw10 * (step(0.0, gy10) - 0.5);

  vec4 gx11 = ixy11 * (1.0 / 7.0);
  vec4 gy11 = floor(gx11) * (1.0 / 7.0);
  vec4 gz11 = floor(gy11) * (1.0 / 6.0);
  gx11 = fract(gx11) - 0.5;
  gy11 = fract(gy11) - 0.5;
  gz11 = fract(gz11) - 0.5;
  vec4 gw11 = vec4(0.75) - abs(gx11) - abs(gy11) - abs(gz11);
  vec4 sw11 = step(gw11, vec4(0.0));
  gx11 -= sw11 * (step(0.0, gx11) - 0.5);
  gy11 -= sw11 * (step(0.0, gy11) - 0.5);

  vec4 g0000 = vec4(gx00.x,gy00.x,gz00.x,gw00.x);
  vec4 g1000 = vec4(gx00.y,gy00.y,gz00.y,gw00.y);
  vec4 g0100 = vec4(gx00.z,gy00.z,gz00.z,gw00.z);
  vec4 g1100 = vec4(gx00.w,gy00.w,gz00.w,gw00.w);
  vec4 g0010 = vec4(gx10.x,gy10.x,gz10.x,gw10.x);
  vec4 g1010 = vec4(gx10.y,gy10.y,gz10.y,gw10.y);
  vec4 g0110 = vec4(gx10.z,gy10.z,gz10.z,gw10.z);
  vec4 g1110 = vec4(gx10.w,gy10.w,gz10.w,gw10.w);
  vec4 g0001 = vec4(gx01.x,gy01.x,gz01.x,gw01.x);
  vec4 g1001 = vec4(gx01.y,gy01.y,gz01.y,gw01.y);
  vec4 g0101 = vec4(gx01.z,gy01.z,gz01.z,gw01.z);
  vec4 g1101 = vec4(gx01.w,gy01.w,gz01.w,gw01.w);
  vec4 g0011 = vec4(gx11.x,gy11.x,gz11.x,gw11.x);
  vec4 g1011 = vec4(gx11.y,gy11.y,gz11.y,gw11.y);
  vec4 g0111 = vec4(gx11.z,gy11.z,gz11.z,gw11.z);
  vec4 g1111 = vec4(gx11.w,gy11.w,gz11.w,gw11.w);

  vec4 norm00 = taylorInvSqrt(vec4(dot(g0000, g0000), dot(g0100, g0100), dot(g1000, g1000), dot(g1100, g1100)));
  g0000 *= norm00.x;
  g0100 *= norm00.y;
  g1000 *= norm00.z;
  g1100 *= norm00.w;

  vec4 norm01 = taylorInvSqrt(vec4(dot(g0001, g0001), dot(g0101, g0101), dot(g1001, g1001), dot(g1101, g1101)));
  g0001 *= norm01.x;
  g0101 *= norm01.y;
  g1001 *= norm01.z;
  g1101 *= norm01.w;

  vec4 norm10 = taylorInvSqrt(vec4(dot(g0010, g0010), dot(g0110, g0110), dot(g1010, g1010), dot(g1110, g1110)));
  g0010 *= norm10.x;
  g0110 *= norm10.y;
  g1010 *= norm10.z;
  g1110 *= norm10.w;

  vec4 norm11 = taylorInvSqrt(vec4(dot(g0011, g0011), dot(g0111, g0111), dot(g1011, g1011), dot(g1111, g1111)));
  g0011 *= norm11.x;
  g0111 *= norm11.y;
  g1011 *= norm11.z;
  g1111 *= norm11.w;

  float n0000 = dot(g0000, Pf0);
  float n1000 = dot(g1000, vec4(Pf1.x, Pf0.yzw));
  float n0100 = dot(g0100, vec4(Pf0.x, Pf1.y, Pf0.zw));
  float n1100 = dot(g1100, vec4(Pf1.xy, Pf0.zw));
  float n0010 = dot(g0010, vec4(Pf0.xy, Pf1.z, Pf0.w));
  float n1010 = dot(g1010, vec4(Pf1.x, Pf0.y, Pf1.z, Pf0.w));
  float n0110 = dot(g0110, vec4(Pf0.x, Pf1.yz, Pf0.w));
  float n1110 = dot(g1110, vec4(Pf1.xyz, Pf0.w));
  float n0001 = dot(g0001, vec4(Pf0.xyz, Pf1.w));
  float n1001 = dot(g1001, vec4(Pf1.x, Pf0.yz, Pf1.w));
  float n0101 = dot(g0101, vec4(Pf0.x, Pf1.y, Pf0.z, Pf1.w));
  float n1101 = dot(g1101, vec4(Pf1.xy, Pf0.z, Pf1.w));
  float n0011 = dot(g0011, vec4(Pf0.xy, Pf1.zw));
  float n1011 = dot(g1011, vec4(Pf1.x, Pf0.y, Pf1.zw));
  float n0111 = dot(g0111, vec4(Pf0.x, Pf1.yzw));
  float n1111 = dot(g1111, Pf1);

  vec4 fade_xyzw = fade(Pf0);
  vec4 n_0w = mix(vec4(n0000, n1000, n0100, n1100), vec4(n0001, n1001, n0101, n1101), fade_xyzw.w);
  vec4 n_1w = mix(vec4(n0010, n1010, n0110, n1110), vec4(n0011, n1011, n0111, n1111), fade_xyzw.w);
  vec4 n_zw = mix(n_0w, n_1w, fade_xyzw.z);
  vec2 n_yzw = mix(n_zw.xy, n_zw.zw, fade_xyzw.y);
  float n_xyzw = mix(n_yzw.x, n_yzw.y, fade_xyzw.x);
  return 2.2 * n_xyzw;
}

float noise(vec3 p) {
    return 0.5 * cnoise(vec4(p, 0)) + 0.5;
}

float nebula(vec3 p) {
    const int steps = 6;
    float scale = pow(2.0, float(steps));
    vec3 displace;
    for (int i = 0; i < steps; i++) {
        displace = vec3(
            noise(p.xyz * scale + displace),
            noise(p.yzx * scale + displace),
            noise(p.zxy * scale + displace)
        );
        scale *= 0.5;
    }
    return noise(p * scale + displace);
}

void main() {
    vec3 posn = normalize(v_position) * uScale;
    float c = min(1.0, nebula(posn + uOffset) * uIntensity);
    c = pow(c, uFalloff);
    fragmentColor = vec4(uColor.xyz, c);
}
)";

static const std::string SKYBOX_NEBULA_VERT = R"(#version 330 core
layout(location = 0) in vec3 position;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 v_position;

void main() {
    vec4 worldPos = vec4(position, 1);
    v_position = worldPos.xyz;
    gl_Position = projectionMatrix * viewMatrix * worldPos;
}
)";

// Simple skybox box with two triangles per side.
static const float SKYBOX_VERTICES[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

static const std::array<GLenum, 6> CUBEMAP_ENUMS = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                                                    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                                                    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// The projection matrix that will be used to generate the skybox.
// This must be 90 degrees view (PI/2)
static const glm::mat4 CAPTURE_PROJECTION = glm::perspective(M_PI / 2.0, 1.0, 0.1, 1000.0);

// The following are views that match the CUBEMAP_ENUMS values.
// We need to capture the skybox from all 6 sides!
static const std::array<glm::mat4, 6> CAPTURE_VIEWS = {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

struct StarVertex {
    glm::vec3 position;
    float brightness;
    glm::vec4 color;
};

Space3d::Skybox::Result::Result() : ref(0) {
    glGenTextures(1, &ref);
}

Space3d::Skybox::Result::~Result() {
    if (ref) {
        glDeleteTextures(1, &ref);
    }
}

void Space3d::Skybox::Result::bind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ref);
}

void Space3d::Skybox::Result::setStorage(const int width, const GLenum internalFormat, const GLenum format,
                                         const GLenum type) {
    bind();
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, width, 0, format, type, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Not doing this will cause the skybox to have seems!
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Space3d::Skybox::Result::generateMipmaps() {
    bind();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Space3d::Skybox::Result::release() {
    ref = 0;
}

Space3d::Skybox::Result::Result(Result&& other) noexcept : ref(0) {
    swap(other);
}

void Space3d::Skybox::Result::swap(Result& other) noexcept {
    std::swap(ref, other.ref);
}

Space3d::Skybox::Result& Space3d::Skybox::Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        swap(other);
    }
    return *this;
}

Space3d::Skybox::Skybox()
    : shaderStars(SKYBOX_STARS_VERT, SKYBOX_STARS_FRAG, SKYBOX_STARS_GEOM),
      shaderNebula(SKYBOX_NEBULA_VERT, SKYBOX_NEBULA_FRAG, std::nullopt) {

    meshSkybox.vao.bind();
    meshSkybox.vbo.bind();
    meshSkybox.vbo.bufferData(reinterpret_cast<const uint8_t*>(SKYBOX_VERTICES), sizeof(SKYBOX_VERTICES));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

// The following algorithm is based on space-3d by wwwtyro from https://github.com/wwwtyro/space-3d
// With minor adjustments, such as using geometry shader to create star billboards instead
// of creating them manually.
Space3d::Skybox::Result Space3d::Skybox::generate(const int64_t seed, const int width) const {
    std::mt19937_64 rng(seed);

    // Cube map that will hold the final skybox texture
    Result result;
    result.setStorage(width, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

    // Temporary FBO object for rendering
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    static const GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);
    glViewport(0, 0, width, width);

    // Clear FBO texture to all black
    const glm::vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
    for (unsigned int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, CUBEMAP_ENUMS[i], result.get(), 0);
        glClearBufferfv(GL_COLOR, 0, &black[0]);
    }

    // Set the blending mode to add only
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Render the stars
    struct Params {
        size_t starsCount;
        glm::vec2 particleSize;
    };

    // clang-format off
    // This is list of star parameters, feel free to add more.
    std::array<Params, 2> allParams = {
        // A lot of tiny stars
        Params{
            20000ULL,
            {0.05f, 0.05f}
        },
        // Just few more bigger stars
        Params{
            100ULL,
            {0.2f, 0.2f}
        }
    };
    // clang-format on

    // Loop through the star parameters from above.
    for (const auto& params : allParams) {
        // First, create some random stars as points.
        std::uniform_real_distribution<float> distPosition(-1.0f, 1.0f);
        std::uniform_real_distribution<float> distColor(0.9f, 1.0f);
        std::uniform_real_distribution<float> distBrightness(0.7f, 1.0f);

        std::vector<StarVertex> stars;
        stars.resize(params.starsCount);

        for (auto& star : stars) {
            star.position = normalize((glm::vec3{distPosition(rng), distPosition(rng), distPosition(rng)})) * 100.0f;
            star.color = glm::vec4{distColor(rng), distColor(rng), distColor(rng), 1.0f};
            star.brightness = distBrightness(rng);
        }

        // Create a VAO and VBO objects that will hold the star points.
        // The points will be converted to triangle strips via geometry shader.
        Vao vaoStars;
        vaoStars.bind();

        Vbo vboStars;
        vboStars.bind();
        vboStars.bufferData(reinterpret_cast<const uint8_t*>(stars.data()), stars.size() * sizeof(StarVertex));

        // The VBO stars with a vec3 (positions)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void*)0);

        // Then it follows with a brightness value.
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void*)(1 * sizeof(glm::vec3)));

        // And ends with a vec4 (color)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(StarVertex),
                              (void*)(1 * sizeof(glm::vec3) + sizeof(float)));

        // The VBO layout:
        // pos.x, pos.y, pos.z, brightness, color.r, color.g, color.b, color.a

        // Render the stars
        shaderStars.use();
        shaderStars.setMat4("projectionMatrix", CAPTURE_PROJECTION);
        shaderStars.setVec2("particleSize", params.particleSize);

        // Render for all cubemap sides.
        for (unsigned int i = 0; i < 6; ++i) {
            shaderStars.setMat4("viewMatrix", CAPTURE_VIEWS[i]);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, CUBEMAP_ENUMS[i], result.get(), 0);
            shaderStars.drawArrays(GL_POINTS, static_cast<GLsizei>(stars.size()));
        }
    }

    // Render the nebulas
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    shaderNebula.use();
    shaderNebula.setMat4("projectionMatrix", CAPTURE_PROJECTION);
    meshSkybox.vao.bind();

    while (true) {
        shaderNebula.setFloat("uScale", dist(rng) * 0.5f + 0.25f);
        shaderNebula.setFloat("uIntensity", dist(rng) * 0.2f + 0.9f);
        shaderNebula.setVec4("uColor", glm::vec4{dist(rng), dist(rng), dist(rng), 1.0f});
        shaderNebula.setFloat("uFalloff", dist(rng) * 3.0f + 3.0f);
        shaderNebula.setVec3("uOffset", glm::vec3{dist(rng) * 2000.0f - 1000.0f, dist(rng) * 2000.0f - 1000.0f,
                                                  dist(rng) * 2000.0f - 1000.0f});

        for (unsigned int i = 0; i < 6; ++i) {
            shaderNebula.setMat4("viewMatrix", CAPTURE_VIEWS[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   result.get(), 0);
            shaderNebula.drawArrays(GL_TRIANGLES, 6 * 6);
        }

        if (dist(rng) < 0.5f) {
            break;
        }
    }

    // Delete the framebuffer and reset it to the default one.
    glDeleteFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Generate cubemap mipmaps.
    result.generateMipmaps();

    return result;
}
