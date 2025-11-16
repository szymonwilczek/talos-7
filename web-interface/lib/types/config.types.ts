// ==================== ENUMS ====================

export enum MacroType {
  KEY_PRESS = 0,
  TEXT_STRING = 1,
  LAYER_TOGGLE = 2,
  SCRIPT = 3,
}

export type ConnectionStatus =
  | "DISCONNECTED"
  | "CONNECTING"
  | "CONNECTED"
  | "ERROR";

// ==================== INTERFACES ====================

export interface MacroEntry {
  type: MacroType;
  value: number;
  macroString: string;
  name: string;
  emoji: string;
  script?: string;
  scriptPlatform?: number;
}

export interface LayerConfig {
  name: string;
  emoji: string;
  macros: MacroEntry[];
}

export interface GlobalConfig {
  layers: LayerConfig[];
}

export interface ConnectionError {
  type:
    | "BROWSER_NOT_SUPPORTED"
    | "DEVICE_NOT_FOUND"
    | "PROTOCOL_ERROR"
    | "PERMISSION_DENIED";
  message: string;
}

// ==================== CONSTANTS ====================

export const DEFAULT_LAYER_EMOJIS = ["🎮", "💼", "🏠", "⚙️"];
export const DEFAULT_BUTTON_EMOJI = "";
export const LAYER_SWITCH_EMOJI = "⚡";

export const FIRMWARE_CONSTANTS = {
  MAX_LAYERS: 4,
  NUM_BUTTONS: 7,
  MAX_NAME_LEN: 16,
  MACRO_STRING_LEN: 32,
  MAX_EMOJI_LEN: 8,
  MAX_SCRIPT_SIZE: 2048, // 2KB
} as const;

// ==================== HELPER TYPES ====================

export type ChangeType = "layer" | "macro";

export interface LayerChange {
  type: "layer";
  layer: number;
  name: string;
  emoji: string;
}

export interface MacroChange {
  type: "macro";
  layer: number;
  button: number;
  macro: MacroEntry;
}

export type ConfigChange = LayerChange | MacroChange;

// ==================== VALIDATORS ====================

export function isValidLayerName(name: string): boolean {
  return name.length > 0 && name.length <= FIRMWARE_CONSTANTS.MAX_NAME_LEN;
}

export function isValidMacroName(name: string): boolean {
  return name.length > 0 && name.length <= FIRMWARE_CONSTANTS.MAX_NAME_LEN;
}

export function isValidMacroString(str: string): boolean {
  return str.length <= FIRMWARE_CONSTANTS.MACRO_STRING_LEN;
}

export function isValidEmoji(emoji: string): boolean {
  // emoji moze byc puste lub max 8 bajtow UTF-8
  const encoder = new TextEncoder();
  const bytes = encoder.encode(emoji);
  return bytes.length <= FIRMWARE_CONSTANTS.MAX_EMOJI_LEN;
}

export function isValidKeycode(keycode: number): boolean {
  return keycode >= 0 && keycode <= 255;
}

export function isValidLayerIndex(layer: number): boolean {
  return layer >= 0 && layer < FIRMWARE_CONSTANTS.MAX_LAYERS;
}

export function isValidButtonIndex(button: number): boolean {
  return button >= 0 && button < FIRMWARE_CONSTANTS.NUM_BUTTONS;
}

// ==================== FACTORIES ====================

export function createEmptyMacro(): MacroEntry {
  return {
    type: MacroType.KEY_PRESS,
    value: 0,
    macroString: "",
    name: "Empty",
    emoji: "",
  };
}

export function createLayerSwitchMacro(targetLayer: number): MacroEntry {
  return {
    type: MacroType.LAYER_TOGGLE,
    value: targetLayer,
    macroString: "",
    name: "LayerSwitch",
    emoji: LAYER_SWITCH_EMOJI,
  };
}

