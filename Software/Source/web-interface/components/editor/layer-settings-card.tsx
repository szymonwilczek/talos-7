import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';

const SUGGESTED_LAYER_EMOJIS = ['🎮', '💼', '🏠', '🔧', '⚡', '📧', '💻', '🎵', '📝', '☕', '🗡️', '❤️', '🔔', '🧪', '🔒', '☂️', '🦕', '👻', '🔫', '⏳', '🌷'];

interface LayerSettingsCardProps {
  layerName: string;
  layerEmoji: string;
  onUpdate: (name: string, emoji: string) => void;
}

export function LayerSettingsCard({ layerName, layerEmoji, onUpdate }: LayerSettingsCardProps) {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Layer Settings</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        <div className="space-y-2">
          <Label>Layer Emoji</Label>
          <div className="flex gap-2 flex-wrap">
            {SUGGESTED_LAYER_EMOJIS.map((emoji) => (
              <Button
                key={emoji}
                variant={layerEmoji === emoji ? 'default' : 'outline'}
                onClick={() => onUpdate(layerName, emoji)}
                className="text-xl w-12 h-12 p-0"
              >
                {emoji}
              </Button>
            ))}
          </div>
        </div>

        <div className="space-y-2">
          <Label htmlFor="layer-name">Layer Name</Label>
          <Input
            id="layer-name"
            value={layerName}
            onChange={(e) =>
              onUpdate(e.target.value.replace(/[^a-zA-Z0-9 ]/g, '').slice(0, 15), layerEmoji)
            }
            maxLength={15}
            placeholder="Enter layer name"
          />
        </div>
      </CardContent>
    </Card>
  );
}
