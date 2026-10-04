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
  ArrowRightLeft,
  ListChecks,
  X,
} from "lucide-react";
import { request, cancel } from "./science";
import {
  browserSource,
  fileByteLimit,
  molecularExtensions,
  native,
  pickNative,
  saveBytes,
  type SourceFile,
} from "./files";
import { ConvertFiles } from "./ConvertFiles";
import { OrbitalBrowser, maxOrbitalBatch } from "./OrbitalBrowser";
import { gridResolutions, isGridResolution } from "./resolution";
import { Viewport } from "./Viewport";
import type {
  MolekelDocument,
  Generation,
  SampledGrid,
  ImportReport,
  ImportResult,
  RenderMode,
  Surface,
} from "./types";

export default function App() {
  const [doc, setDoc] = useState<MolekelDocument | null>(null);
  const [busy, setBusy] = useState("Opening scientific core");
  const [error, setError] = useState("");
  const [status, setStatus] = useState("");
  const [resolution, setResolution] = useState(40);
  const [mode, setMode] = useState<RenderMode>("mesh");
  const [grid, setGrid] = useState<SampledGrid | null>(null);
  const [reset, setReset] = useState(0);
  const [inspected, setInspected] = useState("");
  const [dirty, setDirty] = useState(false);
  const [importReport, setImportReport] = useState<ImportReport | null>(null);
  const [converterOpen, setConverterOpen] = useState(false);
  const [orbitalsOpen, setOrbitalsOpen] = useState(false);
  const [protectedSourcePath, setProtectedSourcePath] = useState<
    string | undefined
  >();
  const file = useRef<HTMLInputElement>(null);
  const generation = useRef(0);
  const current = useRef(doc);
  const unsaved = useRef(dirty);
  current.current = doc;
  unsaved.current = dirty;
  async function compute(
    document: MolekelDocument,
    res = resolution,
    orbitals?: string[],
  ) {
    if (!document.view.field) return;
    if (!isGridResolution(res)) {
      setError("Choose an available grid resolution.");
      return;
    }
    const fields = orbitals ?? [document.view.field];
    if (!fields.length) return;
    if (
      orbitals &&
      (fields.length > maxOrbitalBatch ||
        new Set(fields).size !== fields.length ||
        fields.some((id) => !document.orbitals.some((o) => o.id === id)))
    ) {
      setError(
        `Select at most ${maxOrbitalBatch} distinct orbitals per generation.`,
      );
      return;
    }
    const token = ++generation.current;
    setBusy("Calculating field and surfaces");
    setError("");
    const started = performance.now();
    try {
      const retained = document.surfaces.filter(
        (s) => !s.field || !fields.includes(s.field),
      );
      const generated: Surface[] = [];
      let activeGrid: SampledGrid | null = null;
      const meshBytes = (s: Surface) =>
        8 * (s.positions.length + s.normals.length) + 4 * s.indices.length;
      let bytes = retained.reduce((total, s) => total + meshBytes(s), 0);
      for (const [index, field] of fields.entries()) {
        const description = orbitals
          ? `Calculating orbital ${index + 1} of ${fields.length}: ${document.orbitals.find((o) => o.id === field)!.label}`
          : "Calculating field and surfaces";
        setBusy(description);
        const result = await request<Generation>(
          "generate",
          {
            doc: { ...document, surfaces: [] },
            field,
            resolution: res,
            iso: document.view.isovalue,
          },
          (stage) => {
            if (token !== generation.current) return;
            if (stage === "sampling")
              setBusy(`${description} / Sampling field`);
            if (stage === "meshing")
              setBusy(`${description} / Extracting meshes`);
          },
        );
        if (token !== generation.current) return;
        generated.push(...result.surfaces);
        bytes += result.surfaces.reduce((total, s) => total + meshBytes(s), 0);
        if (retained.length + generated.length > 128 || bytes > fileByteLimit)
          throw new Error(
            "Generated geometry exceeds the preview budget. Select fewer orbitals or lower the grid resolution; previous surfaces were retained.",
          );
        if (field === document.view.field) activeGrid = result.grid;
      }
      // Publish the complete batch only after Rust validates its combined document.
      await request(
        "validate",
        { doc: { ...document, surfaces: [...retained, ...generated] } },
        () => {
          if (token === generation.current)
            setBusy("Calculating surfaces / Checking saved geometry");
        },
      );
      if (token !== generation.current) return;
      setDoc((d) =>
        d
          ? {
              ...d,
              view: {
                ...d.view,
                field: document.view.field,
                isovalue: document.view.isovalue,
              },
              surfaces: [
                ...d.surfaces.filter(
                  (s) => !s.field || !fields.includes(s.field),
                ),
                ...generated.map((s) => ({
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
      setGrid(activeGrid);
      if (orbitals) {
        setMode("mesh");
        setResolution(res);
      }
      setReset((v) => v + 1);
      setDirty(true);
      setStatus(
        `${orbitals ? `${fields.length} orbitals / ` : ""}${generated.reduce((n, s) => n + s.indices.length / 3, 0).toLocaleString()} triangles / ${((performance.now() - started) / 1000).toFixed(2)} s`,
      );
    } catch (e) {
      if (token === generation.current) setError(String(e));
    } finally {
      if (token === generation.current) setBusy("");
    }
  }
  function generateOrbitals(ids: string[], iso: number, res: number) {
    if (!doc || busy || !ids.length) return;
    if (!Number.isFinite(iso) || iso <= 0 || !isGridResolution(res)) {
      setError(
        "Choose a finite positive isovalue and an available grid resolution.",
      );
      return;
    }
    const field = ids.includes(doc.view.field ?? "") ? doc.view.field : ids[0];
    setOrbitalsOpen(false);
    void compute(
      { ...doc, view: { ...doc.view, field, isovalue: iso } },
      res,
      ids,
    );
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
      setOrbitalsOpen(false);
      setGrid(null);
      setMode("mesh");
      setDirty(false);
      setInspected("");
      setImportReport(null);
      setProtectedSourcePath(undefined);
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
  async function prepareGrid(
    document: MolekelDocument,
    res: number,
    nextMode: RenderMode,
  ) {
    if (!document.view.field) return;
    const token = ++generation.current;
    setBusy("Preparing sampled field");
    setError("");
    try {
      const sampled = await request<SampledGrid>("sample", {
        doc: { ...document, surfaces: [] },
        field: document.view.field,
        resolution: res,
      });
      if (token !== generation.current) return;
      setGrid(sampled);
      setMode(nextMode);
      setReset((v) => v + 1);
      setStatus("Sampled field ready");
    } catch (e) {
      if (token === generation.current) setError(String(e));
    } finally {
      if (token === generation.current) setBusy("");
    }
  }
  async function selectField(id: string) {
    if (!doc || id === doc.view.field) return;
    editView({ field: id });
    setGrid(null);
    setMode("mesh");
    if (doc.grids.some((g) => g.id === id))
      await prepareGrid(
        { ...doc, view: { ...doc.view, field: id } },
        resolution,
        "volume",
      );
  }
  async function load(source: SourceFile) {
    const token = ++generation.current;
    cancel();
    setBusy("Opening document");
    setError("");
    try {
      const bytes = await source.read();
      if (token !== generation.current) return;
      if (bytes.length > fileByteLimit)
        throw new Error("File exceeds the 128 MiB preview budget");
      const result = await request<ImportResult>("import_document", {
        bytes,
        name: source.name,
      });
      if (token !== generation.current) return;
      const d = result.document;
      let sampled: SampledGrid | null = null;
      let previewError = "";
      if (d.grids.some((g) => g.id === d.view.field)) {
        try {
          sampled = await request<SampledGrid>("sample", {
            doc: { ...d, surfaces: [] },
            field: d.view.field,
            resolution,
          });
        } catch (e) {
          previewError = `Sampled preview unavailable: ${String(e)}`;
        }
      }
      if (token !== generation.current) return;
      if (
        unsaved.current &&
        !window.confirm(
          "Replace the current document? Unsaved changes will be lost.",
        )
      )
        return;
      setDoc(d);
      setOrbitalsOpen(false);
      setGrid(sampled);
      setMode(sampled && !d.surfaces.length ? "volume" : "mesh");
      setReset((v) => v + 1);
      setDirty(result.report.requires_save);
      setProtectedSourcePath(
        result.report.format !== "molekel" ? source.path : undefined,
      );
      setInspected("");
      setError(previewError);
      setImportReport(result.report.warnings.length ? result.report : null);
      setStatus(
        `Opened ${source.name}${d.surfaces.length ? " / saved geometry restored" : ""}`,
      );
    } catch (e) {
      if (token === generation.current) setError(String(e));
    } finally {
      if (token === generation.current) setBusy("");
    }
  }
  async function openFile() {
    if (!native) {
      file.current?.click();
      return;
    }
    const token = generation.current;
    try {
      const [f] = await pickNative();
      if (f && token === generation.current) await load(f);
    } catch (e) {
      if (token === generation.current) setError(String(e));
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
          protectedSourcePath ? [protectedSourcePath] : [],
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
            onClick={() => setConverterOpen(true)}
            disabled={!!busy}
            title="Convert Files"
            aria-label="Convert Files"
          >
            <ArrowRightLeft size={17} />
            <span>Convert</span>
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
          accept={molecularExtensions}
          aria-label="Open molecular file"
          onChange={async (e) => {
            const f = e.target.files?.[0];
            e.target.value = "";
            if (f) await load(browserSource(f));
          }}
        />
      </header>
      <ConvertFiles
        open={converterOpen}
        onClose={() => setConverterOpen(false)}
      />
      {orbitalsOpen && doc && (
        <OrbitalBrowser
          document={doc}
          resolution={resolution}
          onClose={() => setOrbitalsOpen(false)}
          onGenerate={generateOrbitals}
        />
      )}
      {error && (
        <div className="error" role="alert">
          {error}
          <button onClick={() => setError("")} aria-label="Dismiss error">
            Close
          </button>
        </div>
      )}
      {importReport && (
        <div className="import-report">
          <details>
            <summary>
              {importReport.format.toUpperCase()}:{" "}
              {importReport.warnings.length} import{" "}
              {importReport.warnings.length === 1 ? "warning" : "warnings"}
            </summary>
            <ul>
              {importReport.warnings.map((warning, index) => (
                <li key={index}>{warning}</li>
              ))}
            </ul>
          </details>
          <button
            aria-label="Dismiss import warnings"
            title="Dismiss import warnings"
            onClick={() => setImportReport(null)}
          >
            <X size={15} />
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
                      {group.name === "Orbitals" && (
                        <button
                          className="browse-orbitals"
                          aria-label="Browse orbitals"
                          title="Browse and select orbitals"
                          disabled={!!busy}
                          onClick={() => setOrbitalsOpen(true)}
                        >
                          <ListChecks size={16} />
                          Orbitals ({doc.orbitals.length})
                        </button>
                      )}
                      <div
                        className={
                          group.name === "Orbitals"
                            ? "orbital-field-list"
                            : undefined
                        }
                      >
                        {group.fields.map((f) => (
                          <button
                            className={`field-row ${doc.view.field === f.id ? "selected" : ""}`}
                            key={f.id}
                            disabled={!!busy}
                            onClick={() => void selectField(f.id)}
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
                  disabled={!!busy}
                  onChange={(e) => {
                    setMode(e.target.value as RenderMode);
                    setReset((v) => v + 1);
                  }}
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
                  onChange={(e) => {
                    const value = Number(e.target.value);
                    if (!isGridResolution(value)) return;
                    setResolution(value);
                    if (doc.grids.some((g) => g.id === doc.view.field))
                      void prepareGrid(doc, value, mode);
                  }}
                >
                  {gridResolutions.map((n) => (
                    <option key={n} value={n} title={`${n} x ${n} x ${n}`}>
                      {n}
                      {"\u00b3"}
                    </option>
                  ))}
                </select>
              </label>
              <button
                className="primary generate"
                disabled={
                  busy ? !busy.startsWith("Calculating") : !doc.view.field
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
