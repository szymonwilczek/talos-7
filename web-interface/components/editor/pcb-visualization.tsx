'use client';

import { GlobalConfig } from '@/lib/types/config.types';
import { Card, CardContent } from '@/components/ui/card';
import { OLEDDisplay } from '@/components/oled/oled-display';
import { KeycapButton } from '@/components/layout/KeycapButton';

interface PCBVisualizationProps {
  config: GlobalConfig;
  activeLayer: number;
  selectedButton: number | null;
  hoveredButton: number | null;
  onButtonClick: (buttonIndex: number) => void;
  onButtonHover: (buttonIndex: number | null) => void;
}

export function PCBVisualization({
  config,
  activeLayer,
  selectedButton,
  hoveredButton,
  onButtonClick,
  onButtonHover,
}: PCBVisualizationProps) {
  const layer = config.layers[activeLayer];

  return (
    <Card>
      <CardContent className="p-6">
        <div className="flex flex-col items-center gap-6">
          <OLEDDisplay
            layer={layer}
            layerIndex={activeLayer}
            totalLayers={config.layers.length}
            selectedButton={selectedButton}
            hoveredButton={hoveredButton}
          />

          {/* Buttons Layout */}
          <div className="flex justify-center items-center gap-4">
            {/* 2 rows x 3 columns */}
            <div className="flex flex-col gap-2">
              {/* Top row */}
              <div className="flex gap-2">
                <KeycapButton
                  buttonIndex={0}
                  macro={layer.macros[0]}
                  isSelected={selectedButton === 0}
                  isHovered={hoveredButton === 0}
                  onClick={() => onButtonClick(0)}
                  onMouseEnter={() => onButtonHover(0)}
                  onMouseLeave={() => onButtonHover(null)}
                />
                <KeycapButton
                  buttonIndex={1}
                  macro={layer.macros[1]}
                  isSelected={selectedButton === 1}
                  isHovered={hoveredButton === 1}
                  onClick={() => onButtonClick(1)}
                  onMouseEnter={() => onButtonHover(1)}
                  onMouseLeave={() => onButtonHover(null)}
                />
                <KeycapButton
                  buttonIndex={2}
                  macro={layer.macros[2]}
                  isSelected={selectedButton === 2}
                  isHovered={hoveredButton === 2}
                  onClick={() => onButtonClick(2)}
                  onMouseEnter={() => onButtonHover(2)}
                  onMouseLeave={() => onButtonHover(null)}
                />
              </div>

              {/* Bottom row */}
              <div className="flex gap-2">
                <KeycapButton
                  buttonIndex={3}
                  macro={layer.macros[3]}
                  isSelected={selectedButton === 3}
                  isHovered={hoveredButton === 3}
                  onClick={() => onButtonClick(3)}
                  onMouseEnter={() => onButtonHover(3)}
                  onMouseLeave={() => onButtonHover(null)}
                />
                <KeycapButton
                  buttonIndex={4}
                  macro={layer.macros[4]}
                  isSelected={selectedButton === 4}
                  isHovered={hoveredButton === 4}
                  onClick={() => onButtonClick(4)}
                  onMouseEnter={() => onButtonHover(4)}
                  onMouseLeave={() => onButtonHover(null)}
                />
                <KeycapButton
                  buttonIndex={5}
                  macro={layer.macros[5]}
                  isSelected={selectedButton === 5}
                  isHovered={hoveredButton === 5}
                  onClick={() => onButtonClick(5)}
                  onMouseEnter={() => onButtonHover(5)}
                  onMouseLeave={() => onButtonHover(null)}
                />
              </div>
            </div>

            {/* Layer Switch (same size as others) */}
            <KeycapButton
              buttonIndex={6}
              macro={layer.macros[6]}
              isSelected={selectedButton === 6}
              isHovered={hoveredButton === 6}
              onClick={() => onButtonClick(6)}
              onMouseEnter={() => onButtonHover(6)}
              onMouseLeave={() => onButtonHover(null)}
            />
          </div>
        </div>
      </CardContent>
    </Card>
  );
}
