import { GlobalConfig, MacroEntry } from "../types/config.types";

/**
 * porownuje dwa marka i zwraca czy sa rozne
 */
export function areMacrosDifferent(a: MacroEntry, b: MacroEntry): boolean {
  return (
    a.type !== b.type ||
    a.value !== b.value ||
    a.macroString !== b.macroString ||
    a.name !== b.name
  );
}

/**
 * znajduje roznice miedzy dwiema konfiguracjami
 * zwraca liste zmian do wyslania
 */
export interface ConfigChange {
  type: "MACRO" | "LAYER_NAME";
  layer: number;
  button?: number; // tylko dla MACRO
  data: MacroEntry | string; // MacroEntry dla MACRO, string dla LAYER_NAME
}

export function findConfigChanges(
  original: GlobalConfig,
  edited: GlobalConfig,
): ConfigChange[] {
  const changes: ConfigChange[] = [];

  // sprawdz kazda warstwe
  for (let layer = 0; layer < original.layers.length; layer++) {
    const originalLayer = original.layers[layer];
    const editedLayer = edited.layers[layer];

    // sprawdz nazwe warstwy
    if (originalLayer.name !== editedLayer.name) {
      changes.push({
        type: "LAYER_NAME",
        layer,
        data: editedLayer.name,
      });
    }

    // sprawdz kazde makro
    for (let button = 0; button < originalLayer.macros.length; button++) {
      const originalMacro = originalLayer.macros[button];
      const editedMacro = editedLayer.macros[button];

      if (areMacrosDifferent(originalMacro, editedMacro)) {
        changes.push({
          type: "MACRO",
          layer,
          button,
          data: editedMacro,
        });
      }
    }
  }

  return changes;
}

/**
 * deep clone konfiguracji
 */
export function cloneConfig(config: GlobalConfig): GlobalConfig {
  return {
    layers: config.layers.map((layer) => ({
      emoji: layer.emoji,
      name: layer.name,
      macros: layer.macros.map((macro) => ({ ...macro })),
    })),
    oledTimeout: config.oledTimeout,
  };
}
