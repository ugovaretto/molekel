// wwlk
//
// [Gooch98]
// [King]

varying vec4 v_EyeNormal;
varying vec4 v_EyePos;

varying vec4 v_ObjNormal;
varying vec4 v_ObjPos;

uniform vec4 my01H2;			// please load with (0.0, 1.0, 0.5, 2.0)

uniform vec4 myLightDir;		// NOTA BENE:  You should normalize these directions.
                        		//             This shader assumes that light
								//             directions are already normalized.
								//
								//

uniform vec4 myEyeDir;			// please load with (0.0, 0.0, 1.0, 0.0)
uniform vec4 myHVector;		// please load with normalize(myLightDir1+myEyeDir)


uniform vec4 mySpecularColor1; 	// please load .a with shininess


uniform vec4 myAlphaBeta;		// ( 0.25, 0.5, 0.0, 0.0 )
uniform vec4 myBlue;			// ( 0.55, 0.0, 0.0, 0.0 )
uniform vec4 myYellow;			// ( 0.3,  0.3, 0.0, 0.0 )
uniform vec4 myColor;
uniform vec4 myK;


void main (void)
{

	vec4 NNormal;
	vec4 SurfColor;
	vec4 Cool;
	vec4 Warm;
	vec4 GoochColor;
	vec4 Intensity;

//
//	Since the EyeNormal is getting interpolated, we
//	have to first restore it by normalizing it.
//
	NNormal = normalize( v_EyeNormal );

//
//	Per fragment gooch lighting

	Intensity.a = dot ( NNormal, myLightDir );
//  Gooch maps Intensity from (-1,1) to (0,1)
    Intensity.a = Intensity.a * my01H2.z + my01H2.z;	// Intensity.a * 0.5 + 0.5;

    Cool = myAlphaBeta.x * myColor + myBlue;
	Warm = myAlphaBeta.y * myColor + myYellow;
	SurfColor = myK.y * mix( Cool, Warm, Intensity.a);

        //SurfColor = vec4(Intensity.a);

//
//	Now, specular light....


	Intensity.a = dot ( NNormal, myHVector );
	Intensity.a = max ( Intensity.a, my01H2.x );
	Intensity.a = pow ( Intensity.a, mySpecularColor1.a );
	
SurfColor += myK.z* Intensity.a;		


	SurfColor.a = myColor.a;
	
	gl_FragColor = SurfColor;

}