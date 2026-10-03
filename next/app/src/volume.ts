import * as THREE from "three";
import type { Grid, RenderMode } from "./types";

export function volumeObject(
  grid: Grid,
  mode: RenderMode,
  iso: number,
  positive: string,
  negative: string,
  opacity: number,
  filtered: boolean,
) {
  const [nx, ny, nz] = grid.dims;
  const texture = new THREE.Data3DTexture(
    new Float32Array(grid.values),
    nx,
    ny,
    nz,
  );
  texture.format = THREE.RedFormat;
  texture.type = THREE.FloatType;
  texture.minFilter = texture.magFilter = filtered
    ? THREE.LinearFilter
    : THREE.NearestFilter;
  texture.unpackAlignment = 1;
  texture.needsUpdate = true;
  const a = grid.axes.map((v, i) =>
    new THREE.Vector3(...v).multiplyScalar(grid.dims[i] - 1),
  );
  const transform = new THREE.Matrix4()
    .makeBasis(a[0], a[1], a[2])
    .setPosition(...(grid.origin as [number, number, number]));
  const geometry = new THREE.BoxGeometry(1, 1, 1)
    .translate(0.5, 0.5, 0.5)
    .applyMatrix4(transform);
  // The baked transform bypasses Three.js's object-matrix winding correction.
  if (transform.determinant() < 0) {
    const indices = geometry.getIndex()!;
    for (let i = 0; i < indices.count; i += 3) {
      const second = indices.getX(i + 1);
      indices.setX(i + 1, indices.getX(i + 2));
      indices.setX(i + 2, second);
    }
  }
  const material = new THREE.ShaderMaterial({
    glslVersion: THREE.GLSL3,
    side: THREE.BackSide,
    transparent: true,
    depthWrite: mode === "raycast",
    defines: filtered ? { FILTERED_FIELD: 1 } : {},
    uniforms: {
      field: { value: texture },
      gridSize: { value: new THREE.Vector3(nx, ny, nz) },
      worldToGrid: { value: transform.clone().invert() },
      vp: { value: new THREE.Matrix4() },
      positiveColor: { value: new THREE.Color(positive) },
      negativeColor: { value: new THREE.Color(negative) },
      iso: { value: iso },
      opacity: { value: opacity },
      volumeMode: { value: mode === "volume" },
      epsilon: {
        value:
          Math.min(...grid.axes.map((v) => new THREE.Vector3(...v).length())) *
          0.5,
      },
    },
    vertexShader: `out vec3 worldPoint;
      void main() { vec4 p=modelMatrix*vec4(position,1.0);worldPoint=p.xyz;gl_Position=projectionMatrix*viewMatrix*p; }`,
    fragmentShader: `precision highp float; precision highp sampler3D;
      out vec4 outputColor;
      #define gl_FragColor outputColor
      uniform sampler3D field; uniform vec3 gridSize; uniform mat4 worldToGrid; uniform mat4 vp;
      uniform vec3 positiveColor; uniform vec3 negativeColor; uniform float iso; uniform float opacity;
      uniform bool volumeMode; uniform float epsilon; in vec3 worldPoint;
      float scalar(vec3 p) {
        vec3 g=clamp((worldToGrid*vec4(p,1.0)).xyz,0.0,1.0)*(gridSize-1.0);
        #ifdef FILTERED_FIELD
          return texture(field,(g+0.5)/gridSize).r;
        #else
        ivec3 i=ivec3(floor(g)); ivec3 j=min(i+ivec3(1),ivec3(gridSize)-1);vec3 f=fract(g);
        float z0=mix(mix(texelFetch(field,i,0).r,texelFetch(field,ivec3(j.x,i.y,i.z),0).r,f.x),
          mix(texelFetch(field,ivec3(i.x,j.y,i.z),0).r,texelFetch(field,ivec3(j.x,j.y,i.z),0).r,f.x),f.y);
        float z1=mix(mix(texelFetch(field,ivec3(i.x,i.y,j.z),0).r,texelFetch(field,ivec3(j.x,i.y,j.z),0).r,f.x),
          mix(texelFetch(field,ivec3(i.x,j.y,j.z),0).r,texelFetch(field,j,0).r,f.x),f.y);
        return mix(z0,z1,f.z);
        #endif
      }
      vec3 gradient(vec3 p) {return vec3(scalar(p+vec3(epsilon,0,0))-scalar(p-vec3(epsilon,0,0)),
        scalar(p+vec3(0,epsilon,0))-scalar(p-vec3(0,epsilon,0)),scalar(p+vec3(0,0,epsilon))-scalar(p-vec3(0,0,epsilon)));}
      void main() {
        vec3 direction=normalize(worldPoint-cameraPosition);
        vec3 o=(worldToGrid*vec4(cameraPosition,1)).xyz;vec3 d=(worldToGrid*vec4(direction,0)).xyz;
        vec3 safeD=vec3(d.x==0.0?1e-20:d.x,d.y==0.0?1e-20:d.y,d.z==0.0?1e-20:d.z);
        vec3 ta=(vec3(0)-o)/safeD;vec3 tb=(vec3(1)-o)/safeD;
        vec3 low=min(ta,tb);vec3 high=max(ta,tb);
        float start=max(0.0,max(low.x,max(low.y,low.z)));float end=min(high.x,min(high.y,high.z));
        if(end<=start)discard;
        float stepSize=(end-start)/160.0;
        vec4 accum=vec4(0);float first=-1.0;float prev=scalar(cameraPosition+direction*start);
        for(int i=1;i<=160;i++) {
          float t=start+float(i)*stepSize;vec3 p=cameraPosition+direction*t;float value=scalar(p);
          vec3 color=value>=0.0?positiveColor:negativeColor;float alpha=0.0;float hit=t;
          if(volumeMode) {
            float band=smoothstep(iso*0.3,iso,max(abs(value),0.0));
            alpha=1.0-exp(-band*opacity*stepSize*1.4);
          } else {
            bool pos=(prev-iso)*(value-iso)<0.0;bool neg=(prev+iso)*(value+iso)<0.0;
            if(pos||neg) {
              float level=pos?iso:-iso;float lo=t-stepSize;float hi=t;float vl=prev-level;
              for(int j=0;j<9;j++) {float mid=(lo+hi)*0.5;float vm=scalar(cameraPosition+direction*mid)-level;
                if(vl*vm<=0.0)hi=mid;else{lo=mid;vl=vm;}}
              hit=(lo+hi)*0.5;p=cameraPosition+direction*hit;
              vec3 grad=gradient(p);vec3 n=length(grad)>1e-12?normalize(grad):vec3(0,1,0);
              float light=0.4+0.6*abs(dot(n,normalize(vec3(0.4,0.7,1.0))));
              color=(pos?positiveColor:negativeColor)*light;alpha=opacity;
            }
          }
          if(alpha>0.001&&first<0.0)first=hit;
          accum.rgb+=(1.0-accum.a)*color*alpha;accum.a+=(1.0-accum.a)*alpha;
          if(accum.a>0.99)break;prev=value;
        }
        if(accum.a<0.005||first<0.0)discard;
        vec4 clip=vp*vec4(cameraPosition+direction*first,1.0);gl_FragDepth=0.5*(clip.z/clip.w)+0.5;
        gl_FragColor=vec4(accum.rgb/accum.a,accum.a);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }`,
  });
  const mesh = new THREE.Mesh(geometry, material);
  mesh.onBeforeRender = (_r, _s, camera) =>
    material.uniforms.vp.value.multiplyMatrices(
      camera.projectionMatrix,
      camera.matrixWorldInverse,
    );
  mesh.userData.texture = texture;
  return mesh;
}
