'use client';

import { useState, useEffect, useCallback } from 'react';
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
import { CheckCircle } from 'lucide-react';

const SUGGESTED_LAYER_EMOJIS = ['🎮', '💼', '🏠', '⚙️', '⚡', '📧', '💻', '🎵', '📝', '🔧'];

export function MacroConfigurator() {
  const [serialService] = useState(() => new SerialService());
  const [status, setStatus] = useState<ConnectionStatus>('DISCONNECTED');
  const [error, setError] = useState<ConnectionError | null>(null);
  const [config, setConfig] = useState<GlobalConfig | null>(null);
  const [activeLayer, setActiveLayer] = useState(0);
  const [selectedButton, setSelectedButton] = useState<number | null>(null);
  const [hoveredButton, setHoveredButton] = useState<number | null>(null);
  const [pendingChanges, setPendingChanges] = useState<Map<string, any>>(new Map());
  const [isSaving, setIsSaving] = useState(false);
  const [saveProgress, setSaveProgress] = useState(0);
  const [saveSuccess, setSaveSuccess] = useState(false);

  useEffect(() => {
    if (!SerialService.isSupported()) {
      setError({
        type: 'BROWSER_NOT_SUPPORTED',
        message: 'Web Serial API not supported. Use Chrome or Edge.',
      });
      setStatus('ERROR');
    }
  }, []);

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
    setPendingChanges(new Map());
  };

  const handleLayerNameChange = useCallback(
    (name: string, emoji: string) => {
      if (!config) return;
      const changeKey = `layer_${activeLayer}`;
      setPendingChanges((prev) => {
        const next = new Map(prev);
        next.set(changeKey, { type: 'layer', layer: activeLayer, name, emoji });
        return next;
      });
      setConfig({
        ...config,
        layers: config.layers.map((layer, idx) =>
          idx === activeLayer ? { ...layer, name, emoji } : layer
        ),
      });
    },
    [config, activeLayer]
  );

  const handleMacroChange = useCallback(
    (buttonIndex: number, macro: MacroEntry) => {
      if (!config) return;
      const changeKey = `macro_${activeLayer}_${buttonIndex}`;
      setPendingChanges((prev) => {
        const next = new Map(prev);
        next.set(changeKey, {
          type: 'macro',
          layer: activeLayer,
          button: buttonIndex,
          macro,
        });
        return next;
      });
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
    },
    [config, activeLayer]
  );

  const handleSaveChanges = async () => {
    if (pendingChanges.size === 0) return;
    setIsSaving(true);
    setSaveProgress(0);
    setSaveSuccess(false);

    try {
      const changes = Array.from(pendingChanges.values());
      const totalChanges = changes.length;

      for (let i = 0; i < changes.length; i++) {
        const change = changes[i];
        if (change.type === 'layer') {
          await serialService.setLayerName(change.layer, change.name, change.emoji);
        } else if (change.type === 'macro') {
          await serialService.setMacro(
            change.layer,
            change.button,
            change.macro.type,
            change.macro.value,
            change.macro.macroString,
            change.macro.name,
            change.macro.emoji
          );
        }
        setSaveProgress(((i + 1) / (totalChanges + 1)) * 100);
      }

      await serialService.saveFlash();
      setSaveProgress(100);
      setPendingChanges(new Map());
      setSaveSuccess(true);
      setTimeout(() => setSaveSuccess(false), 3000);
    } catch (err) {
      setError({
        type: 'PROTOCOL_ERROR',
        message: err instanceof Error ? err.message : 'Save failed',
      });
    } finally {
      setIsSaving(false);
      setSaveProgress(0);
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
      {/* Header with Connection & Save */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse" />
            <span>Connected</span>
          </div>
          <Button variant="outline" onClick={handleDisconnect}>
            Disconnect
          </Button>
        </div>
        <Button
          onClick={handleSaveChanges}
          disabled={pendingChanges.size === 0 || isSaving}
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

      {isSaving && <Progress value={saveProgress} />}
      {saveSuccess && (
        <Alert>
          <CheckCircle className="h-4 w-4" />
          <AlertDescription>Configuration saved successfully!</AlertDescription>
        </Alert>
      )}

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
                handleLayerNameChange(e.target.value.slice(0, 16), currentLayer.emoji)
              }
              maxLength={16}
              placeholder="Enter layer name"
            />
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
        onClose={() => setSelectedButton(null)}
        onSave={handleMacroChange}
      />
    </div>
  );
}
