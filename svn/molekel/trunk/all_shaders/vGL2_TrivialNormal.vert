// wwlk
//
// Vertex shader to setup per fragment shading.
// We will transform to two shader spaces.
//
// First, we will transform normals and object coordinates to eye space.
// Second, we will pass through normals and object coordinates.
//

varying vec4 v_EyeNormal;
varying vec4 v_EyePos;

varying vec4 v_ObjNormal;
varying vec4 v_ObjPos;


uniform vec4 my01H2;			// please load with ( 0.0, 1.0, 0.5, 2.0)
uniform vec4 objScale;		// Object Scale Factor


void main(void)
{ 
	vec4 ObjNormal;
	vec4 ObjPos;
	vec3 EyeNormal;
	vec4 EyePos; 

	ObjPos = gl_Vertex;

	//	Must write gl_Position for rasterization to be defined....
	//
	gl_Position = gl_ModelViewProjectionMatrix * ObjPos;

	//	Transform to shading space (we are going to shade in eyespace)
	//
	//
	EyeNormal = gl_NormalMatrix * gl_Normal;
	v_EyeNormal = vec4( EyeNormal, 0.0 );

	EyePos = gl_ModelViewMatrix * ObjPos;
	v_EyePos = EyePos;

	//	Pass through object normal and object position
	//
	ObjNormal = vec4(gl_Normal, 0.0);

	v_ObjNormal = ObjNormal;
	v_ObjPos = ObjPos;
}