'use client';

import { useState } from 'react';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { X, Plus } from 'lucide-react';
import { Card, CardContent } from '@/components/ui/card';
import { KeyPress, MODIFIERS } from '@/lib/types/config.types';

interface KeySequenceInputProps {
  sequence: KeyPress[];
  onChange: (sequence: KeyPress[]) => void;
}

export function KeySequenceInput({ sequence, onChange }: KeySequenceInputProps) {
  const [recording, setRecording] = useState(false);
  const [currentMods, setCurrentMods] = useState<number>(0);

  const isModifierKey = (keycode: number) => keycode >= 224 && keycode <= 231;

  // zamiana kodu modyfikatora na flage bitowa
  const getModifierFlag = (keycode: number): number => {
    switch (keycode) {
      case 224: return MODIFIERS.CTRL;
      case 225: return MODIFIERS.SHIFT;
      case 226: return MODIFIERS.ALT;
      case 227: return MODIFIERS.GUI;
      case 228: return MODIFIERS.RIGHT_CTRL;
      case 229: return MODIFIERS.RIGHT_SHIFT;
      case 230: return MODIFIERS.RIGHT_ALT;
      case 231: return MODIFIERS.RIGHT_GUI;
      default: return 0;
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (!recording) return;

    e.preventDefault();
    e.stopPropagation();

    if (sequence.length >= 3) {
      console.log('Maximum sequence length reached (3 steps)');
      setRecording(false);
      return;
    }

    let mods = currentMods;
    if (e.ctrlKey) mods |= MODIFIERS.CTRL;
    else mods &= ~MODIFIERS.CTRL;
    if (e.shiftKey) mods |= MODIFIERS.SHIFT;
    else mods &= ~MODIFIERS.SHIFT;
    if (e.altKey) mods |= MODIFIERS.ALT;
    else mods &= ~MODIFIERS.ALT;
    if (e.metaKey) mods |= MODIFIERS.GUI;
    else mods &= ~MODIFIERS.GUI;

    setCurrentMods(mods);

    if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) {
      return;
    }

    const keyMap: Record<string, number> = {
      'control': 224, 'shift': 225, 'alt': 226, 'meta': 227,
      'a': 4, 'b': 5, 'c': 6, 'd': 7, 'e': 8, 'f': 9, 'g': 10, 'h': 11,
      'i': 12, 'j': 13, 'k': 14, 'l': 15, 'm': 16, 'n': 17, 'o': 18,
      'p': 19, 'q': 20, 'r': 21, 's': 22, 't': 23, 'u': 24, 'v': 25,
      'w': 26, 'x': 27, 'y': 28, 'z': 29,
      '1': 30, '2': 31, '3': 32, '4': 33, '5': 34,
      '6': 35, '7': 36, '8': 37, '9': 38, '0': 39,
      'Enter': 40, 'Escape': 41, 'Backspace': 42, 'Tab': 43, ' ': 44,
      'F1': 58, 'F2': 59, 'F3': 60, 'F4': 61, 'F5': 62, 'F6': 63,
      'F7': 64, 'F8': 65, 'F9': 66, 'F10': 67, 'F11': 68, 'F12': 69,
      'ArrowRight': 79, 'ArrowLeft': 80, 'ArrowDown': 81, 'ArrowUp': 82,
      '-': 45, '=': 46, '[': 47, ']': 48, '\\': 49, ';': 51, "'": 52,
      ',': 54, '.': 55, '/': 56,
    };

    const key = e.key.toLowerCase();
    const hid = keyMap[key] || keyMap[e.key];

    if (hid) {
      // jesli wykryto modyfikator w mapie, jest to osobny krok (mod)
      if (isModifierKey(hid)) {
        const newSequence = [...sequence, { keycode: hid, modifiers: 0 }]; // modifiers 0 bo to sam klawisz
        onChange(newSequence);
      } else {
        const newSequence = [...sequence, { keycode: hid, modifiers: mods }];
        onChange(newSequence);
        setCurrentMods(0);
      }
    }
  };

  const formatStepString = (kp: KeyPress) => {
    const parts: string[] = [];

    // flagi modyfikatorow
    if (kp.modifiers & MODIFIERS.GUI) parts.push('M');
    if (kp.modifiers & MODIFIERS.CTRL) parts.push('C');
    if (kp.modifiers & MODIFIERS.SHIFT) parts.push('S');
    if (kp.modifiers & MODIFIERS.ALT) parts.push('A');

    const keyNames: Record<number, string> = {
      0: '', // pusty (np dla manual add samego moda)
      224: 'C', 225: 'S', 226: 'A', 227: 'M',
      228: 'RC', 229: 'RS', 230: 'RA', 231: 'RM',
      4: 'a', 5: 'b', 6: 'c', 7: 'd', 8: 'e', 9: 'f', 10: 'g', 11: 'h',
      12: 'i', 13: 'j', 14: 'k', 15: 'l', 16: 'm', 17: 'n', 18: 'o',
      19: 'p', 20: 'q', 21: 'r', 22: 's', 23: 't', 24: 'u', 25: 'v',
      26: 'w', 27: 'x', 28: 'y', 29: 'z',
      30: '1', 31: '2', 32: '3', 33: '4', 34: '5',
      35: '6', 36: '7', 37: '8', 38: '9', 39: '0',
      40: 'Enter', 41: 'Esc', 42: 'BS', 43: 'Tab', 44: 'Space',
      58: 'F1', 59: 'F2', 60: 'F3', 61: 'F4', 62: 'F5', 63: 'F6',
      64: 'F7', 65: 'F8', 66: 'F9', 67: 'F10', 68: 'F11', 69: 'F12',
      79: 'Right', 80: 'Left', 81: 'Down', 82: 'Up',
    };

    const keyName = keyNames[kp.keycode] || (kp.keycode > 0 ? `#${kp.keycode}` : '');

    if (keyName) parts.push(keyName);

    if (parts.length > 0) {
      // < > jesli sa modyfikatory (flags) LUB jesli klawisz to modyfikator (keycode) LUB klawisz wieloliterowy
      if (kp.modifiers !== 0 || isModifierKey(kp.keycode) || keyName.length > 1) {
        return `<${parts.join('-')}>`;
      }
      return keyName;
    }
    return '<Empty>';
  };

  // sklejanie wiszacych modyfikatorow z nastepnym klawiszem
  const getSequencePreview = (seq: KeyPress[]) => {
    if (seq.length === 0) return 'Empty Sequence';

    const parts: string[] = [];
    let pendingMods = 0;

    for (let i = 0; i < seq.length; i++) {
      const step = seq[i];

      // jesli krok to modyfikator (keycode 0 lub 224-231) -> dodaj do pendingMods
      if (step.keycode === 0 || isModifierKey(step.keycode)) {
        let mods = step.modifiers;
        if (step.keycode !== 0) {
          mods |= getModifierFlag(step.keycode);
        }
        pendingMods |= mods;
      } else {
        // zwykly klawisz -> polacz z pendingMods i wyswietl
        const combinedStep: KeyPress = {
          keycode: step.keycode,
          modifiers: step.modifiers | pendingMods
        };
        parts.push(formatStepString(combinedStep));
        pendingMods = 0; // reset po zuzyciu
      }
    }

    // wiszace modyfikatory (np <M> na koncu sekwencji)
    if (pendingMods !== 0) {
      parts.push(formatStepString({ keycode: 0, modifiers: pendingMods }));
    }

    return parts.join('+');
  };

  const removeStep = (index: number) => {
    const newSequence = sequence.filter((_, i) => i !== index);
    onChange(newSequence);
  };

  const addManualStep = () => {
    if (sequence.length >= 3) return;
    // virtual mod step
    onChange([...sequence, { keycode: 0, modifiers: currentMods }]);
  };

  const updateStepKeycode = (index: number, keycode: number) => {
    const newSequence = [...sequence];
    newSequence[index] = { ...newSequence[index], keycode };
    onChange(newSequence);
  };

  return (
    <div className="space-y-6">
      <div className="space-y-2">
        <Label>Record Sequence</Label>
        <div className="flex gap-2">
          <Button
            type="button"
            variant={recording ? "destructive" : "outline"}
            onClick={() => setRecording(!recording)}
            className="flex-1"
          >
            {recording ? 'Recording... (Press keys)' : 'Start Recording'}
          </Button>
          {recording && (
            <Button
              type="button"
              variant="secondary"
              onClick={() => setRecording(false)}
            >
              Stop
            </Button>
          )}
        </div>
        {recording && (
          <Input
            autoFocus
            onKeyDown={handleKeyDown}
            onBlur={() => setRecording(false)}
            placeholder="Type keys..."
            className="font-mono bg-yellow-50/10 border-yellow-500/50"
            readOnly
          />
        )}
      </div>

      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <Label className="text-xs text-muted-foreground">Steps ({sequence.length}/3)</Label>
          <Button
            type="button"
            variant="ghost"
            size="sm"
            onClick={() => onChange([])}
            disabled={sequence.length === 0}
            className="h-6 text-xs"
          >
            Clear
          </Button>
        </div>

        {sequence.length === 0 ? (
          <div className="text-center py-4 text-sm text-muted-foreground border rounded-md border-dashed">
            No steps recorded
          </div>
        ) : (
          <div className="space-y-2">
            {sequence.map((step, index) => (
              <div key={index} className="flex items-center gap-2 bg-muted/30 p-2 rounded-md border">
                <Badge variant="outline" className="w-6 h-6 flex items-center justify-center p-0 shrink-0 bg-background">
                  {index + 1}
                </Badge>

                <div className="flex-1 font-mono text-sm font-bold text-primary">
                  {formatStepString(step)}
                </div>

                <Input
                  type="number"
                  value={step.keycode}
                  onChange={(e) => updateStepKeycode(index, parseInt(e.target.value) || 0)}
                  className="w-12 h-7 text-[10px] font-mono text-right"
                  min={0}
                  max={255}
                />

                <Button
                  type="button"
                  variant="ghost"
                  size="icon"
                  className="h-7 w-7 text-muted-foreground hover:text-destructive"
                  onClick={() => removeStep(index)}
                >
                  <X className="h-3 w-3" />
                </Button>
              </div>
            ))}
          </div>
        )}
      </div>

      <div className="space-y-2">
        <Label className="text-xs">Manual Add</Label>
        <div className="flex gap-2">
          <div className="flex-1 flex flex-wrap gap-1">
            {['Ctrl', 'Shift', 'Alt', 'Meta'].map(mod => {
              const modFlag = mod === 'Ctrl' ? MODIFIERS.CTRL :
                mod === 'Shift' ? MODIFIERS.SHIFT :
                  mod === 'Alt' ? MODIFIERS.ALT : MODIFIERS.GUI;
              const isActive = (currentMods & modFlag) !== 0;
              return (
                <Badge
                  key={mod}
                  variant={isActive ? "default" : "outline"}
                  className={`cursor-pointer select-none ${sequence.length >= 3 ? 'opacity-50 pointer-events-none' : ''}`}
                  onClick={() => {
                    if (sequence.length >= 3) return;
                    setCurrentMods(currentMods ^ modFlag)
                  }}
                >
                  {mod}
                </Badge>
              );
            })}
          </div>
          <Button
            type="button"
            variant="secondary"
            size="sm"
            disabled={sequence.length >= 3}
            onClick={addManualStep}
          >
            <Plus className="h-3 w-3 mr-1" />
            Step
          </Button>
        </div>
      </div>

      <Card className="bg-primary/5 border-primary/20">
        <CardContent className="p-3">
          <Label className="text-xs text-muted-foreground mb-1 block">Full Sequence Preview</Label>
          <div className="font-mono text-lg font-bold text-center break-all text-primary">
            {getSequencePreview(sequence)}
          </div>
        </CardContent>
      </Card>
    </div>
  );
}
