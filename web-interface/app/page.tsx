import { MacroConfigurator } from '@/components/macro/MacroConfigurator';

export default function Home() {
  return (
    <div className="container mx-auto py-8 px-4">
      <div className="mb-8 text-center">
        <h1 className="text-4xl font-bold text-slate-900 dark:text-slate-100 mb-2">
          Macro Keyboard Configurator
        </h1>
        <p className="text-slate-600 dark:text-slate-400">
          Configure your Raspberry Pi Pico macro keyboard with 4 layers and 7 programmable buttons
        </p>
      </div>
      <MacroConfigurator />
    </div>
  );
}
