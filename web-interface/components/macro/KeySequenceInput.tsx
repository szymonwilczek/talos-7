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

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (!recording) return;

    e.preventDefault();
    e.stopPropagation();

    if (sequence.length >= 3) {
      console.log('Maximum sequence length reached (3 keys)');
      setRecording(false);
      return;
    }

    // aktualizacja modyfikatorow na podstawie stanu klawiszy
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

    // ignorowanie samych modyfikatorow
    if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) {
      return;
    }

    const keyMap: Record<string, number> = {
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
      const newSequence = [...sequence, { keycode: hid, modifiers: mods }];
      onChange(newSequence);
      console.log('Added key:', { keycode: hid, modifiers: mods, key: e.key });
      setCurrentMods(0);
    }
  };

  const formatKeyPress = (kp: KeyPress) => {
    const parts: string[] = [];
    if (kp.modifiers & MODIFIERS.CTRL) parts.push('Ctrl');
    if (kp.modifiers & MODIFIERS.SHIFT) parts.push('Shift');
    if (kp.modifiers & MODIFIERS.ALT) parts.push('Alt');
    if (kp.modifiers & MODIFIERS.GUI) parts.push('⊞');

    const keyNames: Record<number, string> = {
      4: 'A', 5: 'B', 6: 'C', 7: 'D', 8: 'E', 9: 'F', 10: 'G', 11: 'H',
      12: 'I', 13: 'J', 14: 'K', 15: 'L', 16: 'M', 17: 'N', 18: 'O',
      19: 'P', 20: 'Q', 21: 'R', 22: 'S', 23: 'T', 24: 'U', 25: 'V',
      26: 'W', 27: 'X', 28: 'Y', 29: 'Z',
      30: '1', 31: '2', 32: '3', 33: '4', 34: '5',
      35: '6', 36: '7', 37: '8', 38: '9', 39: '0',
      40: '↵', 41: 'Esc', 42: '⌫', 43: '⇥', 44: '␣',
      58: 'F1', 59: 'F2', 60: 'F3', 61: 'F4', 62: 'F5', 63: 'F6',
      64: 'F7', 65: 'F8', 66: 'F9', 67: 'F10', 68: 'F11', 69: 'F12',
      79: '→', 80: '←', 81: '↓', 82: '↑',
    };

    parts.push(keyNames[kp.keycode] || `#${kp.keycode}`);

    return parts.join('+');
  };

  const removeStep = (index: number) => {
    const newSequence = sequence.filter((_, i) => i !== index);
    onChange(newSequence);
  };

  const addManualStep = () => {
    onChange([...sequence, { keycode: 0, modifiers: currentMods }]);
  };

  const updateStep = (index: number, keycode: number, modifiers: number) => {
    const newSequence = [...sequence];
    newSequence[index] = { keycode, modifiers };
    onChange(newSequence);
  };

  return (
    <div className="space-y-4">
      <div className="space-y-2">
        <Label>Record Key Sequence</Label>
        <div className="flex gap-2">
          <Button
            type="button"
            variant={recording ? "destructive" : "outline"}
            onClick={() => setRecording(!recording)}
            className="flex-1"
          >
            {recording ? 'Recording... (press keys)' : 'Start Recording'}
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
            placeholder="Press keys in sequence (e.g., Ctrl+A, then X)..."
            className="font-mono bg-yellow-50 dark:bg-yellow-950 border-yellow-300"
            readOnly
          />
        )}
        <p className="text-xs text-muted-foreground">
          Press each key combination one after another. Example: Press Ctrl+A, then press X, then press Enter.
        </p>
      </div>

      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <Label>Key Sequence Steps ({sequence.length}/3)</Label>
          <Button
            type="button"
            variant="ghost"
            size="sm"
            onClick={() => onChange([])}
            disabled={sequence.length === 0}
          >
            Clear All
          </Button>
        </div>

        {sequence.length === 0 ? (
          <Card>
            <CardContent className="py-8 text-center text-muted-foreground">
              No keys recorded. Click "Start Recording" to begin.
            </CardContent>
          </Card>
        ) : (
          <div className="space-y-2">
            {sequence.map((step, index) => (
              <Card key={index}>
                <CardContent className="p-3">
                  <div className="flex items-center gap-3">
                    <Badge variant="secondary" className="font-mono w-8">
                      {index + 1}
                    </Badge>
                    <div className="flex-1 font-mono text-sm bg-muted px-3 py-2 rounded">
                      {formatKeyPress(step)}
                    </div>
                    <div className="flex items-center gap-1">
                      <Input
                        type="number"
                        value={step.keycode}
                        onChange={(e) => updateStep(index, parseInt(e.target.value) || 0, step.modifiers)}
                        className="w-16 h-8 text-xs font-mono"
                        min={0}
                        max={255}
                      />
                      <Button
                        type="button"
                        variant="ghost"
                        size="sm"
                        onClick={() => removeStep(index)}
                      >
                        <X className="h-4 w-4" />
                      </Button>
                    </div>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        )}
      </div>

      <div className="space-y-2">
        <Label>Add Step Manually</Label>
        <div className="flex gap-2">
          <div className="flex-1 flex flex-wrap gap-2">
            <Badge
              variant={currentMods & MODIFIERS.CTRL ? "default" : "outline"}
              className="cursor-pointer"
              onClick={() => setCurrentMods(currentMods ^ MODIFIERS.CTRL)}
            >
              Ctrl
            </Badge>
            <Badge
              variant={currentMods & MODIFIERS.SHIFT ? "default" : "outline"}
              className="cursor-pointer"
              onClick={() => setCurrentMods(currentMods ^ MODIFIERS.SHIFT)}
            >
              Shift
            </Badge>
            <Badge
              variant={currentMods & MODIFIERS.ALT ? "default" : "outline"}
              className="cursor-pointer"
              onClick={() => setCurrentMods(currentMods ^ MODIFIERS.ALT)}
            >
              Alt
            </Badge>
            <Badge
              variant={currentMods & MODIFIERS.GUI ? "default" : "outline"}
              className="cursor-pointer"
              onClick={() => setCurrentMods(currentMods ^ MODIFIERS.GUI)}
            >
              Win
            </Badge>
          </div>
          <Button
            type="button"
            variant="outline"
            size="sm"
            onClick={addManualStep}
          >
            <Plus className="h-4 w-4 mr-1" />
            Add
          </Button>
        </div>
      </div>
    </div>
  );
}
