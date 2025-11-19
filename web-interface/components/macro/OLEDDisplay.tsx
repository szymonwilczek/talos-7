'use client';

import { LayerConfig, KeyPress, MODIFIERS } from '@/lib/types/config.types';
import { Card, CardContent } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { Label } from '@/components/ui/label';
import { Textarea } from '@/components/ui/textarea';
import { useEffect, useRef } from 'react';
import { OLEDAlert } from './OLEDAlert';

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

// emoji z firmware
const emojiBitmaps: number[][] = [
  [0x38, 0x44, 0x94, 0x44, 0x46, 0x95, 0x44, 0x38], // 0: 🎮
  [0x7c, 0x46, 0x4a, 0x5a, 0x5a, 0x4a, 0x46, 0x7c], // 1: 💼
  [0x10, 0xf8, 0x8c, 0xe6, 0x86, 0x8c, 0xf8, 0x10], // 2: 🏠
  [0x00, 0x00, 0xc7, 0x7c, 0x7c, 0xc7, 0x00, 0x00], // 3: 🔧
  [0x00, 0x80, 0xc8, 0x6c, 0x3e, 0x1b, 0x09, 0x00], // 4: ⚡
  [0x00, 0x3e, 0x41, 0x5d, 0x51, 0x4e, 0x20, 0x00], // 5: 📧
  [0x3e, 0x22, 0xa2, 0xe2, 0xe2, 0xa2, 0x22, 0x3e], // 6: 💻
  [0xc0, 0xfe, 0xc2, 0x02, 0x62, 0x7e, 0x60, 0x00], // 7: 🎵
  [0xc0, 0xa0, 0x50, 0x28, 0x1c, 0x0a, 0x07, 0x03], // 8: 📝
  [0x7e, 0xc2, 0xc2, 0xc2, 0xc2, 0xfe, 0x64, 0x3c], // 9: ☕
  [0xd8, 0xf0, 0x78, 0x7c, 0x5e, 0x0f, 0x07, 0x03], // 10: 🗡️
  [0x00, 0x0c, 0x12, 0x22, 0x44, 0x22, 0x12, 0x0c], // 11: ❤️
  [0x40, 0x7c, 0xc6, 0xc6, 0x7c, 0x40, 0x00, 0x00], // 12: 🔔
  [0x40, 0xa0, 0x91, 0x8f, 0x8f, 0x91, 0xa0, 0x40], // 13: 🧪
  [0x78, 0xfe, 0xf9, 0xc9, 0xf9, 0xfe, 0x78, 0x00], // 14: 🔒
  [0x0c, 0x0e, 0x4f, 0x8f, 0x7f, 0x0f, 0x0e, 0x0c], // 15: ☂️
  [0x06, 0x15, 0xff, 0x7d, 0xff, 0x40, 0x30, 0x00], // 16: 🦕
  [0x3e, 0x45, 0x47, 0xf5, 0xcf, 0x83, 0x82, 0x84], // 17: 👻
  [0x07, 0x06, 0x06, 0x0e, 0x16, 0x96, 0xfe, 0x7c], // 18: 🔫
  [0x82, 0xee, 0x92, 0x92, 0x92, 0xee, 0x82, 0x00], // 19: ⏳
  [0x20, 0x4f, 0x9e, 0xff, 0x9e, 0x4f, 0x20, 0x00], // 20: 🌷
];

