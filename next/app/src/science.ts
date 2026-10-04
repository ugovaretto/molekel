let worker: Worker | null = null;
let nextId = 0;
const pending = new Map<
  number,
  {
    resolve: (v: unknown) => void;
    reject: (e: Error) => void;
    onProgress?: (stage: string) => void;
  }
>();
function start() {
  const instance = new Worker(new URL("./science.worker.ts", import.meta.url), {
    type: "module",
  });
  worker = instance;
  instance.onmessage = ({ data }) => {
    if (worker !== instance) return;
    const p = pending.get(data.id);
    if (!p) return;
    if (typeof data.progress === "string") {
      p.onProgress?.(data.progress);
      return;
    }
    pending.delete(data.id);
    if (data.error) p.reject(new Error(data.error));
    else p.resolve(data.result);
  };
  instance.onerror = (event) => {
    if (worker === instance)
      cancel(new Error(event.message || "Scientific worker failed"));
  };
}
export function cancel(reason = new Error("Calculation cancelled")) {
  worker?.terminate();
  worker = null;
  for (const p of pending.values()) p.reject(reason);
  pending.clear();
}
export function request<T>(
  action: string,
  args: Record<string, unknown>,
  onProgress?: (stage: string) => void,
): Promise<T> {
  if (!worker) start();
  const id = ++nextId;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve: (v) => resolve(v as T), reject, onProgress });
    worker!.postMessage({ id, action, args });
  });
}
