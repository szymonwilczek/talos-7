"use client";

import { LedMode, LedModeLabels } from "@/lib/types/config.types";
import {
  Card,
  CardAction,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Button } from "@/components/ui/button";

interface LedSettingsCardProps {
  ledModes: LedMode[];
  onLedModeChange: (button: number, mode: LedMode) => void;
  onSetAll: (mode: LedMode) => void;
}

interface LedKeyProps {
  buttonIndex: number;
  mode: LedMode;
  onToggle: () => void;
}

function LedKey({ buttonIndex, mode, onToggle }: LedKeyProps) {
  const alwaysOn = mode === LedMode.ALWAYS_ON;

  return (
    <button
      type="button"
      onClick={onToggle}
      aria-pressed={alwaysOn}
      title={`Button ${buttonIndex + 1}: ${LedModeLabels[mode]} — click to switch`}
      className="group flex flex-col items-center gap-1.5 focus-visible:outline-none"
    >
      <div
        className="relative flex h-16 w-16 flex-col items-center justify-center rounded-lg border-2 transition-all duration-200 group-focus-visible:ring-2 group-focus-visible:ring-ring group-active:translate-y-[2px]"
        style={{
          background: alwaysOn
            ? "linear-gradient(135deg, #374151 0%, #1f2937 100%)"
            : "linear-gradient(135deg, #1f2937 0%, #111827 100%)",
          borderColor: alwaysOn ? "#f59e0b" : "#374151",
          boxShadow: alwaysOn
            ? "inset 0 2px 4px rgba(255,255,255,0.1), 0 0 16px rgba(245,158,11,0.35)"
            : "inset 0 2px 4px rgba(255,255,255,0.06)",
        }}
      >
        <span
          className="absolute top-2 h-1.5 w-1.5 rounded-full transition-all duration-200"
          style={{
            background: alwaysOn ? "#fbbf24" : "#4b5563",
            boxShadow: alwaysOn ? "0 0 8px 2px rgba(251,191,36,0.8)" : "none",
          }}
        />
        <span
          className="mt-2 font-mono text-lg font-semibold transition-colors"
          style={{ color: alwaysOn ? "#fef3c7" : "#9ca3af" }}
        >
          {buttonIndex + 1}
        </span>
      </div>
      <span
        className="text-[10px] font-medium uppercase tracking-wider transition-colors"
        style={{ color: alwaysOn ? "#f59e0b" : "var(--muted-foreground)" }}
      >
        {LedModeLabels[mode]}
      </span>
    </button>
  );
}

export function LedSettingsCard({
  ledModes,
  onLedModeChange,
  onSetAll,
}: LedSettingsCardProps) {
  const toggle = (button: number) =>
    onLedModeChange(
      button,
      ledModes[button] === LedMode.ALWAYS_ON
        ? LedMode.REACTIVE
        : LedMode.ALWAYS_ON,
    );

  return (
    <Card>
      <CardHeader>
        <CardTitle>Per-Key Lighting</CardTitle>
        <CardDescription>
          Reactive LEDs flash briefly on key press. Always On LEDs stay lit
          even when the display sleeps.
        </CardDescription>
        <CardAction>
          <div className="flex gap-2">
            <Button
              variant="outline"
              size="sm"
              onClick={() => onSetAll(LedMode.REACTIVE)}
            >
              All Reactive
            </Button>
            <Button
              variant="outline"
              size="sm"
              onClick={() => onSetAll(LedMode.ALWAYS_ON)}
            >
              All Always On
            </Button>
          </div>
        </CardAction>
      </CardHeader>
      <CardContent>
        <div className="flex flex-col items-center gap-4">
          <div className="flex items-center gap-4">
            <div className="flex flex-col gap-2">
              <div className="flex gap-2">
                {[0, 1, 2].map((i) => (
                  <LedKey
                    key={i}
                    buttonIndex={i}
                    mode={ledModes[i]}
                    onToggle={() => toggle(i)}
                  />
                ))}
              </div>
              <div className="flex gap-2">
                {[3, 4, 5].map((i) => (
                  <LedKey
                    key={i}
                    buttonIndex={i}
                    mode={ledModes[i]}
                    onToggle={() => toggle(i)}
                  />
                ))}
              </div>
            </div>
            <LedKey
              buttonIndex={6}
              mode={ledModes[6]}
              onToggle={() => toggle(6)}
            />
          </div>

        </div>
      </CardContent>
    </Card>
  );
}
