// wwlk
//
// Sphere setup,
// Just pass down ObjPos (for procedural texture shader space) and EyePos (for lighting space)
//
// Also, calculate per-vertex filter widths based on surface
// dzdx and dzdy in eye space.
//

varying vec4 v_ObjPos;
varying vec4 v_EyePos;

uniform vec4 myEyeCenter;
uniform vec4 myWidth;

void main(void)
{ 
	vec4 ObjPos;
	vec4 EyePos;

	vec3 NNormal;
	vec3 dF; 

	ObjPos = gl_Vertex;

	//
	//	Must write gl_Position for rasterization to be defined....

	gl_Position = gl_ModelViewProjectionMatrix * ObjPos;
	v_ObjPos = ObjPos;

	EyePos = gl_ModelViewMatrix * ObjPos;
	v_EyePos = EyePos;

	//
	//  Analytic normal per vertex, to estimate filter width per vertex

	NNormal = vec3(EyePos - myEyeCenter);
	NNormal = normalize( NNormal );


	dF.z = 1.0/NNormal.z;
	dF = abs( NNormal * dF.z );

	v_EyePos.w =  max( 0.001, ( dF.x + dF.y ) * myWidth.x );

}