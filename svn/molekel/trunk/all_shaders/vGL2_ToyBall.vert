// wwlk
//
// Sphere setup,
// Just pass down ObjPos (for procedural texture shader space) and EyePos (for lighting space)
//

varying vec3 v_ObjPos;
varying vec4 v_EyePos;


uniform vec4 my01H2;			// please load with ( 0.0, 1.0, 0.5, 2.0)

void main(void)
{ 
	vec4 ObjPos;
	vec4 EyePos; 

	ObjPos = gl_Vertex;

	//	Must write gl_Position for rasterization to be defined....
	//
	gl_Position = gl_ModelViewProjectionMatrix * ObjPos;
	v_ObjPos = vec3(ObjPos);

	EyePos = gl_ModelViewMatrix * ObjPos;
	v_EyePos = EyePos;
}