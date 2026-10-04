import { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { Focus, Camera, RotateCcw, ZoomIn, ZoomOut } from "lucide-react";
import type { MolekelDocument, SampledGrid, RenderMode } from "./types";
import { volumeObject } from "./volume";

const BOHR = 0.529177210903;
const elements: Record<
  number,
  { symbol: string; color: string; radius: number }
> = {
  1: { symbol: "H", color: "#fafafa", radius: 1.2 },
  6: { symbol: "C", color: "#535b64", radius: 1.7 },
  7: { symbol: "N", color: "#407bd2", radius: 1.55 },
  8: { symbol: "O", color: "#df5153", radius: 1.52 },
  9: { symbol: "F", color: "#69b45a", radius: 1.47 },
  15: { symbol: "P", color: "#ba72ca", radius: 1.8 },
  16: { symbol: "S", color: "#dcc33d", radius: 1.8 },
  17: { symbol: "Cl", color: "#62bb6c", radius: 1.75 },
};
export function symbol(z: number) {
  return elements[z]?.symbol ?? `Z${z}`;
}
function dispose(group: THREE.Object3D) {
  group.traverse((o) => {
    if (o instanceof THREE.Mesh) {
      o.geometry.dispose();
      for (const m of Array.isArray(o.material) ? o.material : [o.material])
        m.dispose();
      o.userData.texture?.dispose();
    }
  });
}

export function Viewport({
  doc,
  grid,
  mode,
  reset,
  onSelect,
}: {
  doc: MolekelDocument;
  grid: SampledGrid | null;
  mode: RenderMode;
  reset: number;
  onSelect: (text: string) => void;
}) {
  const host = useRef<HTMLDivElement>(null);
  const engine = useRef<{
    renderer: THREE.WebGLRenderer;
    scene: THREE.Scene;
    camera: THREE.PerspectiveCamera;
    controls: OrbitControls;
    group: THREE.Group;
    fit: () => void;
    needsRender: boolean;
  } | null>(null);
  const selection = useRef(onSelect);
  selection.current = onSelect;
  const [error, setError] = useState("");
  useEffect(() => {
    const div = host.current!;
    let renderer: THREE.WebGLRenderer;
    try {
      renderer = new THREE.WebGLRenderer({
        antialias: true,
        preserveDrawingBuffer: true,
      });
    } catch {
      setError(
        "WebGL2 is unavailable. Check graphics acceleration or try another device.",
      );
      return;
    }
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setClearColor("#e8edef");
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.domElement.setAttribute("aria-label", "Molecular scene");
    div.appendChild(renderer.domElement);
    const scene = new THREE.Scene();
    scene.add(new THREE.HemisphereLight(0xffffff, 0x6e7880, 2.4));
    const key = new THREE.DirectionalLight(0xffffff, 3);
    key.position.set(4, 6, 8);
    scene.add(key);
    const camera = new THREE.PerspectiveCamera(35, 1, 0.01, 1000);
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    const group = new THREE.Group();
    scene.add(group);
    const fit = () => {
      const box = new THREE.Box3().setFromObject(group);
      if (box.isEmpty()) return;
      const center = box.getCenter(new THREE.Vector3());
      const size = box.getSize(new THREE.Vector3());
      const radius = size.length() * 0.5 || 1;
      const distance =
        radius /
        Math.sin(THREE.MathUtils.degToRad(camera.fov / 2)) /
        Math.min(camera.aspect, 1);
      camera.position
        .copy(center)
        .add(
          new THREE.Vector3(0.7, 0.45, 1.4)
            .normalize()
            .multiplyScalar(distance * 1.05),
        );
      controls.target.copy(center);
      camera.near = Math.max(0.001, distance / 1000);
      camera.far = distance * 100;
      camera.updateProjectionMatrix();
      controls.update();
      if (engine.current) engine.current.needsRender = true;
    };
    engine.current = {
      renderer,
      scene,
      camera,
      controls,
      group,
      fit,
      needsRender: true,
    };
    // Wheel and middle-drag handlers update the camera before the animation tick.
    const invalidate = () => {
      if (engine.current) engine.current.needsRender = true;
    };
    controls.addEventListener("change", invalidate);
    const resize = new ResizeObserver(() => {
      const { width, height } = div.getBoundingClientRect();
      renderer.setSize(width, height);
      camera.aspect = width / Math.max(height, 1);
      camera.updateProjectionMatrix();
      if (engine.current) engine.current.needsRender = true;
    });
    resize.observe(div);
    const ray = new THREE.Raycaster();
    let down = [0, 0];
    const pointerdown = (e: PointerEvent) => {
      down = [e.clientX, e.clientY];
    };
    const pointerup = (e: PointerEvent) => {
      if (Math.hypot(e.clientX - down[0], e.clientY - down[1]) > 4) return;
      const b = div.getBoundingClientRect();
      ray.setFromCamera(
        new THREE.Vector2(
          ((e.clientX - b.left) / b.width) * 2 - 1,
          (-(e.clientY - b.top) / b.height) * 2 + 1,
        ),
        camera,
      );
      const hit = ray
        .intersectObjects(group.children, true)
        .find((h) => h.object.userData.label);
      if (hit) selection.current(hit.object.userData.label);
    };
    const lost = (e: Event) => {
      e.preventDefault();
      setError("Graphics context lost. Reload the document to recover.");
    };
    renderer.domElement.addEventListener("pointerdown", pointerdown);
    renderer.domElement.addEventListener("pointerup", pointerup);
    renderer.domElement.addEventListener("webglcontextlost", lost);
    renderer.setAnimationLoop(() => {
      const moved = controls.update();
      if (moved || engine.current?.needsRender) {
        try {
          renderer.render(scene, camera);
        } catch (error) {
          setError(
            `Unable to render the scene: ${String(error)}. Try a lower grid resolution or fewer surfaces.`,
          );
        }
        if (engine.current) engine.current.needsRender = false;
      }
    });
    return () => {
      renderer.setAnimationLoop(null);
      resize.disconnect();
      controls.removeEventListener("change", invalidate);
      controls.dispose();
      dispose(group);
      renderer.dispose();
      div.removeChild(renderer.domElement);
      engine.current = null;
    };
  }, []);
  useEffect(() => {
    const e = engine.current;
    if (!e) return;
    const group = new THREE.Group();
    try {
      for (const [i, atom] of doc.atoms.entries()) {
        const element = elements[atom.element] ?? {
          symbol: `Z${atom.element}`,
          color: "#b294b9",
          radius: 1.8,
        };
        const radius =
          doc.view.representation === "space-fill"
            ? element.radius / BOHR
            : doc.view.representation === "liquorice"
              ? 0.28
              : atom.element === 1
                ? 0.32
                : 0.52;
        const m = new THREE.Mesh(
          new THREE.SphereGeometry(radius, 24, 16),
          new THREE.MeshStandardMaterial({
            color: element.color,
            roughness: 0.34,
          }),
        );
        m.position.set(...atom.position);
        m.userData.label = `${element.symbol} ${i + 1} | ${atom.position.map((v) => (v * BOHR).toFixed(3)).join(", ")} angstrom`;
        group.add(m);
      }
      if (doc.view.representation !== "space-fill")
        for (const [i, j] of doc.bonds) {
          const from = new THREE.Vector3(...doc.atoms[i].position);
          const to = new THREE.Vector3(...doc.atoms[j].position);
          const delta = to.clone().sub(from);
          if (delta.length() < 1e-10) continue;
          const radius = doc.view.representation === "liquorice" ? 0.28 : 0.105;
          const bond = new THREE.Mesh(
            new THREE.CylinderGeometry(radius, radius, delta.length(), 12),
            new THREE.MeshStandardMaterial({
              color: "#9ba3a8",
              roughness: 0.5,
            }),
          );
          bond.position.copy(from).add(to).multiplyScalar(0.5);
          bond.quaternion.setFromUnitVectors(
            new THREE.Vector3(0, 1, 0),
            delta.normalize(),
          );
          group.add(bond);
        }
      for (const s of doc.surfaces) {
        if (
          !s.visible ||
          (mode !== "mesh" && grid && s.field === doc.view.field)
        )
          continue;
        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute(
          "position",
          new THREE.Float32BufferAttribute(s.positions, 3),
        );
        geometry.setAttribute(
          "normal",
          new THREE.Float32BufferAttribute(s.normals, 3),
        );
        geometry.setIndex(s.indices);
        const mesh = new THREE.Mesh(
          geometry,
          new THREE.MeshStandardMaterial({
            color: s.color,
            opacity: s.opacity,
            transparent: s.opacity < 1,
            side: THREE.DoubleSide,
            roughness: 0.45,
            depthWrite: s.opacity >= 1,
          }),
        );
        mesh.userData.label = `${s.label} | ${(s.indices.length / 3).toLocaleString()} triangles`;
        group.add(mesh);
      }
      if (grid && mode !== "mesh")
        group.add(
          volumeObject(
            grid,
            mode,
            doc.view.isovalue,
            doc.view.positive_color,
            doc.view.negative_color,
            doc.view.opacity,
            e.renderer.extensions.has("OES_texture_float_linear"),
          ),
        );
    } catch (error) {
      dispose(group);
      setError(
        `Unable to prepare the new scene: ${String(error)}. The previous scene is still displayed. Try a lower grid resolution or fewer surfaces.`,
      );
      return;
    }
    dispose(e.group);
    e.group.clear();
    e.group.add(group);
    if (!e.renderer.getContext().isContextLost()) setError("");
    e.needsRender = true;
  }, [doc, grid, mode]);
  useEffect(() => {
    engine.current?.fit();
  }, [reset, doc.id]);
  function screenshot() {
    const e = engine.current;
    if (!e) return;
    try {
      e.renderer.render(e.scene, e.camera);
      const a = document.createElement("a");
      a.download = "molekel-view.png";
      a.href = e.renderer.domElement.toDataURL("image/png");
      a.click();
    } catch (error) {
      setError(`Unable to export the scene image: ${String(error)}.`);
    }
  }
  return (
    <div className="scene-wrap">
      <div className="scene" ref={host} data-testid="viewport" />
      {error && (
        <div className="viewport-error" role="alert">
          {error}
        </div>
      )}
      <div className="viewport-tools">
        <button
          title="Zoom in"
          aria-label="Zoom in"
          onClick={() => engine.current?.controls.dollyIn(0.8)}
        >
          <ZoomIn size={18} />
        </button>
        <button
          title="Zoom out"
          aria-label="Zoom out"
          onClick={() => engine.current?.controls.dollyOut(0.8)}
        >
          <ZoomOut size={18} />
        </button>
        <button
          title="Fit scene"
          aria-label="Fit scene"
          onClick={() => engine.current?.fit()}
        >
          <Focus size={18} />
        </button>
        <button
          title="Reset view"
          aria-label="Reset view"
          onClick={() => engine.current?.fit()}
        >
          <RotateCcw size={18} />
        </button>
        <button
          title="Export image"
          aria-label="Export image"
          onClick={screenshot}
        >
          <Camera size={18} />
        </button>
      </div>
      <div className="scene-label">
        <span>
          {mode === "mesh"
            ? "Surface geometry"
            : mode === "raycast"
              ? "Sampled raycast preview"
              : "Volume preview"}
        </span>
        <span>Bohr coordinates</span>
      </div>
    </div>
  );
}