export function createDefaultLayer(index: number): LayerConfig {
  return {
    name: `Layer ${index + 1}`,
    emoji: DEFAULT_LAYER_EMOJIS[index] || "⚙️",
    macros: Array.from(
      { length: FIRMWARE_CONSTANTS.NUM_BUTTONS },
      (_, btnIdx) => {
        // BTN7 (index 6) = Layer Switch
        if (btnIdx === 6) {
          return createLayerSwitchMacro(
            (index + 1) % FIRMWARE_CONSTANTS.MAX_LAYERS,
          );
        }
        return createEmptyMacro();
      },
    ),
  };
}

export function createDefaultConfig(): GlobalConfig {
  return {
    layers: Array.from({ length: FIRMWARE_CONSTANTS.MAX_LAYERS }, (_, i) =>
      createDefaultLayer(i),
    ),
  };
}

// ==================== TYPE GUARDS ====================

export function isMacroType(value: unknown): value is MacroType {
  return (
    value === MacroType.KEY_PRESS ||
    value === MacroType.TEXT_STRING ||
    value === MacroType.LAYER_TOGGLE ||
    value === MacroType.SCRIPT
  );
}

export function isConnectionStatus(value: unknown): value is ConnectionStatus {
  return (
    value === "DISCONNECTED" ||
    value === "CONNECTING" ||
    value === "CONNECTED" ||
    value === "ERROR"
  );
}

export function isLayerChange(change: ConfigChange): change is LayerChange {
  return change.type === "layer";
}

export function isMacroChange(change: ConfigChange): change is MacroChange {
  return change.type === "macro";
}

// ==================== FORMATTERS ====================

export function formatMacroType(type: MacroType): string {
  switch (type) {
    case MacroType.KEY_PRESS:
      return "Key Press";
    case MacroType.TEXT_STRING:
      return "Text Macro";
    case MacroType.LAYER_TOGGLE:
      return "Layer Toggle";
    case MacroType.SCRIPT:
      return "Script Execution";
    default:
      return "Unknown";
  }
}

export function formatKeycode(keycode: number): string {
  // Mapowanie podstawowych keycodów HID
  const keycodeMap: Record<number, string> = {
    4: "A",
    5: "B",
    6: "C",
    7: "D",
    8: "E",
    9: "F",
    10: "G",
    11: "H",
    12: "I",
    13: "J",
    14: "K",
    15: "L",
    16: "M",
    17: "N",
    18: "O",
    19: "P",
    20: "Q",
    21: "R",
    22: "S",
    23: "T",
    24: "U",
    25: "V",
    26: "W",
    27: "X",
    28: "Y",
    29: "Z",
    30: "1",
    31: "2",
    32: "3",
    33: "4",
    34: "5",
    35: "6",
    36: "7",
    37: "8",
    38: "9",
    39: "0",
    40: "Enter",
    41: "Escape",
    42: "Backspace",
    43: "Tab",
    44: "Space",
    58: "F1",
    59: "F2",
    60: "F3",
    61: "F4",
    62: "F5",
    63: "F6",
    64: "F7",
    65: "F8",
    66: "F9",
    67: "F10",
    68: "F11",
    69: "F12",
    79: "→",
    80: "←",
    81: "↓",
    82: "↑",
  };

  return keycodeMap[keycode] || `Key ${keycode}`;
}

// ==================== COMPARATORS ====================

export function macrosEqual(a: MacroEntry, b: MacroEntry): boolean {
  return (
    a.type === b.type &&
    a.value === b.value &&
    a.macroString === b.macroString &&
    a.name === b.name &&
    a.emoji === b.emoji &&
    a.script === b.script &&
    a.scriptPlatform === b.scriptPlatform
  );
}

export function layersEqual(a: LayerConfig, b: LayerConfig): boolean {
  if (a.name !== b.name || a.emoji !== b.emoji) return false;
  if (a.macros.length !== b.macros.length) return false;

  for (let i = 0; i < a.macros.length; i++) {
    if (!macrosEqual(a.macros[i], b.macros[i])) return false;
  }

  return true;
}

export function configsEqual(a: GlobalConfig, b: GlobalConfig): boolean {
  if (a.layers.length !== b.layers.length) return false;

  for (let i = 0; i < a.layers.length; i++) {
    if (!layersEqual(a.layers[i], b.layers[i])) return false;
  }

  return true;
}
