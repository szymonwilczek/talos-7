import { useState, useEffect, useRef } from "react";
import { ConfigChange, GlobalConfig } from "@/lib/types/config.types";

export function usePendingChanges(config: GlobalConfig | null) {
  const originalConfigRef = useRef<GlobalConfig | null>(null);
  const [pendingChanges, setPendingChanges] = useState(
    new Map<string, ConfigChange>(),
  );
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

    const changes = new Map<string, ConfigChange>();
    const originalConfig = originalConfigRef.current;

    console.log("🔍 Calculating changes...");

    // ustawienia globalne (OLED Timeout)
    const currentTimeout = config.oledTimeout ?? 300;
    const originalTimeout = originalConfig.oledTimeout ?? 300;

    if (currentTimeout !== originalTimeout) {
      console.log(
        `🔄 OLED timeout changed: {from: ${originalTimeout}, to: ${currentTimeout}}`,
      );
      changes.set("setting-oled", {
        type: "setting",
        settingName: "oledTimeout",
        value: currentTimeout,
      });
    }

    // warstwy i makra
    config.layers.forEach((layer, layerIdx) => {
      const origLayer = originalConfig.layers[layerIdx];

      // zmiany w nazwie/emoji warstwy
      if (layer.name !== origLayer.name || layer.emoji !== origLayer.emoji) {
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
          macro.scriptPlatform !== origMacro.scriptPlatform ||
          JSON.stringify(macro.keySequence) !==
            JSON.stringify(origMacro.keySequence) ||
          JSON.stringify(macro.terminalShortcut) !==
            JSON.stringify(origMacro.terminalShortcut) ||
          macro.repeatCount !== origMacro.repeatCount ||
          macro.repeatInterval !== origMacro.repeatInterval ||
          macro.moveX !== origMacro.moveX ||
          macro.moveY !== origMacro.moveY;

        if (macroChanged) {
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
