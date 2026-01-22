'use client';

import { LayerConfig, KeyPress, MODIFIERS } from '@/lib/types/config.types';
import { Card, CardContent } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { OLEDAlert } from './oled-alert';
import { EmojiCanvas } from './emoji-canvas';

interface OLEDDisplayProps {
  layer: LayerConfig;
  layerIndex: number;
  totalLayers: number;
  selectedButton: number | null;
  hoveredButton: number | null;
}

const emoji_strings = [
  "🎮", "💼", "🏠", "🔧", "⚡", "📧", "💻", "🎵", "📝", "☕", "🗡️", "❤️",
  "🔔", "🧪", "🔒", "☂️", "🦕", "👻", "🔫", "⏳", "🌷"
];

const getEmojiIndex = (emoji: string): number => {
  const index = emoji_strings.indexOf(emoji);
  return index >= 0 ? index : 0; // gamepad
};

export function OLEDDisplay({
  layer,
  layerIndex,
  totalLayers,
  selectedButton,
  hoveredButton,
}: OLEDDisplayProps) {
  const activeButton = selectedButton ?? hoveredButton;

  const getKeyName = (keycode: number): string => {
    const keyNames: Record<number, string> = {
      4: 'A', 5: 'B', 6: 'C', 7: 'D', 8: 'E', 9: 'F', 10: 'G', 11: 'H',
      12: 'I', 13: 'J', 14: 'K', 15: 'L', 16: 'M', 17: 'N', 18: 'O',
      19: 'P', 20: 'Q', 21: 'R', 22: 'S', 23: 'T', 24: 'U', 25: 'V',
      26: 'W', 27: 'X', 28: 'Y', 29: 'Z',
      30: '1', 31: '2', 32: '3', 33: '4', 34: '5',
      35: '6', 36: '7', 37: '8', 38: '9', 39: '0',
      40: 'Enter', 41: 'Esc', 42: 'Backspace', 43: 'Tab', 44: 'Space',
      58: 'F1', 59: 'F2', 60: 'F3', 61: 'F4', 62: 'F5', 63: 'F6',
      64: 'F7', 65: 'F8', 66: 'F9', 67: 'F10', 68: 'F11', 69: 'F12',
      79: '→', 80: '←', 81: '↓', 82: '↑',
    };
    return keyNames[keycode] || `Key ${keycode}`;
  };

  const formatStepVim = (kp: KeyPress) => {
    const parts: string[] = [];
    if (kp.modifiers & MODIFIERS.GUI) parts.push('M');
    if (kp.modifiers & MODIFIERS.CTRL) parts.push('C');
    if (kp.modifiers & MODIFIERS.SHIFT) parts.push('S');
    if (kp.modifiers & MODIFIERS.ALT) parts.push('A');

    const keyName = getKeyName(kp.keycode);
    // jesli keyName to nie "Key 0" (pusty) to dodaj
    if (kp.keycode !== 0) parts.push(keyName);

    if (parts.length > 0) {
      // logika nawiasow < >
      if (kp.modifiers !== 0 || keyName.length > 1) {
        return `<${parts.join('-')}>`;
      }
      return keyName;
    }
    return '';
  };

  // glowna funkcja wyswietlajaca sekwencje (sklejanie krokow)
  const formatSequence = (sequence?: KeyPress[]): string => {
    if (!sequence || sequence.length === 0) return 'Empty';

    const parts: string[] = [];
    let pendingMods = 0;

    for (const step of sequence) {
      // jesli to modyfikator (0 lub 224-231)
      if (step.keycode === 0 || (step.keycode >= 224 && step.keycode <= 231)) {
        // mapowanie z inputa (modifiers ma juz bit, albo keycode to mod)
        let modBit = step.modifiers;
        if (step.keycode === 224) modBit |= 1;
        if (step.keycode === 225) modBit |= 2;
        if (step.keycode === 226) modBit |= 4;
        if (step.keycode === 227) modBit |= 8;
        pendingMods |= modBit;
      } else {
        // zwykly klawisz
        parts.push(formatStepVim({
          keycode: step.keycode,
          modifiers: step.modifiers | pendingMods
        }));
        pendingMods = 0;
      }
    }
    // wiszacy modyfikator na koncu
    if (pendingMods !== 0) {
      parts.push(formatStepVim({ keycode: 0, modifiers: pendingMods }));
    }

    return parts.join('+');
  };

  const analyzeString = (str: string) => {
    let hasUnicode = false;
    let hasVisibleAscii = false;
    let truncated = "";
    let count = 0;
    const LIMIT = 10;
    let originalLength = 0;

    for (const char of str) {
      const cp = char.codePointAt(0) || 0;
      originalLength++;

      if (count < LIMIT) {
        if (cp > 127) {
          truncated += "?";
        } else {
          truncated += char;
        }
        count++;
      }

      if (cp > 127) {
        hasUnicode = true;
      } else if (cp > 32 && cp < 127) {
        hasVisibleAscii = true;
      }
    }

    if (originalLength > LIMIT) {
      truncated += "...";
    }

    return { hasUnicode, hasVisibleAscii, truncated };
  };

  const mapMouseButton = (buttonCode: number): string => {
    const buttonMap: Record<number, string> = {
      1: 'Left',
      2: 'Right',
      4: 'Middle',
    }
    return buttonMap[buttonCode] || `Button ${buttonCode}`;
  };

  const getActionDetails = (macro: any) => {
    switch (macro.type) {
      case 0: // KeyPress
        return { text: `Key: ${getKeyName(macro.value)}`, alert: null };
      case 1: { // TextString
        const { hasUnicode, hasVisibleAscii, truncated } = analyzeString(macro.macroString || '');

        if (hasUnicode && !hasVisibleAscii) {
          // pure unicode - firmware replacement
          return {
            text: "Text: [Unicode/Emoji]",
            alert: "pure" as const
          };
        } else if (hasUnicode) {
          // mixed - firmware shows '?'
          return {
            text: `Text: "${truncated}"`,
            alert: "mixed" as const
          };
        } else {
          // pure ASCII
          return { text: `Text: "${truncated}"`, alert: null };
        }
      }
      case 2: // LayerToggle
        return { text: `Layer Toggle` };
      case 3: // Script
        const platformName = macro.scriptPlatform === 0 ? 'Linux' :
          macro.scriptPlatform === 1 ? 'Windows' : 'macOS';
        return { text: `Script (${platformName})`, alert: null };
      case 4: // KeySequence
        return { text: `Sequence: ${formatSequence(macro.keySequence)}`, alert: null };
      case 5: // Mouse Button
        return { text: `Mouse Button: ${mapMouseButton(macro.value)}`, alert: null };
      case 6: // Mouse Move
        return { text: `Mouse Move: X=${macro.moveX}, Y=${macro.moveY}`, alert: null };
      case 7: // Mouse Wheel
        return { text: `Mouse Wheel (x${Math.abs(macro.value)})`, alert: null };
      case 8: // MIDI Note
        return { text: `Note: ${macro.midiNote}, Vel: ${macro.midiVelocity}, Ch: ${macro.midiChannel}`, alert: null };
      case 9: // MIDI CC
        return { text: `CC: ${macro.midiCCNumber}, Value: ${macro.midiCCValue}, Ch: ${macro.midiChannel}`, alert: null };
      case 10: // GAME
        return { text: "Atari Breakout", alert: null };
      default:
        return { text: 'Unknown Action', alert: null };
    }
  };

  const details = activeButton !== null ? getActionDetails(layer.macros[activeButton]) : null;

  return (
    <Card className="w-[252px] h-[128px] md:w-[378px] md:h-[192px] lg:w-[350px] lg:h-[210px] bg-[#000000] border-2 border-foreground/30 font-mono relative" style={{
      fontFamily: 'PixelMix, monospace',
      fontSize: '12px',
      lineHeight: '1',
      WebkitFontSmoothing: 'none',
      MozOsxFontSmoothing: 'none',
      textRendering: 'optimizeSpeed',
      fontSynthesis: 'none',
      imageRendering: 'pixelated'
    }}>
      <CardContent className="h-full p-4 text-[#00ffff] text-sm overflow-hidden relative">
        {activeButton !== null ? (
          <div className="flex flex-col h-full gap-3">
            <div className="flex items-center justify-center gap-2 -mt-4 shrink-0">
              <div className='flex items-center gap-2'>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[activeButton].emoji)} color="#00ffff" size={20} />
                <span className='max-md:text-sm max-lg:text-md text-xl text-[#00ffff] truncate max-w-[120px]'>
                  {layer.macros[activeButton].name || `Button ${activeButton + 1}`}
                </span>
              </div>
            </div>

            <Separator className="bg-[#00ffff] shrink-0" />

            <div className="flex-1 flex items-center justify-center text-center min-h-0">
              <div className="text-xl text-[#fff000] break-words w-full px-2">
                {details?.text}
              </div>
            </div>

            {details?.alert && (
              <OLEDAlert variant={details.alert} />
            )}
          </div>
        ) : (
          <div className="space-y-3">
            <div className="flex items-center justify-center gap-2 -mt-4">
              <div className='flex items-center gap-2'>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.emoji)} color="#00ffff" size={20} />
                <span className='max-md:text-sm max-lg:text-md text-xl'>{layer.name || `Layer ${layerIndex + 1}/${totalLayers}`}</span>
              </div>
            </div>
            <Separator className="bg-[#00ffff]" />
            <div className="grid grid-cols-3 gap-2 text-start">
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[1]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[0].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[2]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[1].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[3]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[2].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[4]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[3].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[5]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[4].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="flex flex-row items-center justify-center">
                <span className="max-md:text-sm max-lg:text-md text-xl">[6]</span>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[5].emoji)} color="#00ffff" size={20} />
              </div>
              <div className="col-span-3 flex justify-center">
                <div className="flex flex-row items-center justify-center text-[#fff000]">
                  <span className="max-md:text-sm max-lg:text-md text-xl">[7]</span>
                  <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[6].emoji)} color="#fff000" size={20} />
                </div>
              </div>
            </div>
          </div>
        )}
      </CardContent>
      <div className="absolute top-2 right-1">
        <Badge className="opacity-80" variant={selectedButton !== null ? 'default' : 'outline'}>
          {selectedButton !== null ? 'Editing' : 'Preview'}
        </Badge>
      </div>
    </Card >
  );
}
