import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Switch } from "@/components/ui/switch";


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

  const isAfkMode = type === 'MOVE' && repeatCount === 0;

  const MOVE_PIXELS_AFK = 20;

  const handleAfkToggle = (checked: boolean) => {
    if (checked) {
      onRepeatCountChange?.(0);
      onMoveChange(MOVE_PIXELS_AFK, 0);
      if ((repeatInterval || 0) < 500) {
        onRepeatIntervalChange?.(2000);
      }
    } else {
      onRepeatCountChange?.(1);
      onMoveChange(0, 0);
    }
  };

  return (
    <div className="space-y-6">
      {type === 'BUTTON' && (
        <>
          <div className="grid grid-cols-2 gap-4 p-3 bg-muted/20 rounded-md border">
            <div className="space-y-2">
              <Label>Repetitions</Label>
              <Input
                type="number" min={1} max={10000} value={repeatCount || 1}
                onChange={e => onRepeatCountChange?.(parseInt(e.target.value))}
              />
              <p className="text-[10px] text-muted-foreground">Count</p>
            </div>
            <div className="space-y-2">
              <Label>Interval (ms)</Label>
              <Input
                type="number" min={0} max={5000} value={repeatInterval || 0}
                onChange={e => onRepeatIntervalChange?.(parseInt(e.target.value))}
              />
              <p className="text-[10px] text-muted-foreground">Delay</p>
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
        <div className="space-y-4">

          <div className="flex items-center justify-between p-4 bg-muted/40 rounded-lg border">
            <div className="space-y-0.5">
              <Label className="text-base font-semibold">AFK Jiggler Mode</Label>
              <p className="text-xs text-muted-foreground">
                Override settings for continuous movement
              </p>
            </div>
            <Switch
              checked={isAfkMode}
              onCheckedChange={handleAfkToggle}
            />
          </div>

          <div className={`grid grid-cols-2 gap-4 transition-opacity ${isAfkMode ? 'opacity-80' : ''}`}>
            <div className="space-y-2">
              <Label>Move X (px)</Label>
              <Input
                type="number"
                value={moveX}
                disabled={isAfkMode}
                onChange={e => onMoveChange(parseInt(e.target.value) || 0, moveY)}
              />
            </div>
            <div className="space-y-2">
              <Label>Move Y (px)</Label>
              <Input
                type="number"
                value={moveY}
                disabled={isAfkMode}
                onChange={e => onMoveChange(moveX, parseInt(e.target.value) || 0)}
              />
            </div>
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div className={`space-y-2 ${isAfkMode ? 'opacity-80' : ''}`}>
              <Label>Repetitions</Label>
              <Input
                type="number"
                min={0} max={1000}
                value={repeatCount}
                disabled={isAfkMode}
                onChange={e => onRepeatCountChange?.(parseInt(e.target.value) || 1)}
              />
              <p className="text-[10px] text-muted-foreground">
                {isAfkMode ? "0 (Infinite)" : "Count"}
              </p>
            </div>

            <div className="space-y-2">
              <Label className={isAfkMode ? "text-blue-500 font-bold" : ""}>
                Interval (ms)
              </Label>
              <Input
                type="number"
                min={0} max={60000}
                value={repeatInterval}
                onChange={e => onRepeatIntervalChange?.(parseInt(e.target.value) || 0)}
                className={isAfkMode ? "border-blue-500/50 bg-blue-500/10" : ""}
              />
              <p className="text-[10px] text-muted-foreground">Delay between moves</p>
            </div>
          </div>

          {isAfkMode && (
            <div className="p-3 bg-blue-500/10 text-blue-400 rounded-md text-xs border border-blue-500/20 flex items-center gap-2">
              <span className="text-xl">ℹ️</span>
              <span>
                Device will move mouse <strong>{MOVE_PIXELS_AFK}px</strong> every <strong>{repeatInterval}ms</strong>.
                Press the button again to stop.
              </span>
            </div>
          )}
        </div>
      )}

      {type === 'WHEEL' && (
        <div className="space-y-2">
          <Label>Scroll Amount</Label>
          <Input
            type="number"
            value={value}
            onChange={e => onValueChange(parseInt(e.target.value) || 0)}
            placeholder="Positive=Up, Negative=Down"
          />
          <p className="text-xs text-muted-foreground">Positive values scroll UP, negative scroll DOWN</p>
        </div>
      )}
    </div>
  );
}
