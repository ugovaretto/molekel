varying vec3 ViewDirection;
varying vec3 Normal;

uniform vec3 color;
uniform float dn = 0.01;

void main(void)
{
   // uncomment to enable sketch 
   if( length( dFdx( Normal ) + dFdy( Normal ) ) < dn ) discard;	
   float v = 1.0 - sqrt( sqrt( abs(dot( ViewDirection, Normal )) ) );
   clamp( v, 0.1, 0.95 );
   //if( v < 0.8 ) gl_FragColor = vec4( 0.2, 0.8, 0.2, v );
   //else gl_FragColor = vec4( 0.7, 0.8, 0.2, v );
   //gl_FragColor = vec4( 0.2, 0.8, 0.2, v );
   gl_FragColor = vec4( color, v );
}