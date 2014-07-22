// wwlk
//
// Trivial Vertex shader
//



void main(void)
{ 
	
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

}