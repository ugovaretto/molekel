import { useEffect, useRef, useState } from "react";
import {
  ArrowRightLeft,
  FolderPlus,
  Save,
  Square,
  Trash2,
  X,
} from "lucide-react";
import {
  browserSource,
  fileByteLimit,
  molecularExtensions,
  native,
  pickNative,
  saveBytes,
  type SourceFile,
} from "./files";
import { cancel, request } from "./science";
import type { ImportResult } from "./types";

type State =
  | "queued"
  | "converting"
  | "ready"
  | "saving"
  | "saved"
  | "failed"
  | "cancelled";
interface Conversion {
  id: number;
  source: SourceFile;
  state: State;
  output: string;
  bytes?: Uint8Array;
  warnings: string[];
  message: string;
}
const stateLabels: Record<State, string> = {
  queued: "Queued",
  converting: "Converting",
  ready: "Ready to save",
  saving: "Saving",
  saved: native ? "Saved" : "Download requested",
  failed: "Failed",
  cancelled: "Cancelled",
};
const queueLimit = 32;

export function ConvertFiles({
  open,
  onClose,
}: {
  open: boolean;
  onClose: () => void;
}) {
  const dialog = useRef<HTMLDialogElement>(null);
  const picker = useRef<HTMLInputElement>(null);
  const nextId = useRef(0);
  const run = useRef(0);
  const [items, setItems] = useState<Conversion[]>([]);
  const [running, setRunning] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const [progress, setProgress] = useState("");
  const busy = running || saving;

  useEffect(() => {
    if (open) dialog.current?.showModal();
    else dialog.current?.close();
  }, [open]);

  function update(id: number, values: Partial<Conversion>) {
    setItems((previous) =>
      previous.map((item) => (item.id === id ? { ...item, ...values } : item)),
    );
  }
  function addFiles(sources: SourceFile[]) {
    if (items.length + sources.length > queueLimit) {
      setError(`A conversion queue can contain at most ${queueLimit} files.`);
      return;
    }
    setError("");
    setItems((previous) => [
      ...previous,
      ...sources.map(
        (source): Conversion => ({
          id: ++nextId.current,
          source,
          state: "queued",
          output: `${source.name.replace(/\.(?:molden\.input|[^.]+)$/i, "").replace(/[^a-z0-9._ -]/gi, "_") || "converted"}.eigenvista`,
          warnings: [],
          message: "",
        }),
      ),
    ]);
  }
  async function chooseFiles() {
    if (!native) {
      picker.current?.click();
      return;
    }
    try {
      addFiles(await pickNative(true));
    } catch (e) {
      setError(String(e));
    }
  }
  async function convert() {
    const pending = items.filter((item) =>
      ["queued", "failed", "cancelled"].includes(item.state),
    );
    const token = ++run.current;
    setRunning(true);
    setError("");
    let retainedBytes = items.reduce(
      (sum, item) => sum + (item.bytes?.length ?? 0),
      0,
    );
    let completed = 0;
    for (const item of pending) {
      if (token !== run.current) break;
      update(item.id, { state: "converting", message: "", warnings: [] });
      setProgress(
        `Converting ${completed + 1} of ${pending.length}: ${item.source.name}`,
      );
      try {
        const bytes = await item.source.read();
        if (token !== run.current) break;
        if (bytes.length > fileByteLimit)
          throw new Error("File exceeds the 128 MiB preview budget");
        const result = await request<ImportResult>("import_document", {
          bytes,
          name: item.source.name,
        });
        if (token !== run.current) break;
        const output = await request<Uint8Array>("encode", {
          doc: result.document,
        });
        if (token !== run.current) break;
        if (retainedBytes + output.length > fileByteLimit)
          throw new Error(
            "Converted results exceed the 128 MiB queue budget. Save ready files, then retry.",
          );
        retainedBytes += output.length;
        update(item.id, {
          state: "ready",
          bytes: output,
          warnings: result.report.warnings,
        });
      } catch (e) {
        if (token !== run.current) break;
        update(item.id, { state: "failed", message: String(e) });
      }
      completed++;
    }
    if (token === run.current) {
      setRunning(false);
      setProgress(
        `Processed ${completed} ${completed === 1 ? "file" : "files"}`,
      );
    }
  }
  function stop() {
    ++run.current;
    cancel();
    setItems((previous) =>
      previous.map((item) =>
        ["queued", "converting"].includes(item.state)
          ? { ...item, state: "cancelled" }
          : item,
      ),
    );
    setRunning(false);
    setProgress("Conversion cancelled; completed results retained");
  }
  async function save(item: Conversion) {
    if (!item.bytes) return;
    setSaving(true);
    update(item.id, { state: "saving", message: "" });
    try {
      if (
        await saveBytes(
          item.bytes,
          item.output,
          items.flatMap((source) =>
            source.source.path ? [source.source.path] : [],
          ),
        )
      )
        update(item.id, { state: "saved", bytes: undefined });
      else update(item.id, { state: "ready", message: "Save cancelled" });
    } catch (e) {
      update(item.id, { state: "ready", message: String(e) });
    } finally {
      setSaving(false);
    }
  }
  const convertible = items.some((item) =>
    ["queued", "failed", "cancelled"].includes(item.state),
  );
  return (
    <dialog
      className="converter"
      ref={dialog}
      aria-labelledby="converter-title"
      onCancel={(event) => {
        event.preventDefault();
        if (!busy) onClose();
      }}
    >
      {open && (
        <>
          <header className="converter-header">
            <h2 id="converter-title">Convert Files</h2>
            <button
              title="Close conversion queue"
              aria-label="Close conversion queue"
              onClick={onClose}
              disabled={busy}
            >
              <X size={17} />
            </button>
          </header>
          <div className="converter-toolbar">
            <button onClick={chooseFiles} disabled={busy}>
              <FolderPlus size={16} />
              Add files
            </button>
            <span>
              {items.length} / {queueLimit} files
            </span>
            <button
              title="Clear conversion queue"
              aria-label="Clear conversion queue"
              disabled={busy || !items.length}
              onClick={() => {
                setItems([]);
                setError("");
                setProgress("");
              }}
            >
              <Trash2 size={16} />
            </button>
          </div>
          <input
            ref={picker}
            className="file-input"
            type="file"
            multiple
            accept={molecularExtensions}
            aria-label="Files to convert"
            onChange={(event) => {
              const sources = Array.from(event.target.files ?? []).map(
                browserSource,
              );
              event.target.value = "";
              addFiles(sources);
            }}
          />
          {error && (
            <div className="conversion-error" role="alert">
              {error}
            </div>
          )}
          <div className="conversion-list">
            {items.length === 0 && <p className="empty">No files selected</p>}
            {items.map((item) => (
              <div className={`conversion-item ${item.state}`} key={item.id}>
                <div className="conversion-file">
                  <strong>{item.source.name}</strong>
                  <span>{item.output}</span>
                  {item.message && <p>{item.message}</p>}
                  {item.warnings.length > 0 && (
                    <details className="conversion-warnings">
                      <summary>
                        {item.warnings.length} import{" "}
                        {item.warnings.length === 1 ? "warning" : "warnings"}
                      </summary>
                      <ul>
                        {item.warnings.map((warning, index) => (
                          <li key={index}>{warning}</li>
                        ))}
                      </ul>
                    </details>
                  )}
                </div>
                <span className="conversion-state">
                  {stateLabels[item.state]}
                </span>
                <button
                  title={`Save ${item.output}`}
                  aria-label={`Save ${item.output}`}
                  disabled={busy || item.state !== "ready"}
                  onClick={() => save(item)}
                >
                  <Save size={16} />
                </button>
              </div>
            ))}
          </div>
          <div className="converter-footer">
            <span role="status">{progress}</span>
            {running ? (
              <button onClick={stop}>
                <Square size={15} />
                Cancel conversion
              </button>
            ) : (
              <button
                className="primary"
                disabled={saving || !convertible}
                onClick={convert}
              >
                <ArrowRightLeft size={16} />
                Convert
              </button>
            )}
          </div>
        </>
      )}
    </dialog>
  );
}
