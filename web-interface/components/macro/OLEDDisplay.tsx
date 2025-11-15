'use client';

import { LayerConfig } from '@/lib/types/config.types';
import { Card, CardContent } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';

interface OLEDDisplayProps {
  layer: LayerConfig;
  layerIndex: number;
  totalLayers: number;
  selectedButton: number | null;
  hoveredButton: number | null;
}

export function OLEDDisplay({
  layer,
  layerIndex,
  totalLayers,
  selectedButton,
  hoveredButton,
}: OLEDDisplayProps) {
  const activeButton = selectedButton ?? hoveredButton;

  // console.log(layer.macros[activeButton]);

  return (
    <Card className="w-full max-w-2xl h-[290px] bg-black border-2">
      <CardContent className="h-full p-4 text-white text-sm font-mono overflow-auto">
        {activeButton !== null ? (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <Badge variant={selectedButton !== null ? 'default' : 'outline'}>
                {selectedButton !== null ? 'Editing' : 'Preview'}
              </Badge>
              <span>Button {activeButton + 1}</span>
            </div>
            <Separator className="bg-gray-700" />
            <div className="text-center space-y-2">
              <div className="text-5xl">{layer.macros[activeButton].emoji || ''}</div>
              <div className="text-lg">{layer.macros[activeButton].name}</div>
              <div className="text-xs text-gray-400">
                {layer.macros[activeButton].type === 0
                  ? 'Key Press'
                  : layer.macros[activeButton].type === 1
                    ? 'Text String'
                    : 'Layer Toggle'}
              </div>
            </div>
          </div>
        ) : (
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <span className="flex items-center gap-2">
                <span className="text-xl">{layer.emoji}</span>
                <span>{layer.name}</span>
              </span>
              <Badge variant="outline">
                Layer {layerIndex + 1}/{totalLayers}
              </Badge>
            </div>
            <Separator className="bg-gray-700" />
            <div className="space-y-1">
              {layer.macros.slice(0, 6).map((macro, idx) =>
                macro.name !== 'Empty' ? (
                  <div key={idx} className="flex items-center gap-2 text-xs">
                    <Badge variant="secondary">{idx + 1}</Badge>
                    <span>{macro.emoji}</span>
                    <span>{macro.name}</span>
                  </div>
                ) : null
              )}
            </div>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
