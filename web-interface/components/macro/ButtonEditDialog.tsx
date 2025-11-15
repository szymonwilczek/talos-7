'use client';

import React, { useState, useEffect } from 'react';
import { MacroEntry, MacroType } from '@/lib/types/config.types';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select';
import { Textarea } from '@/components/ui/textarea';
import { KeycodeDialog } from './KeycodeDialog';

interface ButtonEditDialogProps {
  open: boolean;
  buttonIndex: number | null;
  macro: MacroEntry | null;
  onClose: () => void;
  onSave: (buttonIndex: number, macro: MacroEntry) => void;
}

const SUGGESTED_EMOJIS = ['🎮', '💼', '🏠', '⚙️', '⚡', '📧', '💻', '🎵', '📝', '🔧'];

export function ButtonEditDialog({
  open,
  buttonIndex,
  macro,
  onClose,
  onSave,
}: ButtonEditDialogProps) {
  const [macroName, setMacroName] = useState('');
  const [macroEmoji, setMacroEmoji] = useState('');
  const [macroType, setMacroType] = useState<MacroType>(MacroType.KEY_PRESS);
  const [macroValue, setMacroValue] = useState(0);
  const [macroString, setMacroString] = useState('');

  useEffect(() => {
    if (macro) {
      setMacroName(macro.name);
      setMacroEmoji(macro.emoji);
      setMacroType(macro.type);
      setMacroValue(macro.value);
      setMacroString(macro.macroString);
    }
  }, [macro]);

  const handleSave = () => {
    if (buttonIndex === null) return;
    onSave(buttonIndex, {
      type: macroType,
      value: macroValue,
      macroString: macroString,
      name: macroName,
      emoji: macroEmoji,
    });
    onClose();
  };

  if (!open || buttonIndex === null) return null;

  return (
    <Dialog open={open} onOpenChange={onClose}>
      <DialogContent className="max-w-md">
        <DialogHeader>
          <DialogTitle>Edit Button {buttonIndex + 1}</DialogTitle>
          <DialogDescription>Configure button macro settings</DialogDescription>
        </DialogHeader>

        <div className="space-y-4">
          <div className="space-y-2">
            <Label>Button Emoji</Label>
            <div className="flex gap-2 flex-wrap">
              {SUGGESTED_EMOJIS.map((emoji) => (
                <Button
                  key={emoji}
                  variant={macroEmoji === emoji ? 'default' : 'outline'}
                  onClick={() => setMacroEmoji(emoji)}
                  className="text-xl w-12 h-12 p-0"
                >
                  {emoji}
                </Button>
              ))}
            </div>
          </div>

          <div className="space-y-2">
            <Label htmlFor="button-name">Button Name</Label>
            <Input
              id="button-name"
              value={macroName}
              onChange={(e) => setMacroName(e.target.value.slice(0, 16))}
              maxLength={16}
              placeholder="Enter button name"
            />
          </div>

          <div className="space-y-2">
            <Label htmlFor="macro-type">Macro Type</Label>
            <Select
              value={macroType.toString()}
              onValueChange={(v) => setMacroType(parseInt(v) as MacroType)}
            >
              <SelectTrigger id="macro-type">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="0">Key Press</SelectItem>
                <SelectItem value="1">Text String</SelectItem>
                <SelectItem value="2">Layer Toggle</SelectItem>
              </SelectContent>
            </Select>
          </div>

          {macroType === MacroType.KEY_PRESS && (
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <Label htmlFor="keycode">HID Keycode</Label>
                <KeycodeDialog />
              </div>
              <Input
                id="keycode"
                type="number"
                value={macroValue}
                onChange={(e) => setMacroValue(parseInt(e.target.value) || 0)}
                min={0}
                max={255}
                placeholder="e.g. 4 for 'A'"
              />
            </div>
          )}

          {macroType === MacroType.TEXT_STRING && (
            <div className="space-y-2">
              <Label htmlFor="macro-text">Text to Type</Label>
              <Textarea
                id="macro-text"
                value={macroString}
                onChange={(e) => setMacroString(e.target.value.slice(0, 32))}
                maxLength={32}
                rows={3}
                placeholder="Enter text to type"
              />
            </div>
          )}

          {macroType === MacroType.LAYER_TOGGLE && (
            <div className="space-y-2">
              <Label htmlFor="target-layer">Target Layer</Label>
              <Select
                value={macroValue.toString()}
                onValueChange={(v) => setMacroValue(parseInt(v))}
              >
                <SelectTrigger id="target-layer">
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

          <div className="flex gap-2 pt-4">
            <Button variant="outline" onClick={onClose} className="flex-1">
              Cancel
            </Button>
            <Button onClick={handleSave} className="flex-1">
              Save Changes
            </Button>
          </div>
        </div>
      </DialogContent>
    </Dialog>
  );
}
