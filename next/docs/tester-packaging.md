# macOS tester ZIP

From the repository root, run:

```sh
npm --prefix next/app run package:macos
```

Or, from `next/app`, run `npm run package:macos`. The script resolves paths
relative to itself, not the current directory. Run
`node next/tools/package-macos.mjs --help` for requirements without building.

## Prerequisites

- Apple Silicon Mac, with an arm64 Node.js installation (tested with Node 26).
- Xcode command-line tools, Rust/rustup, and wasm-bindgen CLI 0.2.108 as described
  in the [development guide](development.md).
- Internet access for locked npm/Cargo dependencies and initial tooling setup.
- No Apple account, paid membership, signing certificate, or notary credentials.

The command installs npm dependencies from the lockfile and installs pinned
`cargo-about` 0.9.1 with its CLI feature under ignored
`next/artifacts/packaging-tools/` when needed. It does not install a global npm
or Cargo tool. Subsequent runs reuse compiled Rust dependencies and the local
license tool. Build/staging temporary files use the repository's ignored `tmp/`.

## Outputs

Successful runs print a ZIP path under `next/artifacts/distributions/`, a
SHA-256 companion file, and a build-information JSON file. Filenames include
the app version, architecture, UTC timestamp, Git revision, and `-dirty` when
the implementation has uncommitted changes. Existing packages are never
overwritten. Send the entire ZIP to testers; send its checksum separately
when useful. A checksum detects transfer errors but does not authenticate a
publisher by itself.

The ZIP contains:

- An optimized, complete `Molekel Preview.app` with an ad-hoc signature.
- An optimized, ad-hoc-signed `Tools/molekel-convert` for headless conversion;
  it uses the same Rust import library and needs no GUI, Python, or vendor tool.
- `READ-ME-FIRST.txt`, the standalone `User-guide.md` (including known
  limitations), structure import policies, `Cube-import.md`, and `Molden-import.md`
  with the exact scientific profiles and CLI usage.
- Synthetic water PDB/XYZ inputs and independently calculated `water.molden`;
  all yield three atoms/two bonds, while Molden also supplies orbitals/density.
- `Examples/signed-affine.cube`, a first-party mathematical signed field with
  two atoms/one bond on a skewed 7 x 7 x 7 grid, for volume/raycast and mesh checks.
- The application license and collected third-party notices. Notices are also
  embedded in the app's Resources, so moving the app does not discard them.
- The exact current first-party source snapshot, lockfiles, and legacy test
  fixtures needed by the regression suite. This includes root/scoped
  `AGENTS.md`, current documentation, the original brief, and historical
  `doc/rewrite/` plans so another agent can continue from the snapshot.
  Uncommitted source is included and identified, not silently replaced with
  the last Git commit. Git history is not included.
- Checksummed original Rust dependency archives and frontend production
  dependency sources, including source for MPL components. Build-only Rust
  dependencies are conservatively included. Normal rebuilding still uses the
  locked registry dependencies; these copies also support source inspection.
- `BUILD-INFO.json`, recording toolchain, deployment target, build host macOS,
  source identity, and verification checks without signing credentials.

## Installing and reproducing a build

Testers should follow the [user guide](user-guide.md) or the copy included at
the top of the ZIP. No developer tools are needed to run the packaged app.
Forward the complete package with its license, notices, and sources.

To work from the supplied source, extract `Source/molekel-source.tar.gz` into
a directory and follow `AGENTS.md` and `next/docs/development.md` there. The
snapshot includes frozen scientific data, four legacy PDB test inputs,
the read-only `data/molden.input` regression, and the cube regressions
`data/h2o-dens.cube`, `all_data/Benzene.MO19-BOTH-SIGNS.cube`, and
`all_data/molden_test/test_homo.cube`;
normal builds/tests do not need the rest of the legacy application. The
snapshot is not a vendored offline build environment. It has no `.git`, and
the ZIP packager itself requires a Git checkout for revision/status recording.
`BUILD-INFO.json` plus `SOURCE-MANIFEST.json` identify the delivered source;
locked dependencies and toolchain records do not guarantee bit-identical binaries.

Do not edit source during packaging: the final source-stability check must
match the initial snapshot. Failed runs keep diagnostic staging beneath
repository `tmp/`; fix the cause and rerun. A later successful build gets a
new timestamped filename without overwriting earlier packages.

## Checks and boundaries

The command runs packaging unit tests and the Rust regression suite, then the
TypeScript/WASM/production web build and Tauri release build. It verifies
arm64 architecture, system-only dynamic library links, strict app/CLI signing,
PDB/XYZ/Molden/cube imports, and source stability during the build. It extracts the
ZIP into a fresh repository-local directory and checks every file's contents
and permissions plus both extracted signatures. The actual extracted CLI converts
the water Molden and signed cube examples, revalidates their native outputs, and
is compared against the same WASM importer embedded in the application. The cube
check includes bonds, scalar samples, and extraction of both signed meshes.
Any failure stops delivery
of a new package; staging is retained for diagnosis.

Browser interaction tests remain `npm run test:e2e`; they require Playwright's
browsers and are not installed or run implicitly by the packager. A signature
check is not a clean-machine installation test. The deployment target is
macOS 13, but that entire OS range has not been qualified. The current build
host is recorded instead of claiming all compatible systems have been tested.

This command deliberately clears Apple and Tauri signing credentials from its
child environment and forces ad-hoc signing. It never notarizes, uploads,
publishes a GitHub release, commits, tags, restarts the running app, or opens
security settings. Testers may need Apple's per-app Open Anyway exception.
It is not a replacement for a future Developer ID-signed/notarized release.

Before sharing a new UI/runtime change, also perform the separate browser and
manual native checks in the [development guide](development.md). The script
does not launch or interact with the user's app. Public signing/notarization
needs a separate approved workflow; installing Apple credentials alone will
not make this deliberately ad-hoc tester command use them.

The new source retains the repository's GPL-2.0-or-later license. Dependency
notices are generated by `cargo-about` and from installed npm package license
files. Missing upstream notices are supplemented from pinned source revisions
in `packaging/licenses/manifest.json`. Those files are checksummed, and missing
notices after a dependency update require review. This automated inventory is
not a legal compatibility opinion or a claim that release qualification is done.
The separately licensed IOData ORCA test fixture retains its GPL-3.0-or-later
notices in both source and `Third-party-notices/Molden-test-data`; no IOData code
is linked into the application or converter. See the [dependency record](dependencies.md).

References: [Tauri ad-hoc signing](https://v2.tauri.app/distribute/sign/macos/#ad-hoc-signing),
[Apple's app-specific security exception](https://support.apple.com/en-us/102445),
[cargo-about](https://embarkstudios.github.io/cargo-about/).
