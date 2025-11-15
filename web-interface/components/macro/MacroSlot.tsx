'use client';

import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select';
import { MacroEntry, MacroType, MacroTypeLabels } from '@/lib/types/macro.types';

interface MacroSlotProps {
  buttonIndex: number;
  macro: MacroEntry;
  onChange: (macro: MacroEntry) => void;
}

export function MacroSlot({ buttonIndex, macro, onChange }: MacroSlotProps) {
  return (
    <Card>
      <CardHeader>
        <CardTitle className="text-lg">Button {buttonIndex + 1}</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Nazwa makra */}
        <div className="space-y-2">
          <Label htmlFor={`name-${buttonIndex}`}>Name</Label>
          <Input
            id={`name-${buttonIndex}`}
            value={macro.name}
            onChange={(e) => onChange({ ...macro, name: e.target.value })}
            placeholder="Enter macro name"
            maxLength={16}
          />
        </div>

        {/* Typ makra */}
        <div className="space-y-2">
          <Label htmlFor={`type-${buttonIndex}`}>Type</Label>
          <Select
            value={macro.type.toString()}
            onValueChange={(value) =>
              onChange({ ...macro, type: parseInt(value) as MacroType })
            }
          >
            <SelectTrigger id={`type-${buttonIndex}`}>
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {Object.entries(MacroTypeLabels).map(([value, label]) => (
                <SelectItem key={value} value={value}>
                  {label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        {/* Wartość dynamiczna w zależności od typu */}
        {macro.type === MacroType.KEY_PRESS && (
          <div className="space-y-2">
            <Label htmlFor={`value-${buttonIndex}`}>HID Keycode</Label>
            <Input
              id={`value-${buttonIndex}`}
              type="number"
              value={macro.value}
              onChange={(e) =>
                onChange({ ...macro, value: parseInt(e.target.value) || 0 })
              }
              placeholder="e.g., 4 for 'A'"
              min={0}
              max={255}
            />
            <p className="text-xs text-slate-500">
              Enter HID keycode (e.g., 4='A', 58='F1')
            </p>
          </div>
        )}

        {macro.type === MacroType.MACRO_STRING && (
          <div className="space-y-2">
            <Label htmlFor={`string-${buttonIndex}`}>Text String</Label>
            <Input
              id={`string-${buttonIndex}`}
              value={macro.macroString}
              onChange={(e) =>
                onChange({
                  ...macro,
                  macroString: e.target.value,
                  value: e.target.value.length,
                })
              }
              placeholder="Enter text to type"
              maxLength={32}
            />
          </div>
        )}

        {macro.type === MacroType.LAYER_TOGGLE && (
          <div className="space-y-2">
            <Label htmlFor={`layer-${buttonIndex}`}>Target Layer</Label>
            <Select
              value={macro.value.toString()}
              onValueChange={(value) =>
                onChange({ ...macro, value: parseInt(value) })
              }
            >
              <SelectTrigger id={`layer-${buttonIndex}`}>
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="0">Layer 1</SelectItem>
                <SelectItem value="1">Layer 2</SelectItem>
                <SelectItem value="2">Layer 3</SelectItem>
                <SelectItem value="3">Layer 4</SelectItem>
              </SelectContent>
            </Select>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
