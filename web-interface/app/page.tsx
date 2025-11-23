import { MacroConfigurator } from '@/components/macro/MacroConfigurator';
import Image from 'next/image';

export default function Home() {
  return (
    <div className="container mx-auto py-8 px-4">
      <div className="mb-8 text-center">
        <h1 className="flex flex-row items-center justify-center text-4xl font-bold text-slate-900 dark:text-slate-100 mb-2">
          <Image
            src="/talos_logo.png"
            alt="Talos Logo"
            width={72}
            height={72}
            className="mr-4"
          />
          Talos Configurator
        </h1>
        <p className="text-slate-600 dark:text-slate-400">
          Configure your Talos SudoPad macros with 4 layers and 7 programmable buttons
        </p>
      </div>
      <MacroConfigurator />
    </div>
  );
}
