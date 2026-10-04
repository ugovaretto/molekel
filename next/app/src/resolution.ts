export const gridResolutions = [
  24, 32, 40, 48, 64, 80, 96, 128, 160, 192, 224, 256,
] as const;

export function isGridResolution(value: number): boolean {
  return gridResolutions.some((resolution) => resolution === value);
}
