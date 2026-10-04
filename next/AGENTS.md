# EigenVista agent guide

Read the [root working agreement](../AGENTS.md) first. Work only inside `next/`
for implementation; do not edit legacy source, existing data, or historical
research. Keep the current branch and preserve existing changes. No C++ or
copied legacy implementation; use Rust and the established TypeScript UI.

The active rewrite is **EigenVista**, formerly the Molekel Rust preview. Its
source remains in `next/`; legacy Molekel code, repository URLs, historical
plans, Git tags, and existing scientific fixtures retain their identity. Use
`eigenvista-*` Rust packages, the `eigenvista-convert` CLI, and `.eigenvista`
for new documents. Older `.molekel` preview documents remain readable. See the
[native profile](docs/preview-format.md) before changing format compatibility.

## Orientation

- [Handoff](docs/handoff.md): current work, next gates, and common traps.
- [Architecture](docs/architecture.md): ownership and data contracts.
- [Development](docs/development.md): exact setup/build/test commands.
- [Documentation index](docs/README.md): planning through delivery.
- [User guide](docs/user-guide.md): actual controls, supported files, and limits.

Before running tools, from the repository root:

```sh
mkdir -p tmp
export TMPDIR="$PWD/tmp" TMP="$PWD/tmp" TEMP="$PWD/tmp"
```

Never use system `/tmp`, including for screenshots, test fixtures, or build
staging. Existing project wrappers enforce the same location. Generated WASM,
builds, packages, reports, and node_modules are ignored; retain both lockfiles.

## Change and verification rules

- Keep chemistry, format validation, and import algorithms in the Rust crates.
  Do not duplicate scientific logic in React or shaders without a tested contract.
- Preserve explicit units, basis normalization, density semantics, affine grid
  order, durable saved meshes, and source identity. Update native and WASM
  reference tests when a shared contract changes. Never regenerate expected
  scientific results simply to make a failing test pass.
- Run Rust formatting, scientific/format tests, workspace/all-target Clippy,
  and the production WASM/frontend build for runtime changes. Test the Tauri
  shell when changing native behavior. Commands are in the development guide.
- For UI/rendering changes, also run Chromium/WebKit workflows, inspect desktop
  and mobile screenshots, and verify canvas pixels and camera interaction.
  A successful compile or screenshot file alone is not a rendering test.
- For packaging changes, run packaging tests and a full verified ZIP build.
  Keep signing credentials out of tester builds. Do not publish or upload.
- For documentation-only edits, verify referenced code, commands, relative
  links, and agreement with status. Do not claim runtime tests were rerun.
- Update `docs/status.md` with evidence and changed decisions. Update the user
  guide for behavior changes and architecture/format docs for contract changes.
- Report tests actually run and any remaining gaps. Do not mark M0-M7 complete
  from preview checks or claim Linux/Windows support from portable dependencies.

The existing [license](LICENSE) is GPL-2.0-or-later. Preserve notices and locked
dependency provenance; do not silently relicense the rewrite.
