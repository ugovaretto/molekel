import { useEffect, useRef, useState } from "react";
import {
  Atom,
  FolderOpen,
  Save,
  Play,
  Square,
  Eye,
  EyeOff,
  Trash2,
  FlaskConical,
  Box,
  Check,
  ChevronDown,
} from "lucide-react";
import { request, cancel } from "./science";
import { native, pickNative, saveBytes } from "./files";
import { Viewport } from "./Viewport";
import type { MolekelDocument, Generation, Grid, RenderMode } from "./types";

export default function App() {
  const [doc, setDoc] = useState<MolekelDocument | null>(null);
  const [busy, setBusy] = useState("Opening scientific core");
  const [error, setError] = useState("");
  const [status, setStatus] = useState("");
  const [resolution, setResolution] = useState(40);
  const [mode, setMode] = useState<RenderMode>("mesh");
  const [grid, setGrid] = useState<Grid | null>(null);
  const [reset, setReset] = useState(0);
  const [inspected, setInspected] = useState("");
  const [dirty, setDirty] = useState(false);
  const file = useRef<HTMLInputElement>(null);
  const generation = useRef(0);
  const current = useRef(doc);
  current.current = doc;
  async function compute(document: MolekelDocument, res = resolution) {
    if (!document.view.field) return;
    const token = ++generation.current;
    setBusy("Calculating field and surfaces");
    setError("");
    const started = performance.now();
    try {
      const result = await request<Generation>("generate", {
        doc: { ...document, surfaces: [] },
        field: document.view.field,
        resolution: res,
        iso: document.view.isovalue,
      });
      if (token !== generation.current) return;
      setDoc((d) =>
        d
          ? {
              ...d,
              surfaces: [
                ...d.surfaces.filter((s) => s.field !== document.view.field),
                ...result.surfaces.map((s) => ({
                  ...s,
                  opacity: d.view.opacity,
                  color:
                    (s.isovalue ?? 0) > 0
                      ? d.view.positive_color
                      : d.view.negative_color,
                })),
              ],
            }
          : d,
      );
      setGrid(result.grid);
      setReset((v) => v + 1);
      setDirty(true);
      setStatus(
        `${result.surfaces.reduce((n, s) => n + s.indices.length / 3, 0).toLocaleString()} triangles / ${((performance.now() - started) / 1000).toFixed(2)} s`,
      );
    } catch (e) {
      if (token === generation.current) setError(String(e));
    } finally {
      if (token === generation.current) setBusy("");
    }
  }
  async function example(openShell = false) {
    if (
      dirty &&
      !window.confirm(
        "Replace the current document? Unsaved changes will be lost.",
      )
    )
      return;
    ++generation.current;
    cancel();
    setBusy("Opening example");
    setError("");
    try {
      const d = await request<MolekelDocument>("example", { openShell });
      setDoc(d);
      setGrid(null);
      setMode("mesh");
      setDirty(false);
      setInspected("");
      await compute(d);
    } catch (e) {
      setError(String(e));
      setBusy("");
    }
  }
  useEffect(() => {
    void example();
    return () => {
      ++generation.current;
      cancel();
    };
  }, []);
  useEffect(() => {
    const guard = (e: BeforeUnloadEvent) => {
      if (dirty) {
        e.preventDefault();
      }
    };
    window.addEventListener("beforeunload", guard);
    return () => window.removeEventListener("beforeunload", guard);
  }, [dirty]);
  function editView(values: Partial<MolekelDocument["view"]>) {
    setDoc((d) =>
      d
        ? {
            ...d,
            view: { ...d.view, ...values },
            surfaces: d.surfaces.map((s) => ({
              ...s,
              ...(values.opacity !== undefined
                ? { opacity: values.opacity }
                : {}),
              ...(values.positive_color && (s.isovalue ?? 0) > 0
                ? { color: values.positive_color }
                : {}),
              ...(values.negative_color && (s.isovalue ?? 0) < 0
                ? { color: values.negative_color }
                : {}),
            })),
          }
        : d,
    );
    setDirty(true);
  }
  async function load(name: string, bytes: Uint8Array) {
    if (
      dirty &&
      !window.confirm(
        "Replace the current document? Unsaved changes will be lost.",
      )
    )
      return;
    ++generation.current;
    cancel();
    setBusy("Opening document");
    setError("");
    try {
      if (bytes.length > 128 * 1024 * 1024)
        throw new Error("File exceeds the 128 MiB preview budget");
      const d = await request<MolekelDocument>(
        name.toLowerCase().endsWith(".molekel") ? "decode" : "import",
        {
          bytes,
          text: name.endsWith(".molekel")
            ? ""
            : new TextDecoder().decode(bytes),
          name,
        },
      );
      setDoc(d);
      setGrid(null);
      setMode("mesh");
      setReset((v) => v + 1);
      setDirty(false);
      setInspected("");
      setStatus(
        `Opened ${name}${d.surfaces.length ? " / saved geometry restored" : ""}`,
      );
    } catch (e) {
      setError(String(e));
    } finally {
      setBusy("");
    }
  }
  async function openFile() {
    if (!native) {
      file.current?.click();
      return;
    }
    try {
      const f = await pickNative();
      if (f) await load(f.name, f.bytes);
    } catch (e) {
      setError(String(e));
    }
  }
  async function save() {
    if (!doc) return;
    setBusy("Saving document");
    setError("");
    const snapshot = doc;
    try {
      const bytes = await request<Uint8Array>("encode", { doc });
      if (
        await saveBytes(
          bytes,
          `${doc.title.replace(/[^a-z0-9-]+/gi, "-").toLowerCase()}.molekel`,
        )
      ) {
        if (current.current === snapshot) setDirty(false);
        setStatus("Saved quantum data, surfaces and appearance");
      }
    } catch (e) {
      setError(String(e));
    } finally {
      setBusy("");
    }
  }
  function stop() {
    ++generation.current;
    cancel();
    setBusy("");
    setStatus("Cancelled; previous surfaces retained");
  }
  const fieldLabel = doc
    ? [...doc.orbitals, ...doc.densities, ...doc.grids].find(
        (f) => f.id === doc.view.field,
      )?.label
    : "";
  return (
    <div className="application">
      <header className="topbar">
        <div className="brand">
          <Atom size={27} strokeWidth={1.5} />
          <h1>Molekel</h1>
          <span className="build-label">PREVIEW</span>
        </div>
        <div className="document-title" title={doc?.title}>
          {doc?.title ?? "New document"}
          {dirty && <span className="dirty" aria-label="Unsaved changes" />}
        </div>
        <div className="file-actions">
          <button onClick={openFile} disabled={!!busy} title="Open document">
            <FolderOpen size={17} />
            <span>Open</span>
          </button>
          <button
            onClick={save}
            disabled={!doc || !!busy}
            title="Save document"
          >
            <Save size={17} />
            <span>Save</span>
          </button>
        </div>
        <input
          ref={file}
          className="file-input"
          type="file"
          accept=".molekel,.xyz,.pdb,.cube,.cub"
          onChange={async (e) => {
            const f = e.target.files?.[0];
            e.target.value = "";
            if (f) {
              if (f.size > 128 * 1024 * 1024) {
                setError("File exceeds the 128 MiB preview budget");
                return;
              }
              await load(f.name, new Uint8Array(await f.arrayBuffer()));
            }
          }}
        />
      </header>
      {error && (
        <div className="error" role="alert">
          {error}
          <button onClick={() => setError("")} aria-label="Dismiss error">
            Close
          </button>
        </div>
      )}
      {doc ? (
        <main>
          <aside className="left-panel">
            <section>
              <div className="section-title">
                <h2>Document</h2>
                <details className="examples">
                  <summary title="Open example">
                    <FlaskConical size={16} />
                    <ChevronDown size={13} />
                  </summary>
                  <div>
                    <button disabled={!!busy} onClick={() => example(false)}>
                      Hydrogen pair
                    </button>
                    <button disabled={!!busy} onClick={() => example(true)}>
                      Fractional open shell
                    </button>
                  </div>
                </details>
              </div>
              <div className="document-summary">
                <Atom size={19} />
                <div>
                  <strong>{doc.atoms.length} atoms</strong>
                  <span>
                    {doc.bonds.length}{" "}
                    {doc.bonds.length === 1 ? "bond" : "bonds"}
                  </span>
                  <span>{doc.basis.length} basis functions</span>
                </div>
              </div>
              <label>
                Representation
                <select
                  aria-label="Representation"
                  value={doc.view.representation}
                  onChange={(e) => editView({ representation: e.target.value })}
                >
                  <option value="ball-stick">Ball and stick</option>
                  <option value="liquorice">Liquorice</option>
                  <option value="space-fill">Van der Waals</option>
                </select>
              </label>
            </section>
            <section className="field-section">
              <h2>Quantum fields</h2>
              {[
                { name: "Orbitals", fields: doc.orbitals },
                { name: "Density matrices", fields: doc.densities },
                { name: "Sampled fields", fields: doc.grids },
              ].map(
                (group) =>
                  group.fields.length > 0 && (
                    <div className="field-group" key={group.name}>
                      <h3>{group.name}</h3>
                      {group.fields.map((f) => (
                        <button
                          className={`field-row ${doc.view.field === f.id ? "selected" : ""}`}
                          key={f.id}
                          disabled={!!busy}
                          onClick={() => {
                            editView({ field: f.id });
                            setGrid(null);
                            setMode("mesh");
                          }}
                        >
                          <span className="field-dot" />
                          <span>
                            {f.label}
                            <small>
                              {"occupation" in f
                                ? `${f.spin} / occ. ${f.occupation ?? "unknown"}`
                                : "kind" in f
                                  ? f.kind
                                  : f.quantity}
                            </small>
                          </span>
                          {doc.view.field === f.id && <Check size={15} />}
                        </button>
                      ))}
                    </div>
                  ),
              )}
              {!doc.view.field && <p className="empty">No quantum fields</p>}
            </section>
            <section className="provenance">
              <h2>Provenance</h2>
              {doc.provenance.map((p, i) => (
                <p key={i}>{p}</p>
              ))}
            </section>
          </aside>
          <div className="viewport-column">
            <Viewport
              doc={doc}
              grid={grid}
              mode={mode}
              reset={reset}
              onSelect={setInspected}
            />
            <div className="selection-status">
              {inspected || fieldLabel || doc.title}
              <span>{doc.surfaces.length} saved surfaces</span>
            </div>
          </div>
          <aside className="right-panel">
            <section>
              <h2>Field rendering</h2>
              <label>
                Mode
                <select
                  aria-label="Rendering mode"
                  value={mode}
                  onChange={(e) => setMode(e.target.value as RenderMode)}
                >
                  <option value="mesh">Isosurface mesh</option>
                  <option value="raycast" disabled={!grid}>
                    Sampled raycast preview
                  </option>
                  <option value="volume" disabled={!grid}>
                    Volume preview
                  </option>
                </select>
              </label>
              <label>
                Isovalue
                <input
                  aria-label="Isovalue"
                  type="number"
                  min="0.00001"
                  step="0.01"
                  value={doc.view.isovalue}
                  disabled={!!busy || !doc.view.field}
                  onChange={(e) => {
                    const v = Number(e.target.value);
                    if (Number.isFinite(v) && v > 0) editView({ isovalue: v });
                  }}
                />
              </label>
              <div className="units">
                {doc.orbitals.some((o) => o.id === doc.view.field)
                  ? "bohr^-3/2"
                  : doc.densities.some((o) => o.id === doc.view.field)
                    ? "electrons / bohr^3"
                    : "Scalar units unspecified"}
              </div>
              <label>
                Grid resolution
                <select
                  aria-label="Grid resolution"
                  value={resolution}
                  disabled={!!busy}
                  onChange={(e) => setResolution(Number(e.target.value))}
                >
                  {[24, 32, 40, 48].map((n) => (
                    <option key={n} value={n}>
                      {n} x {n} x {n}
                    </option>
                  ))}
                </select>
              </label>
              <button
                className="primary generate"
                disabled={
                  !doc.view.field || (!!busy && !busy.startsWith("Calculating"))
                }
                onClick={() => (busy ? stop() : compute(doc))}
              >
                {busy.startsWith("Calculating") ? (
                  <>
                    <Square size={15} />
                    Cancel
                  </>
                ) : (
                  <>
                    <Play size={15} />
                    Generate surfaces
                  </>
                )}
              </button>
            </section>
            <section>
              <h2>Appearance</h2>
              <div className="color-pair">
                <label>
                  <input
                    aria-label="Positive color"
                    type="color"
                    value={doc.view.positive_color}
                    onChange={(e) =>
                      editView({ positive_color: e.target.value })
                    }
                  />
                  Positive
                </label>
                <label>
                  <input
                    aria-label="Negative color"
                    type="color"
                    value={doc.view.negative_color}
                    onChange={(e) =>
                      editView({ negative_color: e.target.value })
                    }
                  />
                  Negative
                </label>
              </div>
              <label className="range-label">
                Opacity<output>{Math.round(doc.view.opacity * 100)}%</output>
                <input
                  aria-label="Opacity"
                  type="range"
                  min="0"
                  max="1"
                  step="0.01"
                  value={doc.view.opacity}
                  onChange={(e) =>
                    editView({ opacity: Number(e.target.value) })
                  }
                />
              </label>
            </section>
            <section className="surfaces">
              <h2>
                Saved surfaces <span>{doc.surfaces.length}</span>
              </h2>
              {doc.surfaces.length === 0 && (
                <p className="empty">No surfaces</p>
              )}
              {doc.surfaces.map((s) => (
                <div className="surface-row" key={s.id}>
                  <button
                    aria-label={`Toggle ${s.label}`}
                    title="Toggle visibility"
                    onClick={() => {
                      setDoc({
                        ...doc,
                        surfaces: doc.surfaces.map((x) =>
                          x.id === s.id ? { ...x, visible: !x.visible } : x,
                        ),
                      });
                      setDirty(true);
                    }}
                  >
                    {s.visible ? <Eye size={16} /> : <EyeOff size={16} />}
                  </button>
                  <span
                    className="surface-swatch"
                    style={{ background: s.color }}
                  />
                  <div>
                    <strong>
                      {s.isovalue === null
                        ? s.label
                        : `${s.field} ${s.isovalue > 0 ? "+" : ""}${s.isovalue}`}
                    </strong>
                    <small>
                      {(s.indices.length / 3).toLocaleString()} triangles
                    </small>
                  </div>
                  <button
                    aria-label={`Delete ${s.label}`}
                    title="Delete surface"
                    disabled={!!busy}
                    onClick={() => {
                      setDoc({
                        ...doc,
                        surfaces: doc.surfaces.filter((x) => x.id !== s.id),
                      });
                      setDirty(true);
                    }}
                  >
                    <Trash2 size={14} />
                  </button>
                </div>
              ))}
            </section>
          </aside>
        </main>
      ) : (
        <div className="loading">
          <Atom size={36} />
          <p>{busy || "Unable to open the scientific core"}</p>
        </div>
      )}
      <footer>
        <span className={busy ? "activity active" : "activity"} />
        <span role="status">{busy || status || "Ready"}</span>
        <span className="runtime">
          <Box size={13} />
          Rust / {native ? "macOS desktop" : "WebAssembly"}
        </span>
      </footer>
    </div>
  );
}
