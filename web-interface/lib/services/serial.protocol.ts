import { MacroType, KeyPress } from "../types/config.types";
import { compileKeySequence, getEmojiIndex } from "./serial.utils";

export class SerialProtocol {
  static buildSetMacroCommand(
    layer: number,
    button: number,
    type: MacroType,
    value: number,
    macroString: string,
    name: string,
    emoji: string,
    repeatCount: number = 1,
    repeatInterval: number = 0,
    moveX: number = 0,
    moveY: number = 0,
  ): string {
    const emojiIndex = getEmojiIndex(emoji);
    return `SET_MACRO|${layer}|${button}|${type}|${value}|${macroString}|${name}|${emojiIndex}|${repeatCount}|${repeatInterval}|${moveX}|${moveY}`;
  }

  static buildSequenceCommand(
    layer: number,
    button: number,
    name: string,
    emoji: string,
    keySequence: KeyPress[],
  ): string {
    const emojiIndex = getEmojiIndex(emoji);
    const steps = compileKeySequence(keySequence).slice(0, 3); // max 3 steps limit hardware
    const stepsStr = steps
      .map((s) => `${s.keycode},${s.modifiers},${s.duration || 50}`)
      .join(",");

    return `SET_MACRO_SEQ|${layer}|${button}|${name}|${emojiIndex}|${steps.length}|${stepsStr}`;
  }

  static buildLayerNameCommand(
    layer: number,
    name: string,
    emoji: string,
  ): string {
    const emojiIndex = getEmojiIndex(emoji);
    return `SET_LAYER_NAME|${layer}|${name}|${emojiIndex}`;
  }
}
