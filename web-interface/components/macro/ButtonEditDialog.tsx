'use client';

import { useState, useEffect } from 'react';
import { KeyPress, MacroEntry, MacroType } from '@/lib/types/config.types';
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
import { MAX_SCRIPT_SIZE, ScriptPlatform, ScriptPlatformLabels } from '@/lib/types/macro.types';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '../ui/tabs';
import { KeySequenceInput } from './KeySequenceInput';

interface ButtonEditDialogProps {
  open: boolean;
  buttonIndex: number | null;
  macro: MacroEntry | null;
  layerMacros: MacroEntry[]; // lista makr do walidacji
  onClose: () => void;
  onSave: (buttonIndex: number, macro: MacroEntry) => void;
}

const SUGGESTED_EMOJIS = ['🎮', '💼', '🏠', '🔧', '⚡', '📧', '💻', '🎵', '📝', '☕', '🗡️', '❤️', '🔔', '🧪', '🔒', '☂️', '🦕', '👻', '🔫', '⏳', '🌷'];

export function ButtonEditDialog({
  open,
  buttonIndex,
  macro,
  layerMacros,
  onClose,
  onSave,
}: ButtonEditDialogProps) {
  const [macroName, setMacroName] = useState('');
  const [macroEmoji, setMacroEmoji] = useState('');
  const [macroType, setMacroType] = useState<MacroType>(MacroType.KEY_PRESS);
  const [macroValue, setMacroValue] = useState(0);
  const [macroString, setMacroString] = useState('');
  const [scriptContent, setScriptContent] = useState('');
  const [scriptPlatform, setScriptPlatform] = useState<ScriptPlatform>(ScriptPlatform.LINUX);
  const [scriptFile, setScriptFile] = useState<File | null>(null);
  const [keySequence, setKeySequence] = useState<KeyPress[]>([]);
  const [terminalShortcut, setTerminalShortcut] = useState<KeyPress[]>([]);
  const [macroRepeatCount, setMacroRepeatCount] = useState(1);
  const [macroRepeatInterval, setMacroRepeatInterval] = useState(0);
  const [moveX, setMoveX] = useState(0);
  const [moveY, setMoveY] = useState(0);
  const [midiNote, setMidiNote] = useState(60);
  const [midiVelocity, setMidiVelocity] = useState(127);
  const [midiChannel, setMidiChannel] = useState(1);
  const [midiCCNumber, setMidiCCNumber] = useState(1);
  const [midiCCValue, setMidiCCValue] = useState(127);

  const canSelectLayerToggle = !layerMacros?.some(
    (m, idx) => m.type === MacroType.LAYER_TOGGLE && idx !== buttonIndex
  );

  useEffect(() => {
    if (macro) {
      setMacroName(macro.name);
      setMacroEmoji(macro.emoji);
      setMacroType(macro.type);
      setMacroValue(macro.value);
      setMacroString(macro.macroString);
      setKeySequence(macro.keySequence ? decompileSequence(macro.keySequence) : []);
      setTerminalShortcut(macro.terminalShortcut || []);
      setMacroRepeatCount(macro.repeatCount || 1);
      setMacroRepeatInterval(macro.repeatInterval || 0);
      setMoveX(macro.moveX || 0);
      setMoveY(macro.moveY || 0);

      if (macro.type === MacroType.SCRIPT) {
        setScriptContent(macro.script || '');
        setScriptPlatform(macro.scriptPlatform || ScriptPlatform.LINUX);
      }

      if (macro.type === MacroType.MIDI_NOTE) {
        setMidiNote(macro.value || 60);
        setMidiVelocity(macro.moveX || 127);
        setMidiChannel(macro.moveY || 1);
      } else {
        // default 
        setMidiNote(60);
        setMidiVelocity(127);
        setMidiChannel(1);
      }

      if (macro.type === MacroType.MIDI_CC) {
        setMidiCCNumber(macro.value || 1);
        setMidiCCValue(macro.moveX || 127);
        setMidiChannel(macro.moveY || 1);
      }
    }
  }, [macro]);

  // useEffect(() => {
  //   if (macro && macro.type === MacroType.SCRIPT) {
  //     setScriptContent(macro.script || '');
  //     setScriptPlatform(macro.scriptPlatform || ScriptPlatform.LINUX);
  //   }
  // }, [macro]);

  const decompileSequence = (compiled: KeyPress[]): KeyPress[] => {
    const raw: KeyPress[] = [];
    compiled.forEach(step => {
      if (step.modifiers & 1) raw.push({ keycode: 224, modifiers: 0 }); // LCtrl
      if (step.modifiers & 2) raw.push({ keycode: 225, modifiers: 0 }); // LShift
      if (step.modifiers & 4) raw.push({ keycode: 226, modifiers: 0 }); // LAlt
      if (step.modifiers & 8) raw.push({ keycode: 227, modifiers: 0 }); // LGUI
      if (step.modifiers & 16) raw.push({ keycode: 228, modifiers: 0 }); // RCtrl
      if (step.modifiers & 32) raw.push({ keycode: 229, modifiers: 0 }); // RShift
      if (step.modifiers & 64) raw.push({ keycode: 230, modifiers: 0 }); // RAlt
      if (step.modifiers & 128) raw.push({ keycode: 231, modifiers: 0 }); // RGUI

      if (step.keycode !== 0) {
        raw.push({ keycode: step.keycode, modifiers: 0 });
      }
    });
    return raw;
  };

  const handleSave = () => {
    if (buttonIndex === null) return;

    const savedMacro: MacroEntry = {
      type: macroType,
      macroString: macroString,
      name: macroName,
      emoji: macroEmoji,
      keySequence: macroType === MacroType.KEY_SEQUENCE ? keySequence : undefined,
      repeatCount: macroRepeatCount,
      repeatInterval: macroRepeatInterval,
      value:
        macroType === MacroType.MIDI_NOTE ? midiNote :
          macroType === MacroType.MIDI_CC ? midiCCNumber :
            macroValue,
      moveX:
        macroType === MacroType.MIDI_NOTE ? midiVelocity :
          macroType === MacroType.MIDI_CC ? midiCCValue :
            moveX,
      moveY: (macroType === MacroType.MIDI_NOTE || macroType === MacroType.MIDI_CC) ? midiChannel : moveY,
    };

    if (macroType === MacroType.SCRIPT) {
      savedMacro.script = scriptContent;
      savedMacro.scriptPlatform = scriptPlatform;
      savedMacro.terminalShortcut = terminalShortcut;
    }

    onSave(buttonIndex, savedMacro);
    onClose();
  };

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (event) => {
        const content = event.target?.result as string;
        if (content.length > MAX_SCRIPT_SIZE) {
          alert(`Script too large! Max ${MAX_SCRIPT_SIZE} bytes`);
          return;
        }
        setScriptContent(content);
        setScriptFile(file);
      };
      reader.readAsText(file);
    }
  };

  if (!open || buttonIndex === null) return null;

  return (
    <Dialog open={open} onOpenChange={onClose}>
      <DialogContent className="max-w-md max-h-[90vh] overflow-y-auto">
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
              maxLength={15}
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
                {canSelectLayerToggle && (
                  <SelectItem value="2">Layer Toggle</SelectItem>
                )}
                <SelectItem value="3">Script Execution</SelectItem>
                <SelectItem value="4">Key Sequence</SelectItem>
                <SelectItem value="5">Mouse Button</SelectItem>
                <SelectItem value="6">Mouse Move</SelectItem>
                <SelectItem value="7">Mouse Scroll</SelectItem>
                <SelectItem value="8">MIDI Note</SelectItem>
                <SelectItem value="9">MIDI Control Change (CC)</SelectItem>
              </SelectContent>
            </Select>
          </div>

          {macroType === MacroType.MIDI_NOTE && (
            <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
              <div className="col-span-2 space-y-2">
                <div className="flex justify-between">
                  <Label>Note (0-127)</Label>
                  <span className="text-xs text-muted-foreground">60 = C4 (Middle C)</span>
                </div>
                <Input
                  type="number"
                  min={0} max={127}
                  value={midiNote}
                  onChange={e => setMidiNote(parseInt(e.target.value))}
                />
              </div>
              <div className="space-y-2">
                <Label>Velocity</Label>
                <Input
                  type="number"
                  min={0} max={127}
                  value={midiVelocity}
                  onChange={e => setMidiVelocity(parseInt(e.target.value))}
                />
              </div>
              <div className="space-y-2">
                <Label>Channel (1-16)</Label>
                <Input
                  type="number"
                  min={1} max={16}
                  value={midiChannel}
                  onChange={e => setMidiChannel(parseInt(e.target.value))}
                />
              </div>
            </div>
          )}

          {macroType === MacroType.MIDI_CC && (
            <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
              <div className="col-span-2 space-y-2">
                <Label>Controller Number (CC#)</Label>
                <Input
                  type="number"
                  min={0} max={119}
                  value={midiCCNumber}
                  onChange={e => setMidiCCNumber(parseInt(e.target.value))}
                  placeholder="e.g. 1 (Mod Wheel), 7 (Volume)"
                />
                <p className="text-[10px] text-muted-foreground">Standard: 1=Mod, 7=Vol, 10=Pan, 11=Expr</p>
              </div>
              <div className="space-y-2">
                <Label>Value (0-127)</Label>
                <Input
                  type="number"
                  min={0} max={127}
                  value={midiCCValue}
                  onChange={e => setMidiCCValue(parseInt(e.target.value))}
                />
              </div>
              <div className="space-y-2">
                <Label>Channel (1-16)</Label>
                <Input
                  type="number"
                  min={1} max={16}
                  value={midiChannel}
                  onChange={e => setMidiChannel(parseInt(e.target.value))}
                />
              </div>
            </div>
          )}

          {(macroType === MacroType.KEY_PRESS || macroType === MacroType.MOUSE_BUTTON) && (
            <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
              <div className="space-y-2">
                <Label>Repetitions</Label>
                <Input
                  type="number"
                  min={1} max={10000}
                  value={macroRepeatCount}
                  onChange={e => setMacroRepeatCount(parseInt(e.target.value))}
                />
                <p className="text-[10px] text-muted-foreground">Count</p>
              </div>
              <div className="space-y-2">
                <Label>Interval (ms)</Label>
                <Input
                  type="number"
                  min={0} max={5000}
                  value={macroRepeatInterval}
                  onChange={e => setMacroRepeatInterval(parseInt(e.target.value))}
                />
                <p className="text-[10px] text-muted-foreground">Delay</p>
              </div>
            </div>
          )}

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

          {macroType === MacroType.MOUSE_BUTTON && (
            <div className="space-y-2">
              <Label>Mouse Button</Label>
              <Select onValueChange={v => setMacroValue(parseInt(v))} value={macroValue.toString()}>
                <SelectTrigger><SelectValue /></SelectTrigger>
                <SelectContent>
                  <SelectItem value="1">Left Click</SelectItem>
                  <SelectItem value="2">Right Click</SelectItem>
                  <SelectItem value="4">Middle Click</SelectItem>
                </SelectContent>
              </Select>
            </div>
          )}

          {macroType === MacroType.MOUSE_MOVE && (
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label>Move X</Label>
                <Input type="number" value={moveX} onChange={e => setMoveX(parseInt(e.target.value))} />
              </div>
              <div className="space-y-2">
                <Label>Move Y</Label>
                <Input type="number" value={moveY} onChange={e => setMoveY(parseInt(e.target.value))} />
              </div>
            </div>
          )}

          {macroType === MacroType.MOUSE_WHEEL && (
            <div className="space-y-2">
              <Label>Scroll Amount</Label>
              <Input type="number" value={macroValue} onChange={e => setMacroValue(parseInt(e.target.value))} placeholder="Positive=Up, Negative=Down" />
            </div>
          )}

          {macroType === MacroType.KEY_SEQUENCE && (
            <KeySequenceInput
              sequence={keySequence}
              onChange={setKeySequence}
            />
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

          {macroType === MacroType.SCRIPT && (
            <div className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="script-platform">Target Platform</Label>
                <Select
                  value={scriptPlatform.toString()}
                  onValueChange={(v) => setScriptPlatform(parseInt(v) as ScriptPlatform)}
                >
                  <SelectTrigger id="script-platform">
                    <SelectValue />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="0">{ScriptPlatformLabels[ScriptPlatform.LINUX]}</SelectItem>
                    <SelectItem value="1">{ScriptPlatformLabels[ScriptPlatform.WINDOWS]}</SelectItem>
                    <SelectItem value="2">{ScriptPlatformLabels[ScriptPlatform.MACOS]}</SelectItem>
                  </SelectContent>
                </Select>
              </div>

              {scriptPlatform === ScriptPlatform.LINUX && (
                <div className="space-y-2">
                  <Label>Terminal Shortcut</Label>
                  <div className="p-2 border rounded-md bg-muted/20">
                    <KeySequenceInput
                      sequence={terminalShortcut}
                      onChange={setTerminalShortcut}
                    />
                  </div>
                  <p className="text-[10px] text-muted-foreground mt-1">
                    Record the keyboard shortcut that opens your terminal (e.g. Ctrl+Alt+T).
                  </p>
                </div>
              )}

              <Tabs defaultValue="editor" className="w-full">
                <TabsList className="grid w-full grid-cols-2">
                  <TabsTrigger value="editor">Write Code</TabsTrigger>
                  <TabsTrigger value="upload">Upload File</TabsTrigger>
                </TabsList>

                <TabsContent value="editor" className="space-y-2">
                  <Label htmlFor="script-editor">Script Content</Label>
                  <Textarea
                    id="script-editor"
                    value={scriptContent}
                    onChange={(e) => setScriptContent(e.target.value.slice(0, MAX_SCRIPT_SIZE))}
                    rows={12}
                    className="font-mono text-xs"
                    placeholder="#!/bin/bash&#10;echo 'Hello World'"
                  />
                  <p className="text-xs text-muted-foreground">
                    {scriptContent.length} / {MAX_SCRIPT_SIZE} bytes
                  </p>
                </TabsContent>

                <TabsContent value="upload" className="space-y-2">
                  <Label htmlFor="script-file">Upload Script File</Label>
                  <Input
                    id="script-file"
                    type="file"
                    accept=".sh,.bash,.bat,.ps1,.zsh"
                    onChange={handleFileUpload}
                  />
                  {scriptFile && (
                    <p className="text-sm text-muted-foreground">
                      Loaded: {scriptFile.name} ({scriptContent.length} bytes)
                    </p>
                  )}
                </TabsContent>
              </Tabs>
            </div>
          )}

          {macroType === MacroType.LAYER_TOGGLE && (
            <div className="space-y-2">
              <Label>Layer Cycling</Label>
              <p className="text-sm text-muted-foreground">
                This button will cycle through layers: 1 → 2 → 3 → 4 → 1
              </p>
              <div className="flex items-center gap-2 p-3 rounded-md bg-muted">
                <span className="text-2xl">🔄</span>
                <span className="text-sm">Automatic layer rotation enabled</span>
              </div>
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
