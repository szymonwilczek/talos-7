import { KeyPress } from "@/lib/types/config.types";
import { MAX_SCRIPT_SIZE, ScriptPlatform, ScriptPlatformLabels } from "@/lib/types/macro.types";
import { Label } from "@/components/ui/label";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { KeySequenceInput } from "@/components/macro-dialog/key-sequence-input";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Textarea } from "@/components/ui/textarea";
import { Input } from "@/components/ui/input";

interface MacroFormScriptProps {
  content: string;
  platform: ScriptPlatform;
  terminalShortcut: KeyPress[];
  onContentChange: (v: string) => void;
  onPlatformChange: (v: ScriptPlatform) => void;
  onShortcutChange: (seq: KeyPress[]) => void;
  onFileUpload: (e: React.ChangeEvent<HTMLInputElement>) => void;
  file: File | null;
}

export function MacroFormScript({
  content, platform, terminalShortcut, file,
  onContentChange, onPlatformChange, onShortcutChange, onFileUpload
}: MacroFormScriptProps) {
  return (
    <div className="space-y-4">
      <div className="space-y-2">
        <Label htmlFor="script-platform">Target Platform</Label>
        <Select
          value={platform.toString()}
          onValueChange={(v) => onPlatformChange(parseInt(v) as ScriptPlatform)}
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

      {platform === ScriptPlatform.LINUX && (
        <div className="space-y-2">
          <Label>Terminal Shortcut</Label>
          <div className="p-2 border rounded-md bg-muted/20">
            <KeySequenceInput
              sequence={terminalShortcut}
              onChange={onShortcutChange}
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
            value={content}
            onChange={(e) => onContentChange(e.target.value.slice(0, MAX_SCRIPT_SIZE))}
            rows={12}
            className="font-mono text-xs"
            placeholder="#!/bin/bash&#10;echo 'Hello World'"
          />
          <p className="text-xs text-muted-foreground">
            {content.length} / {MAX_SCRIPT_SIZE} bytes
          </p>
        </TabsContent>

        <TabsContent value="upload" className="space-y-2">
          <Label htmlFor="script-file">Upload Script File</Label>
          <Input
            id="script-file"
            type="file"
            accept=".sh,.bash,.bat,.ps1,.zsh"
            onChange={onFileUpload}
          />
          {file && (
            <p className="text-sm text-muted-foreground">
              Loaded: {file.name} ({content.length} bytes)
            </p>
          )}
        </TabsContent>
      </Tabs>
    </div>
  );
}
