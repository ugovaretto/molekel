void main()
{
  const float w = abs( gl_FragCoord.w );
  const float z = sqrt( sqrt( gl_FragCoord.z * w ) );
  gl_FragColor = vec4( z, z, z, 1. ); 	
}