# Documentation map

This is the documentation hub for the **Rust Molekel development preview**.
It separates the implemented application from the larger planned v1. Start
with the [user guide](user-guide.md) to use the program, or the
[root AGENTS.md](../../AGENTS.md) to continue development.

## Current documentation

| Need | Read |
| --- | --- |
| What Molekel does, installation, controls, files, saving, troubleshooting | [User guide](user-guide.md) |
| Development setup, run/build commands, tests, outputs | [Development guide](development.md) |
| Components, code ownership, scientific contracts, runtime data flow | [As-built architecture](architecture.md) |
| Agent continuation, current baseline, priorities, verification gaps | [Handoff](handoff.md) and [scoped AGENTS.md](../AGENTS.md) |
| Implemented features, test evidence, M0-M7 progress | [Implementation status](status.md) |
| Repeatable Apple Silicon tester ZIP, contents, signing boundaries | [Tester packaging](tester-packaging.md) |
| Implemented `.molekel` container, validation, persistence | [Native preview format](preview-format.md) |
| XYZ/PDB import policy, automatic bonds, algorithm rationale | [Structure imports](structure-imports.md) |
| Libraries, provenance, licensing | [Dependency record](dependencies.md) and [license](../LICENSE) |
| Independent scientific references and regeneration | [PySCF fixtures](../fixtures/pyscf/README.md) |
| Cross-platform checks and absence of active hosted CI | [Portability checks](../ci/README.md) |
| Fast project overview | [Rewrite README](../README.md) |

## Planning archive

The [original brief](../../prompt-and-info.md) and [research overview](../../doc/rewrite/README.md)
explain the goal and early investigation. These are **historical planning
documents**, not a description of the current executable. Their "not started"
statements refer to that phase. Preserve them as evidence; record subsequent
decisions and progress in the current documents above.

| Planning question | Historical document |
| --- | --- |
| What did the original application do? | [Legacy review](../../doc/rewrite/01-legacy-review.md) |
| What is required for full v1? | [Requirements](../../doc/rewrite/02-requirements.md) |
| Why the shared browser/native approach? | [Architecture decision](../../doc/rewrite/03-architecture-decision.md) |
| Which scientific/rendering methods were considered? | [Scientific methods](../../doc/rewrite/04-scientific-methods.md) |
| What is the proposed complete native format? | [Full format proposal](../../doc/rewrite/05-molekel-format.md) |
| How should external producers be converted? | [Conversion strategy](../../doc/rewrite/06-conversion-strategy.md) |
| What evidence is required before release? | [Validation and delivery](../../doc/rewrite/07-validation-and-delivery.md) |
| Where did the research come from? | [Sources](../../doc/rewrite/08-sources.md) |
| What is the implementation sequence? | [M0-M7 plan](../../doc/rewrite/09-implementation-plan.md) |

Later decisions take precedence for the current implementation: a clean Rust
scientific core instead of the proposed scientific C integrations; classic
`mcubes` as a temporary mesher; bohr coordinates in the scene; WASM worker jobs
in both browser and desktop; a restricted preview format; and ad-hoc-signed
tester ZIPs rather than a public release. The [architecture](architecture.md)
records these differences without treating the remaining requirements as waived.

## Keeping documentation current

The code and tests establish behavior; the status document records dated
evidence. When they disagree, investigate and correct the current docs. Update
the user guide for visible behavior, architecture/format docs for contracts,
development/packaging docs for commands, and status/handoff for changed gates.
Never turn an unverified design intention into a supported-feature claim.

Tester ZIPs include a standalone `User-guide.md`. Their first-party source
archive contains this documentation tree, the agent entry points, original
brief, and planning archive. It excludes development caches and Git history.
