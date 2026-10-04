import init, * as core from "./wasm/molekel_wasm";
import type { MolekelDocument, SampledGrid } from "./types";

const ready = init();
let cached: { key: string; field: core.SampledField } | null = null;
function clearCache() {
  cached?.field.free();
  cached = null;
}
self.onmessage = async (event: MessageEvent) => {
  const { id, action, args } = event.data;
  const progress = (stage: string) => self.postMessage({ id, progress: stage });
  try {
    await ready;
    let result: unknown;
    const transfer: Transferable[] = [];
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
      case "validate":
        progress("validating");
        // Check source identity and native container budgets before publication.
        core.encode(JSON.stringify(args.doc));
        result = null;
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
        if (cached?.key !== key) {
          clearCache();
          progress("sampling");
          cached = {
            key,
            field: new core.SampledField(json, args.field, args.resolution),
          };
        }
        if (action === "generate") progress("meshing");
        const surfaces =
          action === "generate"
            ? JSON.parse(cached.field.surfaces(json, args.iso))
            : null;
        const grid: SampledGrid = {
          ...JSON.parse(cached.field.metadata()),
          values: cached.field.display_values(),
        };
        transfer.push(grid.values.buffer);
        result = action === "sample" ? grid : { grid, surfaces };
        break;
      }
      default:
        throw new Error(`Unknown worker action ${action}`);
    }
    self.postMessage({ id, result }, { transfer });
  } catch (e) {
    // A trapped allocator/runtime must not be reused for the next calculation.
    const fatal =
      e instanceof WebAssembly.RuntimeError || e instanceof RangeError;
    if (!fatal) clearCache();
    self.postMessage({
      id,
      fatal,
      error: fatal
        ? "The calculation worker exhausted memory or failed. Reduce the grid resolution and retry; previous surfaces were retained."
        : String(e),
    });
  }
};
