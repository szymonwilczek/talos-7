'use client';

import { useState, useEffect } from 'react';
import { SerialService } from '@/lib/services/serial.service';
import {
  GlobalConfig,
  ConnectionStatus,
  ConnectionError,
  MacroEntry,
} from '@/lib/types/config.types';
import { ConnectionPanel } from '@/components/device/connection-panel';
import { LayerTabs } from '@/components/layout/LayerTabs';
import { PCBVisualization } from './pcb-visualization';
import { ButtonEditDialog } from '@/components/macro-dialog/button-edit-dialog';
import { MacroType, ScriptPlatform } from '@/lib/types/macro.types';
import { usePendingChanges } from '@/hooks/usePendingChanges';
import { Header } from '../layout/header';
import { SaveFeedback } from '../layout/save-feedback';
import { LayerSettingsCard } from './layer-settings-card';
import { DeviceSettingsCard } from './device-settings-card';

export function Configurator() {
  const [serialService] = useState(() => new SerialService());
  const [status, setStatus] = useState<ConnectionStatus>('DISCONNECTED');
  const [error, setError] = useState<ConnectionError | null>(null);
  const [config, setConfig] = useState<GlobalConfig | null>(null);
  const [activeLayer, setActiveLayer] = useState(0);
  const [selectedButton, setSelectedButton] = useState<number | null>(null);
  const [hoveredButton, setHoveredButton] = useState<number | null>(null);
  const [isSaving, setIsSaving] = useState(false);
  const [saveProgress, setSaveProgress] = useState(0);
  const [saveSuccess, setSaveSuccess] = useState(false);
  const { pendingChanges, updateOriginalConfig } = usePendingChanges(config);

  useEffect(() => {
    if (!SerialService.isSupported()) {
      setError({
        type: 'BROWSER_NOT_SUPPORTED',
        message: 'Web Serial API not supported. Use Chrome or Edge.',
      });
      setStatus('ERROR');
    }
  }, []);

  useEffect(() => {
    if (status === 'CONNECTED' && config) {
      serialService.setConfigMode(pendingChanges.size > 0 ? 1 : 0)
        .then(() => console.log("✅ setConfigMode sent successfully"))
        .catch((err) => console.error("❌ setConfigMode failed:", err));
    }
  }, [pendingChanges.size, status, config]);

  const handleConnect = async () => {
    setStatus('CONNECTING');
    setError(null);
    try {
      await serialService.connectPico();
      setStatus('CONNECTED');
      const loadedConfig = await serialService.readConfig();
      setConfig(loadedConfig);
    } catch (err) {
      setStatus('ERROR');
      const errorMessage = err instanceof Error ? err.message : 'Unknown error';
      if (errorMessage.includes('USER_CANCELLED')) {
        setError({ type: 'DEVICE_NOT_FOUND', message: 'Connection cancelled' });
        setStatus('DISCONNECTED');
      } else {
        setError({ type: 'PROTOCOL_ERROR', message: errorMessage });
      }
    }
  };

  const handleDisconnect = async () => {
    await serialService.disconnect();
    setStatus('DISCONNECTED');
    setConfig(null);
    setActiveLayer(0);
    setSelectedButton(null);
  };

  const handleExportConfig = () => {
    if (!config) return;

    // JSON
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(config, null, 2));
    const downloadAnchorNode = document.createElement('a');
    downloadAnchorNode.setAttribute("href", dataStr);
    downloadAnchorNode.setAttribute("download", "talos_config.json");
    document.body.appendChild(downloadAnchorNode);
    downloadAnchorNode.click();
    downloadAnchorNode.remove();
  };

  const handleImportConfig = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      try {
        const json = JSON.parse(e.target?.result as string);

        if (json.layers && Array.isArray(json.layers) && json.layers.length > 0) {
          setConfig(json as GlobalConfig);
          setSelectedButton(null);
          console.log("✅ Config imported successfully");
        } else {
          alert("Invalid config file format: Missing layers data.");
        }
      } catch (err) {
        console.error("Error parsing JSON", err);
        alert("Error parsing JSON file");
      }
    };
    reader.readAsText(file);
    event.target.value = '';
  };

  const handleLayerNameChange = (name: string, emoji: string) => {
    if (!config) return;
    setConfig({
      ...config,
      layers: config.layers.map((layer, idx) =>
        idx === activeLayer ? { ...layer, name, emoji } : layer
      ),
    });
  };

  const handleMacroChange = async (buttonIndex: number, macro: MacroEntry) => {
    if (!config) return;

    setConfig({
      ...config,
      layers: config.layers.map((layer, idx) =>
        idx === activeLayer
          ? {
            ...layer,
            macros: layer.macros.map((m, btnIdx) =>
              btnIdx === buttonIndex ? macro : m
            ),
          }
          : layer
      ),
    });
  };

  const handleTimeoutChange = (val: number[]) => {
    if (!config) return;
    setConfig({
      ...config,
      oledTimeout: val[0]
    });
  };

  const handleEnterBootloader = async () => {
    const confirmed = window.confirm(
      "This will restart your Talos device into Firmware Update mode.\n\n" +
      "1. The device will disconnect from this app.\n" +
      "2. A folder named 'RPI-RP2' will open on your computer.\n" +
      "3. Drag and drop the new .uf2 firmware file into that folder."
    );

    if (confirmed) {
      try {
        await serialService.enterBootloader();
        setStatus('DISCONNECTED');
        setConfig(null);
        alert("Device is now in update mode. Go into your file explorer and open the 'RPI-RP2' drive to copy the firmware file (.uf2).");
      } catch (err) {
        console.error("Failed to enter bootloader:", err);
        setError({ type: 'PROTOCOL_ERROR', message: 'Failed to restart device' });
      }
    }
  };

  const handleSaveChanges = async () => {
    if (pendingChanges.size === 0) return;
    setIsSaving(true);
    setSaveProgress(0);
    setSaveSuccess(false);

    console.log('🔄 Starting save process...');

    try {
      const changes = Array.from(pendingChanges.values());
      const totalChanges = changes.length;

      for (let i = 0; i < changes.length; i++) {
        const change = changes[i];

        if (change.type === 'setting') {
          if (change.settingName === 'oledTimeout') {
            console.log(`📤 Updating OLED Timeout: ${change.value}s`);
            await serialService.setOledTimeout(change.value);
          }
        }
        else if (change.type === 'layer') {
          console.log(`📤 Processing layer change: ${change.emoji} ${change.name}`);
          if (change.layer !== undefined && change.name !== undefined && change.emoji !== undefined) {
            await serialService.setLayerName(change.layer, change.name, change.emoji);
          }
        }
        else if (change.type === 'macro') {
          const macro = change.macro;
          if (change.layer !== undefined && change.button !== undefined) {
            if (!macro) return;
            if (macro.type === MacroType.SCRIPT && macro.script) {
              await serialService.setScript(
                change.layer,
                change.button,
                macro.scriptPlatform || ScriptPlatform.LINUX,
                macro.script,
                macro.terminalShortcut || []
              );
            } else {
              if (!macro) return;
              await serialService.setMacro(
                change.layer,
                change.button,
                macro.type,
                macro.value,
                macro.macroString,
                macro.name,
                macro.emoji,
                macro.keySequence,
                macro.repeatCount,
                macro.repeatInterval,
                macro.moveX,
                macro.moveY
              );
            }
          }
        }

        await new Promise(resolve => setTimeout(resolve, 150));
        setSaveProgress(((i + 1) / (totalChanges + 1)) * 100);
      }

      console.log('📤 Calling saveFlash...');
      await serialService.saveFlash();
      await new Promise(resolve => setTimeout(resolve, 300));

      console.log('✅ saveFlash completed successfully');
      setSaveProgress(100);

      console.log('🔄 Updating local state after save...');
      updateOriginalConfig();

      await new Promise(resolve => setTimeout(resolve, 400));
      setSaveSuccess(true);
      setTimeout(() => setSaveSuccess(false), 3000);
    } catch (err) {
      console.error('❌ Save failed with error:', err);
      setError({
        type: 'PROTOCOL_ERROR',
        message: err instanceof Error ? err.message : 'Save failed',
      });
    } finally {
      setIsSaving(false);
      setTimeout(() => setSaveProgress(0), 500);
    }
  };

  if (status !== 'CONNECTED' || !config) {
    return (
      <ConnectionPanel
        status={status}
        error={error}
        onConnect={handleConnect}
        onDisconnect={handleDisconnect}
      />
    );
  }

  const currentLayer = config.layers[activeLayer];

  return (
    <div className="container mx-auto p-6 space-y-6">

      <Header
        firmwareVersion={config.firmwareVersion}
        pendingChangesCount={pendingChanges.size}
        isSaving={isSaving}
        saveProgress={saveProgress}
        onDisconnect={handleDisconnect}
        onEnterBootloader={handleEnterBootloader}
        onImport={handleImportConfig}
        onExport={handleExportConfig}
        onSave={handleSaveChanges}
      />

      <SaveFeedback
        isSaving={isSaving}
        saveProgress={saveProgress}
        saveSuccess={saveSuccess}
      />

      <LayerTabs
        activeLayer={activeLayer}
        layerNames={config.layers.map((l) => `${l.emoji} ${l.name}`)}
        onLayerChange={(idx) => {
          setActiveLayer(idx);
          setSelectedButton(null);
        }}
      />

      <LayerSettingsCard
        layerName={currentLayer.name}
        layerEmoji={currentLayer.emoji}
        onUpdate={handleLayerNameChange}
      />

      <DeviceSettingsCard
        oledTimeout={config.oledTimeout}
        onTimeoutChange={handleTimeoutChange}
      />

      <PCBVisualization
        config={config}
        activeLayer={activeLayer}
        selectedButton={selectedButton}
        hoveredButton={hoveredButton}
        onButtonClick={setSelectedButton}
        onButtonHover={setHoveredButton}
      />

      <ButtonEditDialog
        open={selectedButton !== null}
        buttonIndex={selectedButton}
        macro={selectedButton !== null ? currentLayer.macros[selectedButton] : null}
        layerMacros={currentLayer.macros}
        onClose={() => setSelectedButton(null)}
        onSave={handleMacroChange}
      />
    </div>
  );
}
