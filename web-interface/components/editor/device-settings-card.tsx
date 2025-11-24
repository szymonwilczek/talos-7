import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';

interface DeviceSettingsCardProps {
  oledTimeout: number;
  onTimeoutChange: (val: number[]) => void;
}

export function DeviceSettingsCard({ oledTimeout, onTimeoutChange }: DeviceSettingsCardProps) {
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
            step={30}
            onValueChange={onTimeoutChange}
            className="w-full"
          />
          <p className="text-[10px] text-muted-foreground">
            Set to 0 to disable auto-sleep. Display wakes up on any key press.
          </p>
        </div>
      </CardContent>
    </Card>
  );
}
