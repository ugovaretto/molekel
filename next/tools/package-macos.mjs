import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";
import {
  ABOUT_VERSION,
  testerEnvironment,
  digest,
  snapshotFiles,
  npmNotices,
  rustNotices,
  licenseSupplements,
  treeManifest,
  assertEqualManifest,
} from "./package-support.mjs";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const repo = path.dirname(root);
const app = path.join(root, "app");
const env = testerEnvironment(process.env, root);
const target = "aarch64-apple-darwin";
let staging;

function run(command, args, cwd = root, capture = false, overrides = {}) {
  const result = spawnSync(command, args, {
    cwd,
    env: { ...env, ...overrides },
    encoding: "utf8",
    stdio: capture ? ["ignore", "pipe", "pipe"] : "inherit",
    maxBuffer: 64 * 1024 * 1024,
  });
  if (result.error) throw result.error;
  if (result.status !== 0)
    throw new Error(
      `${path.basename(command)} failed (${result.status}): ${result.stderr ?? "see output above"}`,
    );
  return capture ? result.stdout.trimEnd() : "";
}

function copy(source, destination) {
  fs.mkdirSync(path.dirname(destination), { recursive: true });
  fs.cpSync(source, destination, {
    recursive: true,
    errorOnExist: true,
    force: false,
  });
}

