import { KeyPress } from "../types/config.types";

export const EMOJI_STRINGS = [
  "🎮",
  "💼",
  "🏠",
  "🔧",
  "⚡",
  "📧",
  "💻",
  "🎵",
  "📝",
  "☕",
  "🗡️",
  "❤️",
  "🔔",
  "🧪",
  "🔒",
  "☂️",
  "🦕",
  "👻",
  "🔫",
  "⏳",
  "🌷",
];

export const MAX_SCRIPT_SIZE = 2048;

export function compileKeySequence(sequence: KeyPress[]): KeyPress[] {
  const compiledSteps: KeyPress[] = [];
  let pendingMods = 0;

  for (const step of sequence) {
    if (step.keycode === 0 || (step.keycode >= 224 && step.keycode <= 231)) {
      let modBit = step.modifiers;
      if (step.keycode === 224) modBit |= 1;
      if (step.keycode === 225) modBit |= 2;
      if (step.keycode === 226) modBit |= 4;
      if (step.keycode === 227) modBit |= 8;
      if (step.keycode === 228) modBit |= 16;
      if (step.keycode === 229) modBit |= 32;
      if (step.keycode === 230) modBit |= 64;
      if (step.keycode === 231) modBit |= 128;
      pendingMods |= modBit;
    } else {
      compiledSteps.push({
        keycode: step.keycode,
        modifiers: step.modifiers | pendingMods,
        duration: step.duration,
      });
      pendingMods = 0;
    }
  }

  if (pendingMods !== 0) {
    compiledSteps.push({ keycode: 0, modifiers: pendingMods });
  }

  return compiledSteps;
}

export function getEmojiIndex(emoji: string): number {
  return EMOJI_STRINGS.indexOf(emoji);
}

export function getEmojiString(index: number): string {
  return EMOJI_STRINGS[index] || "🎮";
}
