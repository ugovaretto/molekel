//Simplified version of VMD vertex shader

// requires GLSL version 1.10
#version 110

uniform bool orthographic;
uniform int myintvalue;
uniform float factor;

//
// Outputs to fragment shader
//
varying vec3 oglnormal;          // output interpolated normal to frag shader
varying vec3 oglcolor;           // output interpolated color to frag shader
varying vec3 V;                  // output view direction vector
varying float fogZ;
varying vec4 pos;

void main(void) {

  pos = gl_Vertex;
  pos.xyz /= gl_Vertex.w;
  pos.xyz *= factor;	

  // transform vertex to Eye space for user clipping plane calculations
  vec4 ecpos = gl_ModelViewMatrix * gl_Vertex;
  gl_ClipVertex = ecpos;

  // transform, normalize, and output normal.
  oglnormal = normalize(gl_NormalMatrix * gl_Normal);

  // pass along vertex color for use fragment shading,
  // fragment shader will get an interpolated color.
  oglcolor = vec3(gl_Color);

  // setup fog coordinate for fragment shader
  gl_FogFragCoord = abs(ecpos.z);
  fogZ = abs(ecpos.z);

  if (!orthographic) {
    // set view direction vector from eye coordinate of vertex, for
    // perspective views
    V = normalize(vec3(ecpos) / ecpos.w);
  } else {
    // set view direction vector with constant eye coordinate, used for
    // orthographic views
    V = vec3(.0, .0, -1.0);
  }
  // transform vertex to Clip space
  gl_Position = ftransform();
}



