// wwlk
//
// Show half-spaces

varying vec4 v_EyePos;
varying vec3 v_ObjPos;


uniform vec4 my01H2;			// please load with (0.0, 1.0, 0.5, 2.0)
uniform vec4 myLightDir;		// NOTA BENE:  You should normalize these directions.

uniform vec4 myEyeDir;			// please load with (0.0, 0.0, 1.0, 0.0)
uniform vec4 myHVector;		// please load with normalize(myLightDir1+myEyeDir)

uniform vec4 mySpecularColor;

uniform vec4 myRed;
uniform vec4 myYellow;
uniform vec4 myBlue;
uniform vec4 myGreen;

uniform vec3 yellow;
uniform vec3 red;

uniform vec4 eyeCenter;

uniform vec4 myHalfSpace0;
uniform vec4 myHalfSpace1;
uniform vec4 myHalfSpace2;
uniform vec4 myHalfSpace3;
uniform vec4 myHalfSpace4;
uniform vec4 myHalfSpace5;

uniform vec3 myHalfSpace0TEMP;
uniform vec3 myHalfSpace1TEMP;
uniform vec3 myHalfSpace2TEMP;
uniform vec3 myHalfSpace3TEMP;
uniform vec3 myHalfSpace4TEMP;

uniform vec4 zero;
uniform vec4 one;

uniform vec4 toyBallConst;			// please load with (-3.0, 0.4, 0.0078125, 0.02)

void main (void)
{
	vec3 NNormal;
	vec3 ObjPos;
	vec4 TempColor;
	vec4 SurfColor;
	float Intensity;


	vec4 distance;
	float myInOut;

	ObjPos = normalize( v_ObjPos );

	myInOut = toyBallConst.x;

	distance.x = dot( ObjPos, myHalfSpace0TEMP );
	distance.y = dot( ObjPos, myHalfSpace1TEMP );
	distance.z = dot( ObjPos, myHalfSpace2TEMP );
	distance.w = dot( ObjPos, myHalfSpace3TEMP );
	distance += 0.2;
	distance = smoothstep( zero, vec4(toyBallConst.z), distance );
	myInOut += dot( distance,one );

	distance.x = dot( ObjPos, myHalfSpace4TEMP ) + 0.2;
	distance.y = abs( ObjPos.z) - toyBallConst.y;
	distance = smoothstep( zero, vec4(toyBallConst.z), distance );
	myInOut += distance.x;
	myInOut = clamp(  myInOut, 0.0, 1.0);

	SurfColor = mix (myYellow, myRed, myInOut );
	SurfColor = mix ( myBlue, SurfColor, distance.y );

//
//	Analytically calculate the sphere NNormal in eye coordinates
//

    NNormal = vec3(v_EyePos - eyeCenter);
	NNormal = normalize( NNormal );

//
//	Per fragment diffuse lighting

	Intensity = dot ( vec3(myLightDir), NNormal );
	//Intensity = clamp ( Intensity, 0.0, 1.0  );
	SurfColor *= Intensity;
	//SurfColor = vec4(NNormal, 1.0);

//
//	Per fragment specular lighting

	Intensity = dot ( NNormal, vec3(myHVector) );
	Intensity = clamp ( Intensity, 0.0, 1.0 );
	Intensity = pow ( Intensity, mySpecularColor.a );
	SurfColor += mySpecularColor * Intensity;

	SurfColor.a = my01H2.y;							// MyColor.a = 1.0;
	
	gl_FragColor = SurfColor;
}
