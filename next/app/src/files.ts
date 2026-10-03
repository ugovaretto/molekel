import { invoke, isTauri } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-dialog";
import { readFile } from "@tauri-apps/plugin-fs";

export const native = isTauri();
export async function pickNative(): Promise<{
  name: string;
  bytes: Uint8Array;
} | null> {
  const path = await open({
    multiple: false,
    filters: [
      {
        name: "Molecular documents",
        extensions: ["molekel", "xyz", "pdb", "cube", "cub"],
      },
    ],
  });
  if (!path) return null;
  return { name: path.split(/[\\/]/).pop()!, bytes: await readFile(path) };
}
export async function saveBytes(bytes: Uint8Array, name: string) {
  if (native) {
    return invoke<boolean>("save_native", { bytes: Array.from(bytes), name });
  } else {
    const url = URL.createObjectURL(
      new Blob([bytes.slice().buffer], { type: "application/octet-stream" }),
    );
    const a = document.createElement("a");
    a.href = url;
    a.download = name;
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 10000);
  }
  return true;
}
