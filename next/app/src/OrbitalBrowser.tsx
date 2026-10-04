import { useEffect, useMemo, useRef, useState } from "react";
import { ListX, Play, Search, X } from "lucide-react";
import type { MolekelDocument } from "./types";

export const maxOrbitalBatch = 32;

export function OrbitalBrowser({
  document,
  resolution,
  onClose,
  onGenerate,
}: {
  document: MolekelDocument;
  resolution: number;
  onClose: () => void;
  onGenerate: (ids: string[], iso: number, resolution: number) => void;
}) {
  const dialog = useRef<HTMLDialogElement>(null);
  const selectListed = useRef<HTMLInputElement>(null);
  const [filter, setFilter] = useState("");
  const [selected, setSelected] = useState<Set<string>>(
    () =>
      new Set(
        document.orbitals
          .filter((orbital) => orbital.id === document.view.field)
          .map((orbital) => orbital.id),
      ),
  );
  const [isovalue, setIsovalue] = useState(String(document.view.isovalue));
  const [gridResolution, setGridResolution] = useState(resolution);
  const query = filter.trim().toLowerCase();
  const listed = document.orbitals
    .map((orbital, index) => ({ orbital, number: index + 1 }))
    .filter(({ orbital, number }) =>
      [String(number), orbital.id, orbital.label, orbital.spin].some((value) =>
        value.toLowerCase().includes(query),
      ),
    );
  const listedSelected = listed.filter(({ orbital }) =>
    selected.has(orbital.id),
  ).length;
  const savedCounts = useMemo(() => {
    const counts = new Map<string, number>();
    for (const surface of document.surfaces) {
      if (surface.field)
        counts.set(surface.field, (counts.get(surface.field) ?? 0) + 1);
    }
    return counts;
  }, [document.surfaces]);
  const iso = Number(isovalue);
  const validIso = isovalue.trim() !== "" && Number.isFinite(iso) && iso > 0;

  useEffect(() => {
    const element = dialog.current;
    element?.showModal();
    return () => element?.close();
  }, []);

  useEffect(() => {
    if (selectListed.current)
      selectListed.current.indeterminate =
        listedSelected > 0 && listedSelected < listed.length;
  }, [listedSelected, listed.length]);

  function toggle(id: string, checked: boolean) {
    setSelected((previous) => {
      const next = new Set(previous);
      if (checked) next.add(id);
      else next.delete(id);
      return next;
    });
  }

  return (
    <dialog
      ref={dialog}
      className="orbital-browser"
      aria-labelledby="orbital-browser-title"
      onCancel={(event) => {
        event.preventDefault();
        onClose();
      }}
    >
      <div className="orbital-browser-header">
        <div>
          <h2 id="orbital-browser-title">Orbitals</h2>
          <p>{document.title}</p>
        </div>
        <button
          title="Close orbitals"
          aria-label="Close orbitals"
          onClick={onClose}
        >
          <X size={16} />
        </button>
      </div>
      <div className="orbital-browser-toolbar">
        <div className="orbital-filter">
          <Search size={15} aria-hidden="true" />
          <input
            type="search"
            aria-label="Filter orbitals"
            placeholder="Search orbitals"
            value={filter}
            onChange={(event) => setFilter(event.target.value)}
            autoFocus
          />
        </div>
        <span className="orbital-listed-count">
          {listed.length} / {document.orbitals.length}
        </span>
        <button
          title="Clear orbital selection"
          aria-label="Clear orbital selection"
          disabled={selected.size === 0}
          onClick={() => setSelected(new Set())}
        >
          <ListX size={16} />
        </button>
      </div>
      <div className="orbital-table-scroll">
        <table className="orbital-table" aria-label="Molecular orbitals">
          <colgroup>
            <col className="orbital-check-column" />
            <col className="orbital-number-column" />
            <col className="orbital-name-column" />
            <col className="orbital-spin-column" />
            <col className="orbital-occupation-column" />
            <col className="orbital-energy-column" />
            <col className="orbital-saved-column" />
          </colgroup>
          <thead>
            <tr>
              <th scope="col">
                <input
                  ref={selectListed}
                  type="checkbox"
                  aria-label="Select listed orbitals"
                  checked={
                    listed.length > 0 && listedSelected === listed.length
                  }
                  disabled={listed.length === 0}
                  onChange={(event) => {
                    const checked = event.target.checked;
                    setSelected((previous) => {
                      const next = new Set(previous);
                      for (const { orbital } of listed) {
                        if (checked) next.add(orbital.id);
                        else next.delete(orbital.id);
                      }
                      return next;
                    });
                  }}
                />
              </th>
              <th scope="col">No.</th>
              <th scope="col">Orbital</th>
              <th scope="col">Spin</th>
              <th scope="col" className="orbital-numeric">
                Occupation
              </th>
              <th scope="col" className="orbital-numeric">
                Energy (hartree)
              </th>
              <th scope="col" className="orbital-numeric">
                Saved
              </th>
            </tr>
          </thead>
          <tbody>
            {listed.map(({ orbital, number }) => (
              <tr
                key={orbital.id}
                className={selected.has(orbital.id) ? "selected" : ""}
              >
                <td>
                  <input
                    type="checkbox"
                    aria-label={`Select orbital ${number}`}
                    checked={selected.has(orbital.id)}
                    onChange={(event) =>
                      toggle(orbital.id, event.target.checked)
                    }
                  />
                </td>
                <td className="orbital-index">{number}</td>
                <td>
                  <span
                    className="orbital-name"
                    title={orbital.label || orbital.id}
                  >
                    {orbital.label || orbital.id}
                  </span>
                </td>
                <td className="orbital-spin">
                  {orbital.spin && orbital.spin.toLowerCase() !== "unknown"
                    ? orbital.spin
                    : "Unknown"}
                </td>
                <td className="orbital-numeric">
                  {orbital.occupation === null
                    ? "--"
                    : orbital.occupation.toFixed(3)}
                </td>
                <td className="orbital-numeric">
                  {orbital.energy === null ? "--" : orbital.energy.toFixed(6)}
                </td>
                <td className="orbital-numeric">
                  {savedCounts.get(orbital.id) ?? 0}
                </td>
              </tr>
            ))}
            {listed.length === 0 && (
              <tr>
                <td colSpan={7} className="orbital-empty">
                  No matching orbitals
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
      {selected.size > maxOrbitalBatch && (
        <p className="orbital-selection-error" role="alert">
          Select at most {maxOrbitalBatch} orbitals per generation.
        </p>
      )}
      <div className="orbital-browser-footer">
        <span className="orbital-selection-count" aria-live="polite">
          {selected.size} selected
        </span>
        <div className="orbital-generation-settings">
          <label>
            Isovalue
            <input
              type="number"
              aria-label="Orbital isovalue"
              value={isovalue}
              min="0"
              step="any"
              aria-invalid={!validIso}
              onChange={(event) => setIsovalue(event.target.value)}
            />
          </label>
          <label>
            Grid resolution
            <select
              aria-label="Orbital grid resolution"
              value={gridResolution}
              onChange={(event) =>
                setGridResolution(Number(event.target.value))
              }
            >
              {[24, 32, 40, 48].map((value) => (
                <option key={value} value={value}>
                  {value} x {value} x {value}
                </option>
              ))}
            </select>
          </label>
        </div>
        <button
          className="primary orbital-generate"
          disabled={
            selected.size === 0 || selected.size > maxOrbitalBatch || !validIso
          }
          onClick={() =>
            onGenerate(
              document.orbitals
                .filter((orbital) => selected.has(orbital.id))
                .map((orbital) => orbital.id),
              iso,
              gridResolution,
            )
          }
        >
          <Play size={14} />
          Generate selected
        </button>
      </div>
    </dialog>
  );
}
