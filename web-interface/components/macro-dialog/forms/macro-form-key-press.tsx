import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { KeycodeDialog } from "@/components/macro-dialog/keycode-dialog";

interface MacroFormKeyPressProps {
  value: number;
  repeatCount: number;
  repeatInterval: number;
  onValueChange: (val: number) => void;
  onRepeatCountChange: (val: number) => void;
  onRepeatIntervalChange: (val: number) => void;
}

export function MacroFormKeyPress({
  value, repeatCount, repeatInterval,
  onValueChange, onRepeatCountChange, onRepeatIntervalChange
}: MacroFormKeyPressProps) {
  return (
    <div className="space-y-4">
      <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
        <div className="space-y-2">
          <Label>Repetitions</Label>
          <Input
            type="number" min={1} max={10000} value={repeatCount}
            onChange={e => onRepeatCountChange(parseInt(e.target.value))}
          />
          <p className="text-[10px] text-muted-foreground">Count</p>
        </div>
        <div className="space-y-2">
          <Label>Interval (ms)</Label>
          <Input
            type="number" min={0} max={5000} value={repeatInterval}
            onChange={e => onRepeatIntervalChange(parseInt(e.target.value))}
          />
          <p className="text-[10px] text-muted-foreground">Delay</p>
        </div>
      </div>
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <Label htmlFor="keycode">HID Keycode</Label>
          <KeycodeDialog />
        </div>
        <Input
          id="keycode" type="number" value={value}
          onChange={(e) => onValueChange(parseInt(e.target.value) || 0)}
          min={0} max={255} placeholder="e.g. 4 for 'A'"
        />
      </div>
    </div>
  );
}
