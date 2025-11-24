import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";

interface MacroFormMouseProps {
  type: 'BUTTON' | 'MOVE' | 'WHEEL';
  value: number;
  moveX: number;
  moveY: number;
  repeatCount?: number;
  repeatInterval?: number;
  onValueChange: (val: number) => void;
  onMoveChange: (x: number, y: number) => void;
  onRepeatCountChange?: (val: number) => void;
  onRepeatIntervalChange?: (val: number) => void;
}

export function MacroFormMouse({
  type, value, moveX, moveY, repeatCount, repeatInterval,
  onValueChange, onMoveChange, onRepeatCountChange, onRepeatIntervalChange
}: MacroFormMouseProps) {
  return (
    <div className="space-y-4">
      {type === 'BUTTON' && (
        <>
          <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
            <div className="space-y-2">
              <Label>Repetitions</Label>
              <Input
                type="number" min={1} max={10000} value={repeatCount}
                onChange={e => onRepeatCountChange?.(parseInt(e.target.value))}
              />
            </div>
            <div className="space-y-2">
              <Label>Interval (ms)</Label>
              <Input
                type="number" min={0} max={5000} value={repeatInterval}
                onChange={e => onRepeatIntervalChange?.(parseInt(e.target.value))}
              />
            </div>
          </div>
          <div className="space-y-2">
            <Label>Mouse Button</Label>
            <Select onValueChange={v => onValueChange(parseInt(v))} value={value.toString()}>
              <SelectTrigger>
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="1">Left Click</SelectItem>
                <SelectItem value="2">Right Click</SelectItem>
                <SelectItem value="4">Middle Click</SelectItem>
              </SelectContent>
            </Select>
          </div>
        </>
      )}

      {type === 'MOVE' && (
        <div className="grid grid-cols-2 gap-4">
          <div className="space-y-2">
            <Label>Move X</Label>
            <Input type="number" value={moveX} onChange={e => onMoveChange(parseInt(e.target.value), moveY)} />
          </div>
          <div className="space-y-2">
            <Label>Move Y</Label>
            <Input type="number" value={moveY} onChange={e => onMoveChange(moveX, parseInt(e.target.value))} />
          </div>
        </div>
      )}

      {type === 'WHEEL' && (
        <div className="space-y-2">
          <Label>Scroll Amount</Label>
          <Input type="number" value={value} onChange={e => onValueChange(parseInt(e.target.value))} placeholder="Positive=Up, Negative=Down" />
        </div>
      )}
    </div>
  );
}
