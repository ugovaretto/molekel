// Simplified version of VMD fragment shader with fog, used actual reflection vectors
// instead of halfway vectors. Current fog implementation is dependent on distance between
// far and near clipping planes (toolkits like VTK do automatically update near and far planes).

// requires GLSL version 1.10
#version 110

// parameters from vertex shader
varying vec3 oglnormal;       // interpolated normal from the vertex shader
varying vec3 oglcolor;        // interpolated color from the vertex shader
varying vec3 V;               // normalized view direction vector
varying float fogZ;

// parameters
uniform vec3 lightDirection0;
uniform vec3 lightDirection1;
uniform vec3 lightDirection2;
uniform vec3 lightDirection3;


uniform vec4 lightscale; // light scaling factor

uniform vec4 material;   // material[ 0 ] = ambient value,
                         // material[ 1 ] = diffuse scaling factor
                         // material[ 2 ] = specular scaling factor
                         // material[ 3 ] = shininess

uniform float opacity;   // opacity

uniform int fogmode; // (1) linear (2) exponential (3) exponential squared

uniform float fogDensity; // 0 - 0.05

uniform float fogScale; //  0 - 0.05

uniform vec3 fogColor; // background

void main(void)
{

  // Flip the surface normal if it is facing away from the viewer,
  // determined by polygon winding order provided by OpenGL.
  vec3 N = normalize(oglnormal);
  if (!gl_FrontFacing) N = -N;

  // beginning of shading calculations
  float ambient = material[0];   // ambient
  float diffuse = 0.0;
  float specular = 0.0;
  float shininess = material[3]; // shininess

  // calculate diffuse lighting contribution
  diffuse += max(0.0, dot(N, lightDirection0)) * lightscale[0];
  diffuse += max(0.0, dot(N, lightDirection1)) * lightscale[1];
  diffuse += max(0.0, dot(N, lightDirection2)) * lightscale[2];
  diffuse += max(0.0, dot(N, lightDirection3)) * lightscale[3];
  diffuse *= material[1]; // diffuse scaling factor

  // calculate specular lighting contribution with Phong highlights using
  // reflection vectors
  vec3 R0 = reflect( lightDirection0, N );
  vec3 R1 = reflect( lightDirection1, N );
  vec3 R2 = reflect( lightDirection2, N );
  vec3 R3 = reflect( lightDirection3, N );

  specular += pow(max(0.0, dot(R0, V)), shininess) * lightscale[0];
  specular += pow(max(0.0, dot(R1, V)), shininess) * lightscale[1];
  specular += pow(max(0.0, dot(R2, V)), shininess) * lightscale[2];
  specular += pow(max(0.0, dot(R3, V)), shininess) * lightscale[3];
  specular *= material[2]; // specular scaling factor

  // Fog computations
  float Log2E = 1.442695; // = log2(2.718281828)
  float fog = 0.0;
  if (fogmode == 1) {
    // linear fog
    fog = fogZ * fogScale;
  }
  if (fogmode == 2) {
    // exponential fog
    fog = 1.0 - exp2(-fogDensity * fogZ * Log2E);
  }
  if (fogmode == 3) {
    // exponential-squared fog
    fog = 1.0 - exp2(-fogDensity * fogDensity * fogZ * fogZ * Log2E);
  }
  fog = clamp(fog, 0.0, 1.0);       // clamp the final fog parameter [0->1)

  vec3 objcolor = oglcolor * vec3(diffuse);
  vec3 color = objcolor + vec3( ambient + specular );
  gl_FragColor = vec4(mix(color, fogColor, fog ), opacity);
}


