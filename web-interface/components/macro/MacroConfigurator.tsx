'use client';

import { useState, useEffect } from 'react';
import { SerialService } from '@/lib/services/serial.service';
import {
  GlobalConfig,
  ConnectionStatus,
  ConnectionError,
  MacroEntry,
} from '@/lib/types/config.types';
import { ConnectionPanel } from './ConnectionPanel';
import { LayerTabs } from './LayerTabs';
import { PCBVisualization } from './PCBVisualization';
import { ButtonEditDialog } from './ButtonEditDialog';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { Progress } from '@/components/ui/progress';
import { Alert, AlertDescription } from '@/components/ui/alert';
import { CheckCircle, Download, HardDrive, Upload } from 'lucide-react';
import { MacroType, ScriptPlatform } from '@/lib/types/macro.types';
import { usePendingChanges } from '@/hooks/usePendingChanges';
import { motion, AnimatePresence } from 'framer-motion';
import { Slider } from '../ui/slider';
import { Badge } from '../ui/badge';

const SUGGESTED_LAYER_EMOJIS = ['🎮', '💼', '🏠', '🔧', '⚡', '📧', '💻', '🎵', '📝', '☕', '🗡️', '❤️', '🔔', '🧪', '🔒', '☂️', '🦕', '👻', '🔫', '⏳', '🌷'];

export function MacroConfigurator() {
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
      {/* Header with Connection & Save & Import/Export */}
      <div className="flex flex-col md:flex-row items-center justify-between gap-4">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse" />
            <span>Connected</span>
          </div>
          {config.firmwareVersion && (
            <Badge variant="outline" className="font-mono text-xs border-green-500/30 text-green-600 dark:text-green-400">
              v{config.firmwareVersion}
            </Badge>
          )}
          <Button variant="outline" size="sm" onClick={handleDisconnect}>
            Disconnect
          </Button>
        </div>

        <div className="flex items-center gap-2">
          <Button
            size="sm"
            className="bg-green-500 hover:bg-green-600 cursor-pointer"
            onClick={handleEnterBootloader}
            title="Restart device to upload new firmware"
          >
            <HardDrive className="mr-2 h-4 w-4" /> Update Firmware
          </Button>
          <div className="relative">
            <input
              type="file"
              accept=".json"
              onChange={handleImportConfig}
              className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
              title="Import JSON Config"
            />
            <Button variant="outline" size="sm">
              <Upload className="mr-2 h-4 w-4" /> Upload Config
            </Button>
          </div>

          <Button variant="outline" size="sm" onClick={handleExportConfig}>
            <Download className="mr-2 h-4 w-4" /> Download Config
          </Button>

          <Button
            onClick={handleSaveChanges}
            disabled={pendingChanges.size === 0 || isSaving}
            variant="default"
          >
            {isSaving ? (
              <>Saving... {Math.round(saveProgress)}%</>
            ) : pendingChanges.size === 0 ? (
              'No changes'
            ) : (
              <>Save {pendingChanges.size} changes</>
            )}
          </Button>
        </div>
      </div>

      <AnimatePresence mode="wait">
        {isSaving && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -20 }}
            transition={{ duration: 0.3 }}
            key="saving"
          >
            <Progress value={saveProgress} />
          </motion.div>
        )}

        {saveSuccess && !isSaving && (
          <motion.div
            initial={{ opacity: 0, scale: 0.95, y: 20 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: -20 }}
            transition={{ duration: 0.4, ease: [0.4, 0, 0.2, 1] }}
            key="success"
          >
            <Alert>
              <CheckCircle className="h-4 w-4" />
              <AlertDescription>Configuration saved successfully!</AlertDescription>
            </Alert>
          </motion.div>
        )}
      </AnimatePresence>

      {/* Layer Tabs */}
      <LayerTabs
        activeLayer={activeLayer}
        layerNames={config.layers.map((l) => `${l.emoji} ${l.name}`)}
        onLayerChange={(idx) => {
          setActiveLayer(idx);
          setSelectedButton(null);
        }}
      />

      {/* Layer Settings Card */}
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
                  variant={currentLayer.emoji === emoji ? 'default' : 'outline'}
                  onClick={() =>
                    handleLayerNameChange(currentLayer.name, emoji)
                  }
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
              value={currentLayer.name}
              onChange={(e) =>
                handleLayerNameChange(e.target.value
                  .replace(/[^a-zA-Z0-9 ]/g, '')
                  .slice(0, 15), currentLayer.emoji)
              }
              maxLength={15}
              placeholder="Enter layer name"
            />
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>Device Settings</CardTitle>
        </CardHeader>
        <CardContent className="space-y-6">
          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <Label>OLED Sleep Timeout</Label>
              <span className="text-sm text-muted-foreground font-mono">
                {config.oledTimeout === 0 ? "Always On" : `${Math.floor(config.oledTimeout / 60)}m ${config.oledTimeout % 60}s`}
              </span>
            </div>

            <Slider
              value={[config.oledTimeout]}
              min={0}
              max={1800} // max 30 min
              step={30}  // co 30 sekund
              onValueChange={handleTimeoutChange}
              className="w-full"
            />
            <p className="text-[10px] text-muted-foreground">
              Set to 0 to disable auto-sleep. Display wakes up on any key press.
            </p>
          </div>
        </CardContent>
      </Card>

      {/* Button Visualization */}
      <PCBVisualization
        config={config}
        activeLayer={activeLayer}
        selectedButton={selectedButton}
        hoveredButton={hoveredButton}
        onButtonClick={setSelectedButton}
        onButtonHover={setHoveredButton}
      />

      {/* Button Edit Dialog */}
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