function json(file, value) {
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

function sourceNames() {
  return [
    ...new Set([
      ...run(
        "git",
        [
          "ls-files",
          "--cached",
          "--others",
          "--exclude-standard",
          "-z",
          "--",
          "next",
          "AGENTS.md",
          "prompt-and-info.md",
          "doc/rewrite",
        ],
        repo,
        true,
      )
        .split("\0")
        .filter(Boolean),
      ".gitignore",
      "src/license",
      "data/molden.input",
      "data/h2o-dens.cube",
      "all_data/Benzene.MO19-BOTH-SIGNS.cube",
      "all_data/molden_test/test_homo.cube",
      ...["guanine", "3POR", "URIDINE-VANADATE", "alaninemulti"].map(
        (name) => `data/${name}.pdb`,
      ),
    ]),
  ].sort();
}

function main() {
  if (process.argv.includes("--help")) {
    console.log(
      "Usage: node next/tools/package-macos.mjs\nBuild and verify an ad-hoc-signed Apple Silicon tester ZIP.\nOutputs: next/artifacts/distributions/; no upload, notarization, or prompts.\nRequires macOS on Apple Silicon, Xcode command-line tools, Rust, Node, wasm-bindgen 0.2.108.\nInstalls locked npm dependencies and a pinned, repository-local cargo-about on first use.",
    );
    return;
  }
  if (process.argv.length !== 2)
    throw new Error("Unexpected arguments; use --help");
  if (process.platform !== "darwin" || process.arch !== "arm64")
    throw new Error(
      "This tester packager currently requires an Apple Silicon Mac and arm64 Node.js",
    );
  fs.mkdirSync(env.TMPDIR, { recursive: true });
  staging = fs.mkdtempSync(path.join(env.TMPDIR, "molekel-package-"));
  const config = JSON.parse(
    fs.readFileSync(path.join(app, "src-tauri/tauri.conf.json"), "utf8"),
  );
  if (!/^[0-9]+\.[0-9]+\.[0-9]+(?:-[a-zA-Z0-9.-]+)?$/.test(config.version))
    throw new Error("Unsafe package version");
  if (!/^[a-zA-Z0-9 ._-]+$/.test(config.productName))
    throw new Error("Unsafe product name");
  if (!fs.existsSync(path.join(root, "LICENSE")))
    throw new Error("Add the application's LICENSE before redistribution");
  const revision = run("git", ["rev-parse", "HEAD"], repo, true);
  const dirty = !!run(
    "git",
    [
      "status",
      "--porcelain",
      "--untracked-files=all",
      "--",
      "next",
      ".gitignore",
      "AGENTS.md",
      "prompt-and-info.md",
      "doc/rewrite",
    ],
    repo,
    true,
  );
  const started = new Date().toISOString();
  const id = `Molekel-Preview-${config.version}-apple-silicon-${started.replace(/[-:]/g, "").replace(/\.\d+Z$/, "Z")}-${revision.slice(0, 7)}${dirty ? "-dirty" : ""}`;
  const payload = path.join(staging, id);
  const sources = path.join(staging, "source-snapshot");
  fs.mkdirSync(payload);
  const sourceManifest = snapshotFiles(repo, sourceNames(), sources);
  json(path.join(sources, "SOURCE-MANIFEST.json"), sourceManifest);

  console.log("Checking toolchain and locked dependencies...");
  const rust = run("rustc", ["--version"], root, true);
  const wasmBindgen = run("wasm-bindgen", ["--version"], root, true);
  if (wasmBindgen !== "wasm-bindgen 0.2.108")
    throw new Error(
      "Install the pinned wasm-bindgen-cli 0.2.108 from next/docs/development.md",
    );
  run("xcrun", ["--find", "clang"], root, true);
  run("rustup", ["target", "add", "wasm32-unknown-unknown", target]);
  run("npm", ["ci", "--no-audit", "--no-fund"], app);
  run(process.execPath, ["--test", "tools/package-support.test.mjs"]);
  run("cargo", ["test", "--locked"]);

  const toolRoot = path.join(root, "artifacts/packaging-tools");
  const about = path.join(toolRoot, "bin/cargo-about");
  if (
    !fs.existsSync(about) ||
    run(about, ["--version"], root, true) !== `cargo-about ${ABOUT_VERSION}`
  ) {
    run(
      "cargo",
      [
        "install",
        "cargo-about",
        "--version",
        ABOUT_VERSION,
        "--features",
        "cli",
        "--locked",
        "--root",
        toolRoot,
      ],
      root,
      false,
      { CARGO_TARGET_DIR: path.join(env.TMPDIR, "cargo-about-target") },
    );
  }
  console.log("Collecting dependency notices and source packages...");
  const reportFile = path.join(staging, "rust-licenses.json");
  run(about, [
    "generate",
    "--locked",
    "--workspace",
    "--fail",
    "--format",
    "json",
    "--config",
    "packaging/about.toml",
    "--output-file",
    reportFile,
  ]);
  const report = JSON.parse(fs.readFileSync(reportFile, "utf8"));
  const notices = path.join(payload, "Third-party-notices");
  fs.mkdirSync(notices);
  for (const file of ["README.md", "IODATA-LICENSE.txt"])
    copy(
      path.join(root, "fixtures/molden/third-party", file),
      path.join(notices, "Molden-test-data", file),
    );
  rustNotices(
    report,
    path.join(notices, "Rust.txt"),
    licenseSupplements(path.join(root, "packaging/licenses")),
  );
  const npmPackages = npmNotices(app, path.join(notices, "Frontend.txt"));
  const dependencySources = path.join(staging, "dependency-sources");
  const inventory = [];
  const toml = createRequire(path.join(app, "package.json"))("@iarna/toml");
  const cargoLock = toml.parse(
    fs.readFileSync(path.join(root, "Cargo.lock"), "utf8"),
  );
  for (const { package: pkg } of report.crates) {
    if (!pkg.source) continue;
    if (!pkg.source.startsWith("registry+"))
      throw new Error(
        `Source packaging needs review for non-registry crate ${pkg.name}`,
      );
    const directory = path.dirname(pkg.manifest_path);
    const archive = path.resolve(
      directory,
      "../../../cache",
      path.basename(path.dirname(directory)),
      `${pkg.name}-${pkg.version}.crate`,
    );
    const checksum = cargoLock.package.find(
      (p) =>
        p.name === pkg.name &&
        p.version === pkg.version &&
        p.source === pkg.source,
    )?.checksum;
    if (!checksum || digest(fs.readFileSync(archive)) !== checksum)
      throw new Error(`Source archive checksum mismatch: ${pkg.name}`);
    copy(archive, path.join(dependencySources, "rust", path.basename(archive)));
    inventory.push({
      ecosystem: "rust",
      name: pkg.name,
      version: pkg.version,
      license: pkg.license,
      sha256: checksum,
    });
  }
  for (const pkg of npmPackages) {
    copy(pkg.directory, path.join(dependencySources, "frontend", pkg.name));
    inventory.push({
      ecosystem: "npm",
      name: pkg.name,
      version: pkg.version,
      license: pkg.license,
    });
  }
  json(path.join(notices, "Inventory.json"), inventory);
  const sourceOut = path.join(payload, "Source");
  fs.mkdirSync(sourceOut);
  run("/usr/bin/tar", [
    "-czf",
    path.join(sourceOut, "molekel-source.tar.gz"),
    "-C",
    sources,
    ".",
  ]);
  run("/usr/bin/tar", [
    "-czf",
    path.join(sourceOut, "dependency-sources.tar.gz"),
    "-C",
    dependencySources,
    ".",
  ]);

  console.log("Building the optimized native app with ad-hoc signing...");
  run("cargo", [
    "build",
    "--locked",
    "--release",
    "--target",
    target,
    "-p",
    "molekel-convert",
  ]);
  run(
    path.join(app, "node_modules/.bin/tauri"),
    [
      "build",
      "--ci",
      "--target",
      target,
      "--bundles",
      "app",
      "--config",
      path.join(root, "packaging/tauri.tester.conf.json"),
      "--",
      "--locked",
    ],
    app,
  );
  const built = path.join(
    root,
    "target",
    target,
    "release/bundle/macos",
    `${config.productName}.app`,
  );
  const bundle = path.join(payload, `${config.productName}.app`);
  run("/usr/bin/ditto", ["--norsrc", "--noextattr", "--noqtn", built, bundle]);
  const resources = path.join(bundle, "Contents/Resources");
  copy(notices, path.join(resources, "Third-party-notices"));
  copy(path.join(root, "LICENSE"), path.join(resources, "LICENSE"));
  copy(path.join(root, "LICENSE"), path.join(payload, "LICENSE"));
  copy(
    path.join(root, "packaging/TESTING.txt"),
    path.join(payload, "READ-ME-FIRST.txt"),
  );
  copy(path.join(root, "packaging/examples"), path.join(payload, "Examples"));
  copy(
    path.join(root, "fixtures/molden/water-rhf-ccpvdz.molden"),
    path.join(payload, "Examples/water.molden"),
  );
  copy(
    path.join(root, "fixtures/cube/signed-affine.cube"),
    path.join(payload, "Examples/signed-affine.cube"),
  );
  const converter = path.join(payload, "Tools/molekel-convert");
  copy(path.join(root, "target", target, "release/molekel-convert"), converter);
  run("/usr/bin/codesign", [
    "--force",
    "--sign",
    "-",
    "--timestamp=none",
    converter,
  ]);
  run("/usr/bin/codesign", ["--verify", "--strict", "--verbose=2", converter]);
  if (run("/usr/bin/lipo", ["-archs", converter], root, true) !== "arm64")
    throw new Error("Unexpected converter architecture");
  const converterLibraries = run(
    "/usr/bin/otool",
    ["-L", converter],
    root,
    true,
  )
    .split("\n")
    .slice(1)
    .map((line) => line.trim().split(" (")[0]);
  if (
    converterLibraries.some(
      (lib) =>
        !lib.startsWith("/System/Library/") && !lib.startsWith("/usr/lib/"),
    )
  )
    throw new Error("Non-system converter runtime library found");
  copy(
    path.join(root, "docs/user-guide.md"),
    path.join(payload, "User-guide.md"),
  );
  copy(
    path.join(root, "docs/structure-imports.md"),
    path.join(payload, "Structure-imports.md"),
  );
  copy(
    path.join(root, "docs/molden-import.md"),
    path.join(payload, "Molden-import.md"),
  );
  copy(
    path.join(root, "docs/cube-import.md"),
    path.join(payload, "Cube-import.md"),
  );
  const info = JSON.parse(
    run(
      "/usr/bin/plutil",
      ["-convert", "json", "-o", "-", path.join(bundle, "Contents/Info.plist")],
      root,
      true,
    ),
  );
  const executable = path.join(
    bundle,
    "Contents/MacOS",
    info.CFBundleExecutable,
  );
  const architecture = run("/usr/bin/lipo", ["-archs", executable], root, true);
  if (architecture !== "arm64")
    throw new Error(`Unexpected executable architecture: ${architecture}`);
  const libraries = run("/usr/bin/otool", ["-L", executable], root, true)
    .split("\n")
    .slice(1)
    .map((line) => line.trim().split(" (")[0]);
  if (
    libraries.some(
      (lib) =>
        !lib.startsWith("/System/Library/") && !lib.startsWith("/usr/lib/"),
    )
  )
    throw new Error(
      "Non-system runtime library found; review bundle portability before distributing",
    );
  run("/usr/bin/codesign", [
    "--force",
    "--sign",
    "-",
    "--timestamp=none",
    "--identifier",
    config.identifier,
    bundle,
  ]);
  run("/usr/bin/codesign", [
    "--verify",
    "--deep",
    "--strict",
    "--verbose=2",
    bundle,
  ]);
  const buildInfo = {
    name: config.productName,
    version: config.version,
    package: id,
    gitRevision: revision,
    sourceDirty: dirty,
    sourceManifestSha256: digest(JSON.stringify(sourceManifest)),
    builtAt: started,
    target,
    architecture,
    minimumMacOS: info.LSMinimumSystemVersion,
    buildMacOS: run("/usr/bin/sw_vers", ["-productVersion"], root, true),
    signing: "ad-hoc",
    notarized: false,
    toolchain: {
      rust,
      node: process.version,
      wasmBindgen,
      cargoAbout: ABOUT_VERSION,
    },
    checks: [
      "packaging unit tests",
      "Rust tests",
      "TypeScript and production WASM/frontend build",
      "arm64 architecture",
      "system-only dynamic libraries",
      "strict app signature",
      "ZIP extraction byte and mode comparison",
      "extracted app signature",
      "sample PDB/XYZ/Molden/cube imports",
      "standalone converter signature, architecture, and runtime libraries",
      "extracted converter Molden/cube/native roundtrip and WASM agreement",
    ],
    limitations:
      "No clean-machine/Gatekeeper installation or full supported-OS-range qualification. Browser checks are a separate test command.",
  };
  json(path.join(payload, "BUILD-INFO.json"), buildInfo);
  // Exercise exactly the generated WASM that is embedded in the release app.
  const core = path.join(app, "src/wasm");
  run(process.execPath, [
    "tools/verify-package-examples.mjs",
    core,
    path.join(payload, "Examples"),
    converter,
    path.join(staging, "converted-example.molekel"),
  ]);
  assertEqualManifest(sourceManifest, snapshotFiles(repo, sourceNames()));

  console.log("Creating and verifying the ZIP...");
  const zip = path.join(staging, `${id}.zip`);
  run("/usr/bin/ditto", [
    "-c",
    "-k",
    "--norsrc",
    "--noextattr",
    "--noqtn",
    "--keepParent",
    payload,
    zip,
  ]);
  const extracted = path.join(staging, "verification");
  run("/usr/bin/ditto", ["-x", "-k", "--noqtn", zip, extracted]);
  const restored = path.join(extracted, id);
  assertEqualManifest(treeManifest(payload), treeManifest(restored));
  run("/usr/bin/codesign", [
    "--verify",
    "--deep",
    "--strict",
    "--verbose=2",
    path.join(restored, `${config.productName}.app`),
  ]);
  run("/usr/bin/codesign", [
    "--verify",
    "--strict",
    "--verbose=2",
    path.join(restored, "Tools/molekel-convert"),
  ]);
  run(process.execPath, [
    "tools/verify-package-examples.mjs",
    core,
    path.join(restored, "Examples"),
    path.join(restored, "Tools/molekel-convert"),
    path.join(staging, "extracted-converted-example.molekel"),
  ]);
  const out = path.join(root, "artifacts/distributions");
  fs.mkdirSync(out, { recursive: true });
  const filename = path.basename(zip);
  fs.copyFileSync(zip, path.join(out, filename), fs.constants.COPYFILE_EXCL);
  fs.writeFileSync(
    path.join(out, `${filename}.sha256`),
    `${digest(fs.readFileSync(zip))}  ${filename}\n`,
    { flag: "wx" },
  );
  json(path.join(out, `${id}.json`), buildInfo);
  console.log(
    `\nTester package: ${path.join(out, filename)}\nChecksum: ${path.join(out, `${filename}.sha256`)}\nStaging retained: ${staging}\nNot notarized; share only with informed testers. Nothing was uploaded.`,
  );
}

try {
  main();
} catch (error) {
  console.error(`Packaging failed: ${error.message}`);
  if (staging) console.error(`Diagnostic files retained at ${staging}`);
  process.exitCode = 1;
}
