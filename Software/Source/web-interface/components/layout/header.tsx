import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { HardDrive, Upload, Download } from 'lucide-react';

interface HeaderProps {
  firmwareVersion?: string;
  pendingChangesCount: number;
  isSaving: boolean;
  saveProgress: number;
  onDisconnect: () => void;
  onEnterBootloader: () => void;
  onImport: (e: React.ChangeEvent<HTMLInputElement>) => void;
  onExport: () => void;
  onSave: () => void;
}

export function Header({
  firmwareVersion,
  pendingChangesCount,
  isSaving,
  saveProgress,
  onDisconnect,
  onEnterBootloader,
  onImport,
  onExport,
  onSave
}: HeaderProps) {
  return (
    <div className="flex flex-col md:flex-row items-center justify-between gap-4">
      <div className="flex items-center gap-4">
        <div className="flex items-center gap-2">
          <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse" />
          <span>Connected</span>
        </div>
        {firmwareVersion && (
          <Badge variant="outline" className="font-mono text-xs border-green-500/30 text-green-600 dark:text-green-400">
            v{firmwareVersion}
          </Badge>
        )}
        <Button variant="outline" size="sm" onClick={onDisconnect}>
          Disconnect
        </Button>
      </div>

      <div className="flex items-center gap-2">
        <Button
          size="sm"
          className="bg-green-500 hover:bg-green-600 cursor-pointer"
          onClick={onEnterBootloader}
          title="Restart device to upload new firmware"
        >
          <HardDrive className="mr-2 h-4 w-4" /> Update Firmware
        </Button>
        <div className="relative">
          <input
            type="file"
            accept=".json"
            onChange={onImport}
            className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
          />
          <Button variant="outline" size="sm">
            <Upload className="mr-2 h-4 w-4" /> Upload Config
          </Button>
        </div>

        <Button variant="outline" size="sm" onClick={onExport}>
          <Download className="mr-2 h-4 w-4" /> Download Config
        </Button>

        <Button
          onClick={onSave}
          disabled={pendingChangesCount === 0 || isSaving}
          variant="default"
        >
          {isSaving ? (
            <>Saving... {Math.round(saveProgress)}%</>
          ) : pendingChangesCount === 0 ? (
            'No changes'
          ) : (
            <>Save {pendingChangesCount} changes</>
          )}
        </Button>
      </div>
    </div>
  );
}
