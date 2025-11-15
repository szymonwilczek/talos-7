'use client';

import { MacroEntry } from '@/lib/types/config.types';
import { Badge } from '@/components/ui/badge';

interface KeycapButtonProps {
  buttonIndex: number;
  macro: MacroEntry;
  isSelected: boolean;
  isHovered: boolean;
  onClick: () => void;
  onMouseEnter: () => void;
  onMouseLeave: () => void;
}

export function KeycapButton({
  buttonIndex,
  macro,
  isSelected,
  isHovered,
  onClick,
  onMouseEnter,
  onMouseLeave,
}: KeycapButtonProps) {
  const hoverTransform = isHovered ? 'scale(0.98) translateY(2px)' : 'scale(1) translateY(0)';
  const selectedTransform = isSelected ? 'scale(0.95) translateY(4px)' : 'scale(1) translateY(0)';
  const transform = isSelected ? selectedTransform : hoverTransform;
  const shadowOffset = isSelected ? 1 : isHovered ? 2 : 4;

  const background = (isSelected || isHovered)
    ? 'linear-gradient(135deg, #1f2937 0%, #111827 100%)'
    : 'linear-gradient(135deg, #374151 0%, #1f2937 100%)';

  const borderColor = (isSelected || isHovered) ? '#374151' : '#4b5563';

  return (
    <div
      onClick={onClick}
      onMouseEnter={onMouseEnter}
      onMouseLeave={onMouseLeave}
      className="relative w-24 h-24 cursor-pointer group"
      style={{
        transform,
        transition: 'transform 0.1s ease',
      }}
    >
      {/* Shadow */}
      <div
        className="absolute inset-0 rounded-lg"
        style={{
          background: 'linear-gradient(135deg, rgba(0,0,0,0.3) 0%, rgba(0,0,0,0.1) 100%)',
          transform: `translateY(${shadowOffset}px)`,
          borderRadius: '8px',
          transition: 'transform 0.1s ease',
        }}
      />

      {/* Keycap Body */}
      <div
        className="relative w-full h-full rounded-lg border-2 flex flex-col items-center justify-center gap-1 overflow-hidden"
        style={{
          background,
          borderColor,
          transition: 'background 0.3s ease, border-color 0.3s ease',
          boxShadow: isSelected
            ? 'inset 0 2px 4px rgba(255,255,255,0.2), 0 4px 8px rgba(0,0,0,0.3)'
            : 'inset 0 2px 4px rgba(255,255,255,0.1), 0 4px 8px rgba(0,0,0,0.2)',
        }}
      >
        <Badge
          variant="secondary"
          className="absolute top-1 right-1 w-5 h-5 p-0 flex items-center justify-center text-[10px] bg-black/50 text-white border-0"
        >
          {buttonIndex + 1}
        </Badge>

        {/* Top Surface Highlight */}
        <div
          className="absolute inset-1 rounded-md"
          style={{
            background: 'linear-gradient(135deg, rgba(255,255,255,0.1) 0%, rgba(255,255,255,0.05) 50%, rgba(0,0,0,0.1) 100%)',
          }}
        />

        {/* Content */}
        <div className="relative z-10 flex flex-col items-center justify-center gap-1">
          <div className="text-3xl">{macro.emoji || '⚪'}</div>
          <div className="text-[10px] truncate w-full text-center text-white/90">
            {macro.name}
          </div>
        </div>
      </div>
    </div>
  );
}
