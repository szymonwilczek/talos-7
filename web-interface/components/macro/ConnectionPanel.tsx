'use client';

import { Button } from '@/components/ui/button';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { ConnectionStatus, ConnectionError } from '@/lib/types/macro.types';
import { AlertCircle, CheckCircle, Loader2, Unplug, Usb } from 'lucide-react';

interface ConnectionPanelProps {
  status: ConnectionStatus;
  error: ConnectionError | null;
  onConnect: () => void;
  onDisconnect: () => void;
}

export function ConnectionPanel({
  status,
  error,
  onConnect,
  onDisconnect,
}: ConnectionPanelProps) {
  const isConnected = status === 'CONNECTED';
  const isConnecting = status === 'CONNECTING';

  return (
    <Card className="mb-6">
      <CardHeader>
        <div className="flex items-center justify-between">
          <div>
            <CardTitle className="flex items-center gap-2">
              <Usb className="h-5 w-5" />
              Device Connection
            </CardTitle>
            <CardDescription>
              Connect your Raspberry Pi Pico macro keyboard
            </CardDescription>
          </div>
          <Badge
            variant={
              isConnected ? 'default' : status === 'ERROR' ? 'destructive' : 'secondary'
            }
          >
            {status}
          </Badge>
        </div>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Błąd połączenia */}
        {error && (
          <Alert variant="destructive">
            <AlertCircle className="h-4 w-4" />
            <AlertTitle>Connection Error</AlertTitle>
            <AlertDescription>
              {error.type === 'BROWSER_NOT_SUPPORTED' && (
                <>
                  Your browser doesn't support Web Serial API. Please use{' '}
                  <strong>Chrome</strong>, <strong>Edge</strong>, or{' '}
                  <strong>Opera</strong>.
                  <br />
                  <span className="text-sm mt-2 block">
                    ❌ Firefox is not supported
                  </span>
                </>
              )}
              {error.type !== 'BROWSER_NOT_SUPPORTED' && error.message}
            </AlertDescription>
          </Alert>
        )}

        {/* Status połączenia */}
        {isConnected && (
          <Alert>
            <CheckCircle className="h-4 w-4" />
            <AlertTitle>Connected</AlertTitle>
            <AlertDescription>
              Your device is connected and ready to configure.
            </AlertDescription>
          </Alert>
        )}

        {/* Przyciski */}
        <div className="flex gap-2">
          {!isConnected ? (
            <Button
              onClick={onConnect}
              disabled={isConnecting}
              className="w-full"
              size="lg"
            >
              {isConnecting ? (
                <>
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                  Connecting...
                </>
              ) : (
                <>
                  <Usb className="mr-2 h-4 w-4" />
                  Connect Device
                </>
              )}
            </Button>
          ) : (
            <Button
              onClick={onDisconnect}
              variant="destructive"
              className="w-full"
              size="lg"
            >
              <Unplug className="mr-2 h-4 w-4" />
              Disconnect
            </Button>
          )}
        </div>

        {/* Instrukcje */}
        {!isConnected && !error && (
          <div className="text-sm text-slate-600 dark:text-slate-400 space-y-1">
            <p className="font-medium">Connection Instructions:</p>
            <ol className="list-decimal list-inside space-y-1 ml-2">
              <li>Make sure your Pico is plugged in via USB</li>
              <li>Click "Connect Device"</li>
              <li>Select "Raspberry Pi Pico" from the browser dialog</li>
              <li>Click "Connect"</li>
            </ol>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
