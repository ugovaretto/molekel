varying vec3 ViewDirection;
varying vec3 Normal;
   
void main( void )
{
   gl_Position = ftransform();
 
   const vec3 fvEyePosition = vec3( 0.0, 0.0, 0.0 );	
   vec4 fvObjectPosition = gl_ModelViewMatrix * gl_Vertex;
   
   ViewDirection  = normalize(fvEyePosition - fvObjectPosition.xyz);
   Normal         = gl_NormalMatrix * gl_Normal;
   
}