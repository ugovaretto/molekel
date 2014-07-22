// wwlk
//
// Show half-spaces

varying vec4 v_EyePos;
varying vec4 v_ObjPos;

uniform vec4 my01H2;			// please load with (0.0, 1.0, 0.5, 2.0)
uniform vec4 myLightDir;		// NOTA BENE:  You should normalize these directions.

uniform vec4 myEyeDir;			// please load with (0.0, 0.0, 1.0, 0.0)
uniform vec4 myHVector;

uniform vec4 mySpecularColor;

uniform vec4 myRed;
uniform vec4 myYellow;
uniform vec4 myBlue;
uniform vec4 myGreen;

uniform vec4 eyeCenter;

uniform vec4 myHalfSpace0;
uniform vec4 myHalfSpace1;
uniform vec4 myHalfSpace2;
uniform vec4 myHalfSpace3;
uniform vec4 myHalfSpace4;
uniform vec4 myHalfSpace5;

uniform vec4 toyBallConst;			// please load with (-3.0, 0.4, 0.0, 0.0)
uniform vec4 zero;
uniform vec4 one;
uniform vec4 myWidth;

void main (void)
{
	vec4 NNormal;
	vec4 P;				// The point in shader space
	vec4 SurfColor;
	vec4 FWidth;
	float Intensity;

	vec4 distance;
	float myInOut;

	P = v_ObjPos;
	P.w = 0.0;
	P = normalize( P );
	P.w = 1.0;

	FWidth = vec4( v_EyePos.w );

	myInOut = toyBallConst.x;

	distance.x = dot( P, myHalfSpace0 );
	distance.y = dot( P, myHalfSpace1 );
	distance.z = dot( P, myHalfSpace2 );
	distance.w = dot( P, myHalfSpace3 );
	distance = smoothstep( -FWidth, FWidth, distance );
	myInOut += dot( distance, one );

	distance.x = dot( P, myHalfSpace4 );
	distance.y = toyBallConst.y - abs( P.z );
	distance = smoothstep( -FWidth, FWidth, distance );
	myInOut += distance.x;

	myInOut = clamp( myInOut, 0.0, 1.0 );

	SurfColor = mix (myYellow, myRed, myInOut );
	SurfColor = mix (SurfColor, myBlue, distance.y );

//
//	Analytically calculate the sphere NNormal in eye coordinates

	NNormal = v_EyePos - eyeCenter;
	NNormal = normalize( NNormal );
//
//	Per fragment diffuse lighting

	Intensity = clamp( dot ( myLightDir, NNormal ), 0.0, 1.0 );
	SurfColor *= Intensity;

//
//	Per fragment specular lighting

	Intensity = clamp( dot ( myHVector, NNormal ), 0.0, 1.0 );
	Intensity = pow ( Intensity, mySpecularColor.a );
	SurfColor += mySpecularColor * Intensity;

	SurfColor.a = 1.0;
	
	gl_FragColor = SurfColor;
}