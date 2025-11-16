/**
 * typy makr zgodne z firmware
 */
export enum MacroType {
  KEY_PRESS = 0,
  MACRO_STRING = 1,
  LAYER_TOGGLE = 2,
  SCRIPT = 3,
}

/**
 * mapowanie enum -> czytelne nazwy
 */
export const MacroTypeLabels: Record<MacroType, string> = {
  [MacroType.KEY_PRESS]: "Key Press",
  [MacroType.MACRO_STRING]: "Text Macro",
  [MacroType.LAYER_TOGGLE]: "Layer Toggle",
  [MacroType.SCRIPT]: "Script Execution",
};

export enum ScriptPlatform {
  LINUX = 0,
  WINDOWS = 1,
  MACOS = 2,
}

export const ScriptPlatformLabels: Record<ScriptPlatform, string> = {
  [ScriptPlatform.LINUX]: "Linux (bash)",
  [ScriptPlatform.WINDOWS]: "Windows (batch/PowerShell)",
  [ScriptPlatform.MACOS]: "macOS (bash/zsh)",
};

/**
 * pojedyncze makro
 */
export interface MacroEntry {
  type: MacroType;
  value: number;
  macroString: string;
  name: string;
  emoji: string;
  script?: string;
  scriptPlatform?: ScriptPlatform;
}

/**
 * globalna konfiguracja
 */
export interface GlobalConfig {
  layers: LayerConfig[];
}

export interface LayerConfig {
  name: string;
  emoji: string;
  macros: MacroEntry[];
}

export const DEFAULT_LAYER_EMOJIS = ["🎮", "💼", "🏠", "⚙️"];
export const DEFAULT_BUTTON_EMOJI = "";
export const LAYER_SWITCH_EMOJI = "⚡";
export const MAX_SCRIPT_SIZE = 2048; // 2KB

/**
 * stale z firmware
 */
export const FIRMWARE_CONSTANTS = {
  NUM_BUTTONS: 7,
  MAX_LAYERS: 4,
  MAX_NAME_LEN: 16,
  MACRO_STRING_LEN: 32,
  MAX_EMOJI_LEN: 8,
} as const;

/**
 * status polaczenia
 */
export type ConnectionStatus =
  | "DISCONNECTED"
  | "CONNECTING"
  | "CONNECTED"
  | "ERROR";

/**
 * blad polaczenia
 */
export interface ConnectionError {
  type:
    | "BROWSER_NOT_SUPPORTED"
    | "DEVICE_NOT_FOUND"
    | "PROTOCOL_ERROR"
    | "UNKNOWN";
  message: string;
}
