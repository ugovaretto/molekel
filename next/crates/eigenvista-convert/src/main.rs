use eigenvista_import::{ImportReport, MAX_IMPORT_BYTES, import_bytes};
use serde::Serialize;
use std::{
    collections::HashSet,
    ffi::OsString,
    fs::{self, File},
    io::{self, Read, Write},
    path::{Path, PathBuf},
    process::ExitCode,
};

const HELP: &str = "EigenVista file converter

Usage: eigenvista-convert [OPTIONS] INPUT...

Read supported Molden, XYZ, PDB, cube, or native documents and write .eigenvista.
Explicit .molekel destinations remain accepted for filename compatibility.
Molden is detected by its [Molden Format] header, including .molden.input files.

  -o, --output FILE     Destination for one input (default: INPUT.eigenvista)
      --output-dir DIR Destination directory, required for multiple inputs
      --force          Replace existing outputs, never input files
      --check          Validate and report without writing any documents
      --json           Print a machine-readable result array to stdout
  -h, --help           Show this help
  -V, --version        Show the converter version
      --               Treat all remaining arguments as input filenames

Output directories must already exist. No external tools or calculation engines
are invoked. Output publication is atomic; existing outputs are protected unless
--force is supplied. Batch failures retain completed outputs and exit nonzero.
Exit codes: 0 success, 1 conversion/write failure, 2 invalid arguments/paths.
";

#[derive(Default)]
struct Options {
    inputs: Vec<PathBuf>,
    output: Option<PathBuf>,
    directory: Option<PathBuf>,
    force: bool,
    check: bool,
    json: bool,
}

enum Command {
    Help,
    Version,
    Convert(Options),
}

fn options(args: impl IntoIterator<Item = OsString>) -> Result<Command, String> {
    let mut args = args.into_iter();
    let mut options = Options::default();
    let mut positional = false;
    while let Some(arg) = args.next() {
        if positional {
            options.inputs.push(arg.into());
            continue;
        }
        match arg.to_str() {
            Some("--") => positional = true,
            Some("-h" | "--help") => return Ok(Command::Help),
            Some("-V" | "--version") => return Ok(Command::Version),
            Some("--force") => options.force = true,
            Some("--check") => options.check = true,
            Some("--json") => options.json = true,
            Some("-o" | "--output") => {
                if options.output.is_some() {
                    return Err("--output may only be specified once".into());
                }
                options.output = Some(args.next().ok_or("--output needs a filename")?.into());
            }
            Some("--output-dir") => {
                if options.directory.is_some() {
                    return Err("--output-dir may only be specified once".into());
                }
                options.directory =
                    Some(args.next().ok_or("--output-dir needs a directory")?.into());
            }
            Some(s) if s.starts_with('-') => return Err(format!("Unknown option {s}; use --help")),
            _ => options.inputs.push(arg.into()),
        }
    }
    if options.inputs.is_empty() {
        return Err("Choose at least one input file; use --help".into());
    }
    if options.inputs.len() > 256 {
        return Err("At most 256 input files may be converted in one batch".into());
    }
    if options.output.is_some() && options.directory.is_some() {
        return Err("Choose --output or --output-dir, not both".into());
    }
    if options.check && (options.output.is_some() || options.directory.is_some() || options.force) {
        return Err("--check does not accept destination or --force options".into());
    }
    if !options.check && options.inputs.len() > 1 && options.directory.is_none() {
        return Err("Multiple input files require --output-dir".into());
    }
    Ok(Command::Convert(options))
}

fn output_name(input: &Path) -> Result<PathBuf, String> {
    let name = input.file_name().ok_or("Input must name a file")?;
    let mut path = PathBuf::from(name);
    if name
        .to_string_lossy()
        .to_ascii_lowercase()
        .ends_with(".molden.input")
    {
        path.set_extension("");
    }
    path.set_extension("eigenvista");
    Ok(path)
}

fn canonical_destination(path: &Path) -> Result<PathBuf, String> {
    let name = path.file_name().ok_or("Output must name a file")?;
    let parent = path
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."));
    let parent = fs::canonicalize(parent)
        .map_err(|e| format!("Output directory {}: {e}", parent.display()))?;
    if !parent.is_dir() {
        return Err("Output parent is not a directory".into());
    }
    let target = parent.join(name);
    if let Ok(metadata) = fs::symlink_metadata(&target) {
        if !metadata.is_file() || metadata.file_type().is_symlink() {
            return Err(format!(
                "Output must be a regular file, not a directory or symlink: {}",
                target.display()
            ));
        }
        return fs::canonicalize(target).map_err(|e| e.to_string());
    }
    Ok(target)
}

struct Destination {
    path: PathBuf,
    replace: bool,
}

