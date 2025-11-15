'use client';

import { useState, useEffect } from 'react';
import { ConnectionPanel } from './ConnectionPanel';
import { LayerTabs } from './LayerTabs';
import { MacroSlot } from './MacroSlot';
import { Button } from '@/components/ui/button';
import { Card, CardContent } from '@/components/ui/card';
import { Progress } from '@/components/ui/progress';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Separator } from '@/components/ui/separator';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { SerialService } from '@/lib/services/serial.service';
import {
  GlobalConfig,
  ConnectionStatus,
  ConnectionError,
  MacroEntry,
  FIRMWARE_CONSTANTS,
} from '@/lib/types/macro.types';
import { findConfigChanges, cloneConfig } from '@/lib/utils/config-compare';
import { Save, CheckCircle, AlertCircle, Loader2 } from 'lucide-react';

export function MacroConfigurator() {
  // ==================== STATE ====================
  const [status, setStatus] = useState<ConnectionStatus>('DISCONNECTED');
  const [error, setError] = useState<ConnectionError | null>(null);
  const [serialService] = useState(() => new SerialService());
  const [config, setConfig] = useState<GlobalConfig | null>(null);
  const [originalConfig, setOriginalConfig] = useState<GlobalConfig | null>(null);
  const [activeLayer, setActiveLayer] = useState(0);
  const [isSaving, setIsSaving] = useState(false);
  const [saveProgress, setSaveProgress] = useState(0);
  const [saveSuccess, setSaveSuccess] = useState(false);

  // ==================== SPRAWDZENIE WSPARCIA PRZEGLĄDARKI ====================
  useEffect(() => {
    if (!SerialService.isSupported()) {
      setError({
        type: 'BROWSER_NOT_SUPPORTED',
        message: 'Web Serial API is not supported in this browser',
      });
      setStatus('ERROR');
    }
  }, []);

  // ==================== POŁĄCZENIE ====================
  const handleConnect = async () => {
    setStatus('CONNECTING');
    setError(null);

    try {
      console.log('🔌 Connecting to Pico...');
      await serialService.connectPico();
      console.log('✅ Connected successfully');

      setStatus('CONNECTED');

      console.log('📖 Reading configuration...');
      const loadedConfig = await serialService.readConfig();
      console.log('✅ Configuration loaded:', loadedConfig);

      setConfig(loadedConfig);
      setOriginalConfig(cloneConfig(loadedConfig));

      console.log('🎉 Ready to configure!');
    } catch (err) {
      console.error('❌ Connection error:', err);
      setStatus('ERROR');

      const errorMessage = err instanceof Error ? err.message : 'Unknown error';
      console.error('Error message:', errorMessage);

      if (errorMessage.includes('BROWSER_NOT_SUPPORTED')) {
        setError({
          type: 'BROWSER_NOT_SUPPORTED',
          message: 'Web Serial API is not supported in this browser',
        });
      } else if (errorMessage.includes('USER_CANCELLED')) {
        setError({
          type: 'DEVICE_NOT_FOUND',
          message: 'Device selection was cancelled',
        });
        setStatus('DISCONNECTED');
      } else if (errorMessage.includes('PORT_BUSY')) {
        setError({
          type: 'DEVICE_NOT_FOUND',
          message: 'Port is already open in another application. Please close picocom, minicom, or any other serial terminal.',
        });
      } else {
        setError({
          type: 'PROTOCOL_ERROR',
          message: `Failed to load configuration: ${errorMessage}`,
        });
      }
    }
  };

  const handleDisconnect = async () => {
    await serialService.disconnect();
    setStatus('DISCONNECTED');
    setConfig(null);
    setOriginalConfig(null);
    setActiveLayer(0);
  };

  // ==================== EDYCJA KONFIGURACJI ====================
  const handleMacroChange = (buttonIndex: number, newMacro: MacroEntry) => {
    if (!config) return;

    const newConfig = cloneConfig(config);
    newConfig.layers[activeLayer].macros[buttonIndex] = newMacro;
    setConfig(newConfig);
  };

  const handleLayerNameChange = (layerIndex: number, newName: string) => {
    if (!config) return;

    const newConfig = cloneConfig(config);
    newConfig.layers[layerIndex].name = newName;
    setConfig(newConfig);
  };

  // ==================== ZAPISYWANIE DO PICO ====================
  const handleSave = async () => {
    if (!config || !originalConfig) return;

    setIsSaving(true);
    setSaveProgress(0);
    setSaveSuccess(false);

    try {
      const changes = findConfigChanges(originalConfig, config);

      if (changes.length === 0) {
        console.log('No changes to save');
        setIsSaving(false);
        return;
      }

      console.log(`Saving ${changes.length} changes...`);

      // wysylanie kazdej zmiany
      for (let i = 0; i < changes.length; i++) {
        const change = changes[i];

        if (change.type === 'MACRO' && change.button !== undefined) {
          await serialService.setMacro(
            change.layer,
            change.button,
            change.data as MacroEntry
          );
        } else if (change.type === 'LAYER_NAME') {
          await serialService.setLayerName(change.layer, change.data as string);
        }

        setSaveProgress(((i + 1) / (changes.length + 1)) * 100);
      }

      await serialService.saveFlash();
      setSaveProgress(100);

      setOriginalConfig(cloneConfig(config));

      setSaveSuccess(true);
      setTimeout(() => setSaveSuccess(false), 3000);

      console.log('Configuration saved successfully');
    } catch (err) {
      console.error('Save error:', err);
      setError({
        type: 'PROTOCOL_ERROR',
        message: err instanceof Error ? err.message : 'Failed to save configuration',
      });
    } finally {
      setIsSaving(false);
      setSaveProgress(0);
    }
  };

  // ==================== SPRAWDZENIE CZY SĄ ZMIANY ====================
  const hasChanges = () => {
    if (!config || !originalConfig) return false;
    return findConfigChanges(originalConfig, config).length > 0;
  };

  // ==================== RENDER ====================
  const isConnected = status === 'CONNECTED';
  const isConfigLoaded = config !== null;

  return (
    <div className="space-y-6">
      <ConnectionPanel
        status={status}
        error={error}
        onConnect={handleConnect}
        onDisconnect={handleDisconnect}
      />

      {isConnected && !config && (
        <Card>
          <CardContent className="pt-6">
            <Button onClick={async () => {
              try {
                console.log('🔄 Manual config load...');
                const cfg = await serialService.readConfig();
                console.log('✅ Loaded:', cfg);
                setConfig(cfg);
                setOriginalConfig(cloneConfig(cfg));
              } catch (err) {
                console.error('❌ Error:', err);
              }
            }}>
              🔄 Load Configuration Manually
            </Button>
          </CardContent>
        </Card>
      )}

      {/* Panel edycji (tylko gdy połączony) */}
      {isConnected && isConfigLoaded && (
        <>
          {saveSuccess && (
            <Alert className="border-green-500 bg-green-50 dark:bg-green-950">
              <CheckCircle className="h-4 w-4 text-green-600" />
              <AlertTitle className="text-green-600">Success!</AlertTitle>
              <AlertDescription className="text-green-600">
                Configuration saved to device successfully.
              </AlertDescription>
            </Alert>
          )}

          {/* Zakładki warstw */}
          <Card>
            <CardContent className="pt-6">
              <LayerTabs
                activeLayer={activeLayer}
                layerNames={config.layers.map((l) => l.name)}
                onLayerChange={setActiveLayer}
              />

              <Separator className="my-6" />

              {/* Edycja nazwy warstwy */}
              <div className="mb-6 space-y-2">
                <Label htmlFor="layer-name">Layer Name</Label>
                <Input
                  id="layer-name"
                  value={config.layers[activeLayer].name}
                  onChange={(e) => handleLayerNameChange(activeLayer, e.target.value)}
                  placeholder="Enter layer name"
                  maxLength={16}
                />
              </div>

              {/* Grid makr (7 slotów) */}
              <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
                {config.layers[activeLayer].macros.map((macro, index) => (
                  <MacroSlot
                    key={`${activeLayer}-${index}`}
                    buttonIndex={index}
                    macro={macro}
                    onChange={(newMacro) => handleMacroChange(index, newMacro)}
                  />
                ))}
              </div>
            </CardContent>
          </Card>

          {/* Przycisk zapisywania */}
          <Card>
            <CardContent className="pt-6">
              <div className="space-y-4">
                {/* Progress bar podczas zapisywania */}
                {isSaving && (
                  <div className="space-y-2">
                    <div className="flex items-center justify-between text-sm">
                      <span>Saving configuration...</span>
                      <span>{Math.round(saveProgress)}%</span>
                    </div>
                    <Progress value={saveProgress} />
                  </div>
                )}

                {/* Przycisk Save */}
                <Button
                  onClick={handleSave}
                  disabled={!hasChanges() || isSaving}
                  className="w-full"
                  size="lg"
                >
                  {isSaving ? (
                    <>
                      <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                      Saving...
                    </>
                  ) : (
                    <>
                      <Save className="mr-2 h-4 w-4" />
                      Save to Pico
                      {hasChanges() && (
                        <span className="ml-2 text-xs bg-white/20 px-2 py-1 rounded">
                          {findConfigChanges(originalConfig!, config).length} changes
                        </span>
                      )}
                    </>
                  )}
                </Button>

                {/* Info o braku zmian */}
                {!hasChanges() && !isSaving && (
                  <p className="text-sm text-center text-slate-500">
                    No changes to save
                  </p>
                )}
              </div>
            </CardContent>
          </Card>
        </>
      )}

      {isConnected && !isConfigLoaded && (
        <Card>
          <CardContent className="pt-6">
            <div className="text-center space-y-4">
              <Loader2 className="h-12 w-12 animate-spin mx-auto text-slate-400" />
              <div>
                <h3 className="text-lg font-semibold mb-2">
                  Loading Configuration
                </h3>
                <p className="text-sm text-slate-600 dark:text-slate-400">
                  Please wait while we load your device configuration...
                </p>
              </div>
            </div>
          </CardContent>
        </Card>
      )}

      {/* Instrukcje gdy nie połączony */}
      {!isConnected && status !== 'ERROR' && (
        <Card>
          <CardContent className="pt-6">
            <div className="text-center space-y-4">
              <div className="mx-auto w-16 h-16 bg-slate-200 dark:bg-slate-800 rounded-full flex items-center justify-center">
                <AlertCircle className="h-8 w-8 text-slate-400" />
              </div>
              <div>
                <h3 className="text-lg font-semibold mb-2">
                  Connect Your Device
                </h3>
                <p className="text-sm text-slate-600 dark:text-slate-400">
                  Click the "Connect Device" button above to start configuring your macro keyboard
                </p>
              </div>
            </div>
          </CardContent>
        </Card>
      )}
    </div>
  );
}
