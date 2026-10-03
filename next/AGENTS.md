# Rewrite constraints

- Work only inside `next/`; do not edit legacy source, existing data, or existing research documents.
- Keep the implementation on the current user-created branch. No new branch/worktree unless requested.
- Write the scientific and native core in Rust, with TypeScript/React/Three.js for the shared interface. No C++ and no copied legacy implementation.
- Use the repository's ignored `../tmp/` for temporary work. Set `TMPDIR`, `TMP`, and `TEMP` accordingly for tools. Do not use system `/tmp`.
- Keep progress and changed decisions in `next/docs/`. Do not claim that the full M0-M7 plan is complete when only preview checks pass.
- Run Rust tests and Clippy, build the frontend/WASM, and exercise desktop/mobile browser rendering with screenshots and canvas-pixel checks before handing over a UI change.
- Generated WASM, build output, test artifacts, and node_modules are ignored. Keep dependency lockfiles.
