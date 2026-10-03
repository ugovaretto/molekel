export interface Atom {
  element: number;
  position: [number, number, number];
}
export interface Orbital {
  id: string;
  label: string;
  spin: string;
  occupation: number | null;
  energy: number | null;
  coefficients: number[];
}
export interface Density {
  id: string;
  label: string;
  kind: string;
  matrix: number[];
}
export interface Grid {
  id: string;
  label: string;
  quantity: string;
  origin: number[];
  axes: number[][];
  dims: number[];
  values: number[];
}
export interface Surface {
  id: string;
  label: string;
  field: string | null;
  source_hash: string | null;
  isovalue: number | null;
  algorithm: string;
  resolution: number[];
  grid_origin: number[];
  grid_axes: number[][];
  precision: string;
  positions: number[];
  normals: number[];
  indices: number[];
  color: string;
  opacity: number;
  visible: boolean;
}
export interface MolekelDocument {
  id: string;
  title: string;
  atoms: Atom[];
  bonds: number[][];
  basis: {
    center: number[];
    exponents: number[];
    coefficients: number[];
    terms: { powers: number[]; weight: number }[];
  }[];
  orbitals: Orbital[];
  densities: Density[];
  grids: Grid[];
  surfaces: Surface[];
  view: {
    representation: string;
    positive_color: string;
    negative_color: string;
    opacity: number;
    isovalue: number;
    field: string | null;
  };
  provenance: string[];
}
export interface Generation {
  surfaces: Surface[];
  grid: Grid;
}
export interface ImportReport {
  format: string;
  warnings: string[];
}
export interface ImportResult {
  document: MolekelDocument;
  report: ImportReport;
}
export type RenderMode = "mesh" | "raycast" | "volume";
