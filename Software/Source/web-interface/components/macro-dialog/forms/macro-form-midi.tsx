import { Input } from "../../ui/input";
import { Label } from "../../ui/label";

interface MacroFormMidiProps {
  type: 'NOTE' | 'CC';
  note: number;
  velocity: number;
  channel: number;
  ccNumber: number;
  ccValue: number;
  onNoteChange: (v: number) => void;
  onVelocityChange: (v: number) => void;
  onChannelChange: (v: number) => void;
  onCCNumberChange: (v: number) => void;
  onCCValueChange: (v: number) => void;
}

export function MacroFormMidi({
  type, note, velocity, channel, ccNumber, ccValue,
  onNoteChange, onVelocityChange, onChannelChange, onCCNumberChange, onCCValueChange
}: MacroFormMidiProps) {
  return (
    <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
      <div className="col-span-2 space-y-2">
        {type === 'NOTE' ? (
          <>
            <div className="flex justify-between">
              <Label>Note (0-127)</Label>
              <span className="text-xs text-muted-foreground">60 = C4 (Middle C)</span>
            </div>
            <Input
              type="number" min={0} max={127} value={note}
              onChange={e => onNoteChange(parseInt(e.target.value))}
            />
          </>
        ) : (
          <>
            <Label>Controller Number (CC#)</Label>
            <Input
              type="number" min={0} max={119} value={ccNumber}
              onChange={e => onCCNumberChange(parseInt(e.target.value))}
              placeholder="e.g. 1 (Mod Wheel), 7 (Volume)"
            />
            <p className="text-[10px] text-muted-foreground">Standard: 1=Mod, 7=Vol, 10=Pan, 11=Expr</p>
          </>
        )}
      </div>

      <div className="space-y-2">
        <Label>{type === 'NOTE' ? 'Velocity' : 'Value'} (0-127)</Label>
        <Input
          type="number" min={0} max={127}
          value={type === 'NOTE' ? velocity : ccValue}
          onChange={e => type === 'NOTE' ? onVelocityChange(parseInt(e.target.value)) : onCCValueChange(parseInt(e.target.value))}
        />
      </div>

      <div className="space-y-2">
        <Label>Channel (1-16)</Label>
        <Input
          type="number" min={1} max={16} value={channel}
          onChange={e => onChannelChange(parseInt(e.target.value))}
        />
      </div>
    </div>
  );
}
