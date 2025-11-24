'use client';

import { Tabs, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { FIRMWARE_CONSTANTS } from '@/lib/types/config.types';

interface LayerTabsProps {
  activeLayer: number;
  layerNames: string[];
  onLayerChange: (layer: number) => void;
}

export function LayerTabs({ activeLayer, layerNames, onLayerChange }: LayerTabsProps) {
  return (
    <Tabs value={activeLayer.toString()} onValueChange={(v) => onLayerChange(parseInt(v))}>
      <TabsList className="grid w-full grid-cols-4">
        {Array.from({ length: FIRMWARE_CONSTANTS.MAX_LAYERS }, (_, i) => (
          <TabsTrigger key={i} value={i.toString()}>
            {layerNames[i] || `Layer ${i + 1}`}
          </TabsTrigger>
        ))}
      </TabsList>
    </Tabs>
  );
}
