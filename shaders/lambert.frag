varying vec4 color;
varying vec3 normal;

uniform vec3 lightDir;
uniform vec3 ambientColor;

void main()
{
  float kd = abs( dot( normalize( lightDir ), normal ) );   
  gl_FragColor = vec4( color.rgb * kd + ambientColor, color.a );
}
