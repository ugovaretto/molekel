import { createHash } from "node:crypto";
import fs from "node:fs";
import path from "node:path";

export const ABOUT_VERSION = "0.9.1";

export function testerEnvironment(parent, root) {
  const env = { ...parent };
  // A tester build must never use developer credentials or contact the notary.
  for (const key of Object.keys(env)) {
    if (key.startsWith("APPLE_") || key.startsWith("TAURI_SIGNING_"))
      delete env[key];
  }
  const tmp = path.resolve(root, "../tmp");
  return {
    ...env,
    TMPDIR: tmp,
    TMP: tmp,
    TEMP: tmp,
    CARGO_TARGET_DIR: path.join(root, "target"),
    APPLE_SIGNING_IDENTITY: "-",
    MACOSX_DEPLOYMENT_TARGET: "13.0",
    COPYFILE_DISABLE: "1",
    CI: "true",
  };
}

export function inside(root, relative) {
  const resolved = path.resolve(root, relative);
  if (
    !relative ||
    path.isAbsolute(relative) ||
    !resolved.startsWith(`${path.resolve(root)}${path.sep}`)
  ) {
    throw new Error(`Unsafe package path: ${relative}`);
  }
  return resolved;
}

export function digest(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

export function snapshotFiles(root, names, destination) {
  const manifest = {};
  for (const name of [...new Set(names)].sort()) {
    const source = inside(root, name);
    if (fs.realpathSync(source) !== source || !fs.lstatSync(source).isFile()) {
      throw new Error(
        `Only regular, non-symlink source files may be packaged: ${name}`,
      );
    }
    if (/(^|\/)(\.env[^/]*|[^/]*\.(pem|p12|p8|key))$/i.test(name)) {
      throw new Error(
        `Possible credentials must not enter the source package: ${name}`,
      );
    }
    const bytes = fs.readFileSync(source);
    manifest[name] = digest(bytes);
    if (destination) {
      const target = inside(destination, name);
      fs.mkdirSync(path.dirname(target), { recursive: true });
      fs.copyFileSync(source, target);
    }
  }
  return manifest;
}

export function noticeFiles(directory) {
  return fs
    .readdirSync(directory)
    .filter(
      (name) =>
        /^(licen[cs]e|notice|copying|copyright|authors)([._-]|$)/i.test(name) &&
        fs.statSync(path.join(directory, name)).isFile(),
    )
    .sort();
}

export function npmNotices(app, output) {
  const lock = JSON.parse(
    fs.readFileSync(path.join(app, "package-lock.json"), "utf8"),
  );
  if (lock.lockfileVersion !== 3)
    throw new Error("Expected npm lockfile version 3");
  const packages = [];
  const sections = [
    "FRONTEND DEPENDENCIES\nVersions and license text from the locked, installed packages.\n",
  ];
  for (const [relative, entry] of Object.entries(lock.packages).sort()) {
    if (!relative || entry.dev) continue;
    const directory = inside(app, relative);
    const pkg = JSON.parse(
      fs.readFileSync(path.join(directory, "package.json"), "utf8"),
    );
    if (pkg.version !== entry.version)
      throw new Error(`Installed version differs from lock: ${relative}`);
    const files = noticeFiles(directory);
    if (!pkg.license || !files.some((f) => /^(licen[cs]e|copying)/i.test(f))) {
      throw new Error(`Missing frontend license text: ${pkg.name}`);
    }
    sections.push(`\n${pkg.name} ${pkg.version}\nLicense: ${pkg.license}\n`);
    for (const file of files)
      sections.push(
        `--- ${file} ---\n${fs.readFileSync(path.join(directory, file), "utf8")}\n`,
      );
    packages.push({
      name: pkg.name,
      version: pkg.version,
      license: pkg.license,
      directory,
    });
  }
  if (!packages.length) throw new Error("No frontend dependencies found");
  fs.writeFileSync(output, sections.join("\n"));
  return packages;
}

export function rustNotices(report, output, supplements = []) {
  if (!report.licenses?.length || !report.crates?.length)
    throw new Error("Empty Rust license report");
  const sections = [
    "RUST DEPENDENCIES\nIncludes native/WASM and build-time dependencies; some are not shipped in the executable.\n",
  ];
  for (const license of report.licenses) {
    if (!license.text?.trim() || !license.used_by?.length)
      throw new Error("Incomplete Rust license text");
    sections.push(
      `\n${license.name} (${license.id})\nUsed by:\n${license.used_by.map((u) => `  ${u.crate.name} ${u.crate.version}`).join("\n")}\n\n${license.text}\n`,
    );
  }
  for (const { package: pkg, license } of report.crates) {
    if (!pkg.source) continue;
    if (["Unknown", "Ignore"].includes(license))
      throw new Error(`Unresolved license: ${pkg.name}`);
    const directory = path.dirname(pkg.manifest_path);
    const files = noticeFiles(directory);
    if (
      !files.some((f) => /^(licen[cs]e|copying)/i.test(f)) &&
      !supplements.some((s) =>
        s.packages.includes(`${pkg.name}@${pkg.version}`),
      )
    ) {
      throw new Error(
        `Upstream license notice needs review: ${pkg.name} ${pkg.version}`,
      );
    }
    for (const file of files) {
      // Preserve notices that are not themselves detected as license boilerplate.
      sections.push(
        `\n--- ${pkg.name} ${pkg.version}: ${file} ---\n${fs.readFileSync(path.join(directory, file), "utf8")}\n`,
      );
    }
  }
  for (const supplement of supplements) {
    sections.push(
      `\n--- Supplement for ${supplement.packages.join(", ")} ---\nSource: ${supplement.url}\n${supplement.text}\n`,
    );
  }
  fs.writeFileSync(output, sections.join("\n"));
}

export function licenseSupplements(directory) {
  const entries = JSON.parse(
    fs.readFileSync(path.join(directory, "manifest.json"), "utf8"),
  );
  return entries.map((entry) => {
    const bytes = fs.readFileSync(inside(directory, entry.file));
    if (digest(bytes) !== entry.sha256)
      throw new Error(`License supplement checksum mismatch: ${entry.file}`);
    return { ...entry, text: bytes.toString("utf8") };
  });
}

export function treeManifest(root, relative = "") {
  const result = {};
  for (const entry of fs
    .readdirSync(path.join(root, relative), { withFileTypes: true })
    .sort((a, b) => a.name.localeCompare(b.name))) {
    const name = path.join(relative, entry.name);
    if (entry.isDirectory()) Object.assign(result, treeManifest(root, name));
    else if (entry.isFile()) {
      const file = path.join(root, name);
      result[name] = {
        sha256: digest(fs.readFileSync(file)),
        mode: fs.statSync(file).mode & 0o777,
      };
    } else throw new Error(`Unexpected non-regular archive entry: ${name}`);
  }
  return result;
}

export function assertEqualManifest(before, after) {
  if (JSON.stringify(before) !== JSON.stringify(after))
    throw new Error("Package/source file manifest changed unexpectedly");
}
