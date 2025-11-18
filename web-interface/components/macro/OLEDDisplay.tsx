'use client';

import { LayerConfig, KeyPress, MODIFIERS } from '@/lib/types/config.types';
import { Card, CardContent } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { Label } from '@/components/ui/label';
import { Textarea } from '@/components/ui/textarea';

interface OLEDDisplayProps {
  layer: LayerConfig;
  layerIndex: number;
  totalLayers: number;
  selectedButton: number | null;
  hoveredButton: number | null;
}

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

  const formatSequence = (sequence?: KeyPress[]): string => {
    if (!sequence || sequence.length === 0) return 'Empty';
    return sequence.map(formatKeyPress).join('+');
  };

  const getActionDetails = (macro: any) => {
    switch (macro.type) {
      case 0: // KeyPress
        return `Key: ${getKeyName(macro.value)}`;
      case 1: // TextString
        return `Text: "${macro.macroString || ''}"`;
      case 2: // LayerToggle
        return `Layer Toggle (to Layer ${macro.value + 1})`;
      case 3: // Script
        const platform = macro.scriptPlatform === 0 ? 'Linux' :
          macro.scriptPlatform === 1 ? 'Windows' : 'macOS';
        return `Script (${platform})`;
      case 4: // KeySequence
        return `Sequence: ${formatSequence(macro.keySequence)}`;
      default:
        return 'Unknown Action';
    }
  };

  return (
    <Card className="w-full max-w-2xl h-[290px] bg-black border-2 border-foreground/30">
      <CardContent className="h-full p-4 text-white text-sm font-mono overflow-auto">
        {activeButton !== null ? (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <Badge variant={selectedButton !== null ? 'default' : 'outline'}>
                {selectedButton !== null ? 'Editing' : 'Preview'}
              </Badge>
              <span>Button {activeButton + 1}</span>
            </div>
            <Separator className="bg-gray-700" />
            <div className="text-center space-y-2">
              <div className="text-5xl">{layer.macros[activeButton].emoji || ''}</div>
              <div className="text-lg">{layer.macros[activeButton].name}</div>
              <div className="text-xs text-gray-400">
                {getActionDetails(layer.macros[activeButton])}
              </div>
            </div>

            {/* Szczegóły dla Script */}
            {layer.macros[activeButton].type === 3 && (
              <div className="mt-4">
                <Label className="text-xs text-gray-400 block mb-2">Script Content (first 50 chars):</Label>
                <Textarea
                  value={layer.macros[activeButton].script?.substring(0, 50) || ''}
                  readOnly
                  className="text-xs font-mono bg-gray-800 text-white border-gray-600 resize-none"
                  rows={3}
                />
              </div>
            )}
          </div>
        ) : (
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <span className="flex items-center gap-2">
                <span className="text-xl">{layer.emoji}</span>
                <span>{layer.name}</span>
              </span>
              <Badge variant="outline">
                Layer {layerIndex + 1}/{totalLayers}
              </Badge>
            </div>
            <Separator className="bg-gray-700" />
            <div className="space-y-1">
              {layer.macros.slice(0, 6).map((macro, idx) =>
                macro.name !== 'Empty' ? (
                  <div key={idx} className="flex items-center gap-2 text-xs">
                    <Badge variant="secondary">{idx + 1}</Badge>
                    <span>{macro.emoji}</span>
                    <span>{macro.name}</span>
                    <span className="text-gray-400">({getActionDetails(macro)})</span>
                  </div>
                ) : null
              )}
            </div>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
