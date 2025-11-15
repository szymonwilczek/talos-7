import { GlobalConfig, MacroEntry, MacroType, FIRMWARE_CONSTANTS } from '../types/macro.types';

/**
 * serwis do komunikacji z Pico przez Web Serial API
 */
export class SerialService {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private readBuffer: string = '';

  /**
   * sprawdza czy przegladarka wspiera Web Serial API
   */
  static isSupported(): boolean {
    return typeof navigator !== 'undefined' && 'serial' in navigator;
  }

  /**
   * nawiazuje polaczenie z Pico
   */
  async connectPico(): Promise<void> {
    if (!SerialService.isSupported()) {
      throw new Error('Web Serial API is not supported in this browser');
    }

    try {
      // popup wybrania urzadzenia
      this.port = await navigator.serial.requestPort({
        filters: [
          { usbVendorId: 0x2e8a, usbProductId: 0x000a }, // Raspberry Pi Pico
        ],
      });

      // port z baudrate 115200
      await this.port.open({ baudRate: 115200 });

      // reader i writer
      if (this.port.readable) {
        this.reader = this.port.readable.getReader();
      }
      if (this.port.writable) {
        this.writer = this.port.writable.getWriter();
      }

      // stabilizacja polaczenia
      await this.delay(500);
    } catch (error) {
      throw new Error(`Failed to connect: ${error}`);
    }
  }

  /**
   * rozlacza sie z Pico
   */
  async disconnect(): Promise<void> {
    try {
      if (this.reader) {
        await this.reader.cancel();
        this.reader.releaseLock();
        this.reader = null;
      }
      if (this.writer) {
        await this.writer.releaseLock();
        this.writer = null;
      }
      if (this.port) {
        await this.port.close();
        this.port = null;
      }
      this.readBuffer = '';
    } catch (error) {
      console.error('Disconnect error:', error);
    }
  }

  /**
   * wysyla komende i czeka na odpowiedz
   */
  private async sendCommand(command: string): Promise<string> {
    if (!this.writer) {
      throw new Error('Not connected');
    }

    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(command + '\n'));

    return await this.readUntilNewline();
  }

  /**
   * czyta dane az do napotkania newline
   */
  private async readUntilNewline(timeout: number = 5000): Promise<string> {
    if (!this.reader) {
      throw new Error('Not connected');
    }

    const startTime = Date.now();

    while (true) {
      if (Date.now() - startTime > timeout) {
        throw new Error('Read timeout');
      }

      // czy w buforze jest juz newline
      const newlineIndex = this.readBuffer.indexOf('\n');
      if (newlineIndex !== -1) {
        const line = this.readBuffer.substring(0, newlineIndex).trim();
        this.readBuffer = this.readBuffer.substring(newlineIndex + 1);
        return line;
      }

      // czytaj wiecej danych
      const { value, done } = await this.reader.read();
      if (done) {
        throw new Error('Stream closed');
      }

      const decoder = new TextDecoder();
      this.readBuffer += decoder.decode(value);
    }
  }

  /**
   * pobiera konfiguracje z Pico
   */
  async readConfig(): Promise<GlobalConfig> {
    // GET_CONF
    const response = await this.sendCommand('GET_CONF');

    if (response !== 'CONF_START') {
      throw new Error(`Expected CONF_START, got: ${response}`);
    }

    // inicjalizacja struktury konfiguracji
    const config: GlobalConfig = {
      layers: Array.from({ length: FIRMWARE_CONSTANTS.MAX_LAYERS }, (_, i) => ({
        name: `Layer ${i + 1}`,
        macros: Array.from({ length: FIRMWARE_CONSTANTS.NUM_BUTTONS }, () => ({
          type: MacroType.KEY_PRESS,
          value: 0,
          macroString: '',
          name: 'Empty',
        })),
      })),
    };

    // parsowanie odpowiedzi
    while (true) {
      const line = await this.readUntilNewline();

      if (line === 'CONF_END') {
        break;
      }

      // LAYER_NAME|0|GameLayer
      if (line.startsWith('LAYER_NAME|')) {
        const parts = line.split('|');
        const layerIndex = parseInt(parts[1]);
        const layerName = parts[2] || `Layer ${layerIndex + 1}`;
        config.layers[layerIndex].name = layerName;
      }

      // MACRO|0|0|0|4|Hello|KeyA
      if (line.startsWith('MACRO|')) {
        const parts = line.split('|');
        const layerIndex = parseInt(parts[1]);
        const buttonIndex = parseInt(parts[2]);
        const type = parseInt(parts[3]) as MacroType;
        const value = parseInt(parts[4]);
        const macroString = parts[5] || '';
        const name = parts[6] || 'Empty';

        config.layers[layerIndex].macros[buttonIndex] = {
          type,
          value,
          macroString,
          name,
        };
      }
    }

    return config;
  }

  /**
   * zapisuje pojedyncze makro
   */
  async setMacro(
    layer: number,
    button: number,
    macro: MacroEntry
  ): Promise<void> {
    const command = `SET_MACRO|${layer}|${button}|${macro.type}|${macro.value}|${macro.macroString}|${macro.name}`;
    const response = await this.sendCommand(command);

    if (response !== 'OK') {
      throw new Error(`SET_MACRO failed: ${response}`);
    }
  }

  /**
   * ustawia nazwe warstwy
   */
  async setLayerName(layer: number, name: string): Promise<void> {
    const command = `SET_LAYER_NAME|${layer}|${name}`;
    const response = await this.sendCommand(command);

    if (response !== 'OK') {
      throw new Error(`SET_LAYER_NAME failed: ${response}`);
    }
  }

  /**
   * zapisuje konfiguracje do flash
   */
  async saveFlash(): Promise<void> {
    const response = await this.sendCommand('SAVE_FLASH');

    if (response !== 'OK') {
      throw new Error(`SAVE_FLASH failed: ${response}`);
    }
  }

  /**
   * pomocnicza funkcja delay
   */
  private delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }
}
