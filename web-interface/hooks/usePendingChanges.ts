import { useState, useEffect, useRef } from "react";
import { GlobalConfig } from "@/lib/types/config.types";

export function usePendingChanges(config: GlobalConfig | null) {
  const originalConfigRef = useRef<GlobalConfig | null>(null);
  const [pendingChanges, setPendingChanges] = useState(new Map<string, any>());
  const [updateTrigger, setUpdateTrigger] = useState(0);

  // aktualizuj originalConfig przy zmianie config
  useEffect(() => {
    if (config && !originalConfigRef.current) {
      originalConfigRef.current = JSON.parse(JSON.stringify(config));
      console.log("🔄 Initial originalConfig set");
    }
  }, [config]);

  // przeliczaj zmiany przy kazdej zmianie config LUB updateTrigger
  useEffect(() => {
    if (!config || !originalConfigRef.current) {
      setPendingChanges(new Map());
      return;
    }

    const changes = new Map<string, any>();
    const originalConfig = originalConfigRef.current;

    console.log("🔍 Calculating changes...");

    config.layers.forEach((layer, layerIdx) => {
      const origLayer = originalConfig.layers[layerIdx];

      // zmiany w nazwie/emoji warstwy
      if (layer.name !== origLayer.name || layer.emoji !== origLayer.emoji) {
        console.log(`🔄 Layer ${layerIdx} changed:`, {
          from: { name: origLayer.name, emoji: origLayer.emoji },
          to: { name: layer.name, emoji: layer.emoji },
        });
        const changeKey = `layer_${layerIdx}`;
        changes.set(changeKey, {
          type: "layer",
          layer: layerIdx,
          name: layer.name,
          emoji: layer.emoji,
        });
      }

      // zmiany w makrach
      layer.macros.forEach((macro, buttonIdx) => {
        const origMacro = origLayer.macros[buttonIdx];

        const macroChanged =
          macro.type !== origMacro.type ||
          macro.value !== origMacro.value ||
          macro.macroString !== origMacro.macroString ||
          macro.name !== origMacro.name ||
          macro.emoji !== origMacro.emoji ||
          macro.script !== origMacro.script ||
          macro.scriptPlatform !== origMacro.scriptPlatform;

        if (macroChanged) {
          console.log(`🔄 Macro ${layerIdx}-${buttonIdx} changed`);
          const changeKey = `macro_${layerIdx}_${buttonIdx}`;
          changes.set(changeKey, {
            type: "macro",
            layer: layerIdx,
            button: buttonIdx,
            macro,
          });
        }
      });
    });

    console.log(`📊 Total changes: ${changes.size}`);
    setPendingChanges(changes);
  }, [config, updateTrigger]);

  const updateOriginalConfig = () => {
    if (config) {
      console.log("🔄 Updating originalConfig to current config");
      originalConfigRef.current = JSON.parse(JSON.stringify(config));
      setUpdateTrigger((prev) => prev + 1);
      console.log("✅ originalConfig updated and recalculation triggered");
    }
  };

  return { pendingChanges, updateOriginalConfig };
}
