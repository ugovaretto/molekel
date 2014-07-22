// X-ray shader
// Opacity is proportional to angle between view vector and surface normal.
// Fragment whose dNormal is less than a specified threshold are discarded; i.e.
// flat areas facing the viewer are completely transparent

varying vec3 ViewDirection;
varying vec3 Normal;

uniform float minOpacity; // min alpha value
uniform float maxOpacity; // max alpha value
uniform vec3 color; // color
uniform float min_dN; // minimum length of dN = |dN/dx + dN/dy|

void main(void)
{
   if( length( dFdx( Normal ) + dFdy( Normal ) ) < min_dN ) discard;
   float alpha = 1.0 - abs( dot( ViewDirection, Normal ) );
   alpha = clamp( alpha, minOpacity, maxOpacity );
   gl_FragColor = vec4( color, alpha );
}
