'use client';

import { MacroEntry } from '@/lib/types/config.types';
import { Button } from '@/components/ui/button';
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
  return (
    <Button
      variant={isSelected ? 'default' : 'outline'}
      onClick={onClick}
      onMouseEnter={onMouseEnter}
      onMouseLeave={onMouseLeave}
      className="relative w-24 h-24 flex flex-col border-2 cursor-pointer items-center justify-center gap-1 transition-transform"
      style={{
        transform: isHovered ? 'translateY(2px)' : 'translateY(0)',
      }}
    >
      <Badge variant="secondary" className="absolute top-1 right-1 w-5 h-5 p-0 flex items-center justify-center text-[10px]">
        {buttonIndex + 1}
      </Badge>
      <div className="text-3xl">{macro.emoji || '⚪'}</div>
      <div className="text-[10px] truncate w-full text-center">{macro.name}</div>
    </Button>
  );
}
