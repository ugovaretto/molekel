let worker: Worker | null = null;
let nextId = 0;
const pending = new Map<
  number,
  { resolve: (v: unknown) => void; reject: (e: Error) => void }
>();
function start() {
  worker = new Worker(new URL("./science.worker.ts", import.meta.url), {
    type: "module",
  });
  worker.onmessage = ({ data }) => {
    const p = pending.get(data.id);
    if (!p) return;
    pending.delete(data.id);
    if (data.error) p.reject(new Error(data.error));
    else p.resolve(data.result);
  };
  worker.onerror = (event) =>
    cancel(new Error(event.message || "Scientific worker failed"));
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
): Promise<T> {
  if (!worker) start();
  const id = ++nextId;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve: (v) => resolve(v as T), reject });
    worker!.postMessage({ id, action, args });
  });
}