const EmojiCanvas: React.FC<{ emojiIndex: number; color: string, size?: number }> = ({ emojiIndex, color, size = 16 }) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const bitmap = emojiBitmaps[emojiIndex] || emojiBitmaps[0];
    const scale = size / 8;

    ctx.clearRect(0, 0, size, size);
    for (let x = 0; x < 8; x++) {
      for (let y = 0; y < 8; y++) {
        if (bitmap[x] & (1 << y)) {
          ctx.fillStyle = color;
          ctx.fillRect(x * scale, y * scale, scale, scale);
        }
      }
    }

    ctx.imageSmoothingEnabled = false;
  }, [emojiIndex, size]);

  return <canvas ref={canvasRef} width={size} height={size} className="inline-block" />;
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

  const formatKeyPress = (kp: KeyPress): string => {
    const parts: string[] = [];
    if (kp.modifiers & MODIFIERS.CTRL) parts.push('Ctrl');
    if (kp.modifiers & MODIFIERS.SHIFT) parts.push('Shift');
    if (kp.modifiers & MODIFIERS.ALT) parts.push('Alt');
    if (kp.modifiers & MODIFIERS.GUI) parts.push('⊞');
    parts.push(getKeyName(kp.keycode));
    return parts.join('+');
  };

  const formatSequenceVim = (sequence?: KeyPress[]): string => {
    if (!sequence || sequence.length === 0) return 'Empty';
    let buf = '';
    // pierwszy klawisz z modyfikatorami (vim like)
    const first = sequence[0];
    let mods = '';
    if (first.modifiers & MODIFIERS.CTRL) mods += 'C';
    if (first.modifiers & MODIFIERS.SHIFT) mods += 'S';
    if (first.modifiers & MODIFIERS.ALT) mods += 'A';
    if (first.modifiers & MODIFIERS.GUI) mods += 'M';
    if (mods) {
      buf += `<${mods}-${getKeyName(first.keycode)}>`;
    } else {
      buf += getKeyName(first.keycode);
    }
    // nastepne klawisze
    for (let i = 1; i < sequence.length; i++) {
      buf += '+' + getKeyName(sequence[i].keycode);
    }
    return buf;
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
        return { text: `Layer Toggle to Layer ${macro.value + 1}`, alert: null };
      case 3: // Script
        const platform = macro.scriptPlatform === 0 ? 'Linux' :
          macro.scriptPlatform === 1 ? 'Windows' : 'macOS';
        return { text: `Script (${platform})`, alert: null };
      case 4: // KeySequence
        return { text: `Sequence: ${formatSequenceVim(macro.keySequence)}`, alert: null };
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
            <div className="flex items-center justify-between gap-2 -mt-4 shrink-0">
              <div className='flex items-center gap-2'>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.macros[activeButton].emoji)} color="#00ffff" size={20} />
                <span className='max-md:text-sm max-lg:text-md text-xl text-[#00ffff] truncate max-w-[120px]'>
                  {layer.macros[activeButton].name || `Button ${activeButton + 1}`}
                </span>
              </div>
              <Badge variant={selectedButton !== null ? 'default' : 'outline'} className="shrink-0">
                {selectedButton !== null ? 'Editing' : 'Preview'}
              </Badge>
            </div>

            <Separator className="bg-[#00ffff] shrink-0" />

            <div className="flex-1 flex items-center justify-center text-center min-h-0">
              <div className="text-xl text-[#fff000] break-words w-full px-2">
                {details?.text}
              </div>
            </div>

            {/* Script Preview */}
            {layer.macros[activeButton].type === 3 && (
              <div className="mt-1 shrink-0">
                <Label className="text-xs text-gray-400 block mb-2">Script Content (first 50 chars):</Label>
                <Textarea
                  value={layer.macros[activeButton].script?.substring(0, 50) || ''}
                  readOnly
                  className="text-xs font-mono bg-gray-800 text-white border-gray-600 resize-none h-12"
                />
              </div>
            )}

            {details?.alert && (
              <OLEDAlert variant={details.alert} />
            )}
          </div>
        ) : (
          <div className="space-y-3">
            <div className="flex items-center justify-between gap-2 -mt-4">
              <div className='flex items-center gap-2'>
                <EmojiCanvas emojiIndex={getEmojiIndex(layer.emoji)} color="#00ffff" size={20} />
                <span className='max-md:text-sm max-lg:text-md text-xl'>{layer.name || `Layer ${layerIndex + 1}/${totalLayers}`}</span>
              </div>
              <Badge variant={selectedButton !== null ? 'default' : 'outline'}>
                {selectedButton !== null ? 'Editing' : 'Preview'}
              </Badge>
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
    </Card >
  );
}
