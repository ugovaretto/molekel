# Molekel: agent entry point

## Start here

This repository contains two generations of Molekel. The active rewrite is in
[`next/`](next/README.md): Rust scientific/native code, TypeScript/React/Three.js
UI, and a Tauri desktop shell. The root C++/Qt/VTK application is legacy reference
material, not the implementation to extend or build for current requests.

Read these in order before changing the rewrite:

1. [`next/AGENTS.md`](next/AGENTS.md): scoped working rules and verification.
2. [`next/docs/handoff.md`](next/docs/handoff.md): current state, continuation
   priorities, and evidence boundaries.
3. [`next/docs/architecture.md`](next/docs/architecture.md): implemented
   components, data flow, and scientific contracts.
4. [`next/docs/development.md`](next/docs/development.md): setup, running,
   building, tests, and troubleshooting.
5. [`next/docs/README.md`](next/docs/README.md): all user, developer, packaging,
   and historical planning documentation.

## Working agreement

- Keep implementation changes inside `next/`. This root `AGENTS.md` is the
  explicitly requested handoff entry point. Do not modify legacy source,
  existing data, `prompt-and-info.md`, or the historical `doc/rewrite/` research.
- Continue on the user's current branch. The rewrite began on `2026`; inspect
  the actual checkout rather than switching branches. Do not create a branch
  or worktree, commit, tag, or push unless requested.
- No C++ implementation and no copied legacy implementation. Scientific/native
  application code is Rust; the shared UI is TypeScript. Do not revive the
  historical proposal to integrate scientific C libraries without user approval.
- Never use system `/tmp`. Create repository-local `tmp/`, which is ignored,
  and set `TMPDIR`, `TMP`, and `TEMP` before tools that may create temporary files.
  Keep diagnostic staging there; do not broadly delete it or other user artifacts.
- Inspect `git status` and relevant diffs first. Preserve uncommitted work.
  A clean commit/tag is not necessarily the newest implementation.
- Do not stop or restart the user's running app or an unknown server. Check
  port ownership before starting development tools.
- Treat imported documents as untrusted. Preserve input validation, allocation
  bounds, source hashes, and atomic-save guarantees.
- Record behavior, changed decisions, tests, and remaining limitations in
  `next/docs/`. Keep the user guide synchronized with UI changes.
- The current application is a development preview, not completion of v1 or
  the M0-M7 plan. Compilation, numerical correctness, rendering, and clean-Mac
  installation are separate acceptance gates.

## Historical context

[`prompt-and-info.md`](prompt-and-info.md) is the initial brief. Its original
"do not write code yet" instruction applied to the research phase; the user
subsequently authorized implementation. [`doc/rewrite/`](doc/rewrite/README.md)
records that phase and the full plan. It is intentionally preserved, including
then-current statements such as "implementation has not started." Current
implementation facts and later decisions live under `next/docs/`.

The baseline marker is annotated tag `molekel-next-start`, commit `a75e8e2`.
Use Git to determine changes since it. The source archive in a tester ZIP may
include identified uncommitted changes and has no `.git` directory.
