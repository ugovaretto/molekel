import init, * as core from "./wasm/molekel_wasm";
import type { MolekelDocument, Grid } from "./types";

const ready = init();
let cached: { key: string; grid: Grid } | null = null;
self.onmessage = async (event: MessageEvent) => {
  const { id, action, args } = event.data;
  try {
    await ready;
    let result: unknown;
    switch (action) {
      case "example":
        result = JSON.parse(core.example(args.openShell));
        break;
      case "decode":
        result = JSON.parse(core.decode(new Uint8Array(args.bytes)));
        break;
      case "encode":
        result = core.encode(JSON.stringify(args.doc));
        break;
      case "import":
        result = JSON.parse(core.import_text(args.text, args.name));
        break;
      case "import_document":
        result = JSON.parse(
          core.import_document(new Uint8Array(args.bytes), args.name),
        );
        break;
      case "sample":
      case "generate": {
        const doc = args.doc as MolekelDocument;
        const json = JSON.stringify(doc);
        core.validate(json);
        const key = JSON.stringify([
          doc.basis,
          doc.orbitals,
          doc.densities,
          doc.grids,
          args.field,
          args.resolution,
        ]);
        if (cached?.key !== key)
          cached = {
            key,
            grid: JSON.parse(core.sample(json, args.field, args.resolution)),
          };
        result =
          action === "sample"
            ? cached!.grid
            : {
                grid: cached!.grid,
                surfaces: JSON.parse(
                  core.surfaces(
                    json,
                    JSON.stringify(cached!.grid),
                    args.field,
                    args.iso,
                  ),
                ),
              };
        break;
      }
      default:
        throw new Error(`Unknown worker action ${action}`);
    }
    self.postMessage({ id, result });
  } catch (e) {
    self.postMessage({ id, error: String(e) });
  }
};