fn destinations(options: &Options) -> Result<Vec<Option<Destination>>, String> {
    let inputs: HashSet<PathBuf> = options
        .inputs
        .iter()
        .map(|input| fs::canonicalize(input).map_err(|e| format!("{}: {e}", input.display())))
        .collect::<Result<_, _>>()?;
    if options.check {
        return Ok((0..options.inputs.len()).map(|_| None).collect());
    }
    let mut destinations = HashSet::new();
    #[cfg(unix)]
    let mut file_identities = HashSet::new();
    options
        .inputs
        .iter()
        .map(|input| {
            let destination = match (&options.output, &options.directory) {
                (Some(output), _) => output.clone(),
                (_, Some(directory)) => directory.join(output_name(input)?),
                _ => input.with_file_name(output_name(input)?),
            };
            if !destination.extension().is_some_and(|ext| {
                ext.eq_ignore_ascii_case("eigenvista") || ext.eq_ignore_ascii_case("molekel")
            }) {
                return Err("Output filenames must end in .eigenvista or .molekel".into());
            }
            let destination = canonical_destination(&destination)?;
            if inputs.contains(&destination) {
                return Err(format!(
                    "Refusing to replace an input file: {}",
                    destination.display()
                ));
            }
            // Case-insensitive aliases must also collide on the macOS target.
            let key = if cfg!(target_os = "macos") || cfg!(target_os = "windows") {
                destination.to_string_lossy().to_lowercase()
            } else {
                destination.to_string_lossy().into_owned()
            };
            if !destinations.insert(key) {
                return Err(format!(
                    "Inputs map to the same output: {}",
                    destination.display()
                ));
            }
            let exists = destination.exists();
            #[cfg(unix)]
            if exists {
                use std::os::unix::fs::MetadataExt;
                let metadata = fs::metadata(&destination).map_err(|e| e.to_string())?;
                if !file_identities.insert((metadata.dev(), metadata.ino())) {
                    return Err(format!(
                        "Inputs map to the same existing output: {}",
                        destination.display()
                    ));
                }
            }
            if exists && !options.force {
                return Err(format!(
                    "Output already exists: {}; use --force to replace it",
                    destination.display()
                ));
            }
            // Even --force must not clobber another batch result or a newly created file.
            Ok(Some(Destination {
                path: destination,
                replace: exists && options.force,
            }))
        })
        .collect()
}

fn read_bounded(input: &Path) -> Result<Vec<u8>, String> {
    let metadata = fs::metadata(input).map_err(|e| e.to_string())?;
    if !metadata.is_file() || metadata.len() > MAX_IMPORT_BYTES as u64 {
        return Err("Input must be a regular file no larger than 128 MiB".into());
    }
    let file = File::open(input).map_err(|e| e.to_string())?;
    let metadata = file.metadata().map_err(|e| e.to_string())?;
    if !metadata.is_file() || metadata.len() > MAX_IMPORT_BYTES as u64 {
        return Err("Input must be a regular file no larger than 128 MiB".into());
    }
    let mut bytes = Vec::new();
    file.take(MAX_IMPORT_BYTES as u64 + 1)
        .read_to_end(&mut bytes)
        .map_err(|e| e.to_string())?;
    if bytes.len() > MAX_IMPORT_BYTES {
        return Err("Input exceeds the 128 MiB byte budget".into());
    }
    Ok(bytes)
}

#[derive(Serialize)]
struct Outcome {
    input: String,
    output: Option<String>,
    status: &'static str,
    report: Option<ImportReport>,
    atoms: usize,
    orbitals: usize,
    densities: usize,
    error: Option<String>,
}

fn convert(options: Options) -> Result<u8, String> {
    let destinations = destinations(&options)?;
    let mut outcomes = Vec::new();
    for (input, destination) in options.inputs.iter().zip(destinations) {
        let mut outcome = Outcome {
            input: input.to_string_lossy().into_owned(),
            output: destination
                .as_ref()
                .map(|d| d.path.to_string_lossy().into_owned()),
            status: "failed",
            report: None,
            atoms: 0,
            orbitals: 0,
            densities: 0,
            error: None,
        };
        let result = (|| {
            let bytes = read_bounded(input)?;
            let name = input.file_name().unwrap_or_default().to_string_lossy();
            let imported = import_bytes(&bytes, &name)?;
            outcome.atoms = imported.document.atoms.len();
            outcome.orbitals = imported.document.orbitals.len();
            outcome.densities = imported.document.densities.len();
            outcome.report = Some(imported.report);
            if let Some(destination) = destination {
                let encoded = eigenvista_format::encode(&imported.document)?;
                if destination.replace {
                    eigenvista_format::native::save_atomic(&destination.path, &encoded)?;
                } else {
                    eigenvista_format::native::save_atomic_new(&destination.path, &encoded)?;
                }
            }
            Ok::<_, String>(())
        })();
        match result {
            Ok(()) => {
                outcome.status = if options.check {
                    "validated"
                } else {
                    "converted"
                }
            }
            Err(error) => outcome.error = Some(error),
        }
        if !options.json {
            if let Some(error) = &outcome.error {
                eprintln!("{}: {error}", outcome.input);
            } else {
                eprintln!(
                    "{}: {} ({} atoms, {} orbitals, {} density matrices){}",
                    outcome.input,
                    outcome.status,
                    outcome.atoms,
                    outcome.orbitals,
                    outcome.densities,
                    outcome
                        .output
                        .as_ref()
                        .map(|p| format!(" -> {p}"))
                        .unwrap_or_default()
                );
                for warning in &outcome.report.as_ref().unwrap().warnings {
                    eprintln!("  Warning: {warning}");
                }
            }
        }
        outcomes.push(outcome);
    }
    if options.json {
        let mut stdout = io::stdout().lock();
        serde_json::to_writer_pretty(&mut stdout, &outcomes).map_err(|e| e.to_string())?;
        writeln!(stdout).map_err(|e| e.to_string())?;
    }
    Ok(u8::from(outcomes.iter().any(|o| o.status == "failed")))
}

fn main() -> ExitCode {
    let result = match options(std::env::args_os().skip(1)) {
        Ok(Command::Help) => {
            print!("{HELP}");
            Ok(0)
        }
        Ok(Command::Version) => {
            println!("eigenvista-convert {}", env!("CARGO_PKG_VERSION"));
            Ok(0)
        }
        Ok(Command::Convert(options)) => convert(options),
        Err(error) => Err(error),
    };
    match result {
        Ok(code) => ExitCode::from(code),
        Err(error) => {
            eprintln!("eigenvista-convert: {error}");
            ExitCode::from(2)
        }
    }
}
