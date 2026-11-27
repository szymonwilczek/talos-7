import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { Switch } from '@/components/ui/switch';

interface DeviceSettingsCardProps {
  oledTimeout: number;
  onTimeoutChange: (val: number[]) => void;
  companionAutostart: boolean;
  onCompanionAutostartChange: (checked: boolean) => void;
}

export function DeviceSettingsCard({
  oledTimeout,
  onTimeoutChange,
  companionAutostart,
  onCompanionAutostartChange,
}: DeviceSettingsCardProps) {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Device Settings</CardTitle>
      </CardHeader>
      <CardContent className="space-y-6">
        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <Label>OLED Sleep Timeout</Label>
            <span className="text-sm text-muted-foreground font-mono">
              {oledTimeout === 0 ? "Always On" : `${Math.floor(oledTimeout / 60)}m ${oledTimeout % 60}s`}
            </span>
          </div>

          <Slider
            value={[oledTimeout]}
            min={0}
            max={1800}
            step={10}
            onValueChange={onTimeoutChange}
            className="w-full"
          />
          <p className="text-[10px] text-muted-foreground">
            Set to 0 to disable auto-sleep. Display wakes up on any key press.
          </p>
        </div>

        <div className="flex items-center justify-between gap-4 border-t pt-4">
          <div className="space-y-1">
            <Label htmlFor="companion-autostart">Start in Companion Mode on boot</Label>
            <p className="text-[11px] text-muted-foreground">
              Launch Talos Mochi automatically when plugged in. Hold [7] for 1.5s to return to macros.
            </p>
          </div>
          <Switch
            id="companion-autostart"
            checked={companionAutostart}
            onCheckedChange={onCompanionAutostartChange}
          />
        </div>
      </CardContent>
    </Card>
  );
}
