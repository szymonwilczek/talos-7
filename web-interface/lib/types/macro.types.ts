/**
 * typy makr zgodne z firmware
 */
export enum MacroType {
  KEY_PRESS = 0,
  MACRO_STRING = 1,
  LAYER_TOGGLE = 2,
}

/**
 * mapowanie enum -> czytelne nazwy
 */
export const MacroTypeLabels: Record<MacroType, string> = {
  [MacroType.KEY_PRESS]: 'Key Press',
  [MacroType.MACRO_STRING]: 'Text Macro',
  [MacroType.LAYER_TOGGLE]: 'Layer Toggle',
};

/**
 * pojedyncze makro
 */
export interface MacroEntry {
  type: MacroType;
  value: number;
  macroString: string;
  name: string;
}

/**
 * globalna konfiguracja
 */
export interface GlobalConfig {
  layers: LayerConfig[];
}

export interface LayerConfig {
  name: string;
  macros: MacroEntry[];
}

/**
 * stale z firmware
 */
export const FIRMWARE_CONSTANTS = {
  NUM_BUTTONS: 7,
  MAX_LAYERS: 4,
  MAX_NAME_LEN: 16,
  MACRO_STRING_LEN: 32,
} as const;

/**
 * status polaczenia
 */
export type ConnectionStatus = 'DISCONNECTED' | 'CONNECTING' | 'CONNECTED' | 'ERROR';

/**
 * blad polaczenia
 */
export interface ConnectionError {
  type: 'BROWSER_NOT_SUPPORTED' | 'DEVICE_NOT_FOUND' | 'PROTOCOL_ERROR' | 'UNKNOWN';
  message: string;
}
