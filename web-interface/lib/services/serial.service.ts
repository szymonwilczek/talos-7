import {
  GlobalConfig,
  MacroEntry,
  MacroType,
  FIRMWARE_CONSTANTS,
  DEFAULT_LAYER_EMOJIS,
} from "../types/macro.types";

/**
 * serwis do komunikacji z Pico przez Web Serial API
 */
export class SerialService {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private readBuffer: string = "";

  /**
   * sprawdza czy przegladarka wspiera Web Serial API
   */
  static isSupported(): boolean {
    return typeof navigator !== "undefined" && "serial" in navigator;
  }

  /**
   * nawiazuje polaczenie z Pico
   */
  async connectPico(): Promise<void> {
    if (!SerialService.isSupported()) {
      throw new Error("Web Serial API is not supported in this browser");
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
      this.readBuffer = "";
    } catch (error) {
      console.error("Disconnect error:", error);
    }
  }

  /**
   * wysyla komende i czeka na odpowiedz
   */
  private async sendCommand(command: string): Promise<string> {
    if (!this.writer) {
      throw new Error("Not connected");
    }

    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(command + "\n"));

    return await this.readUntilNewline();
  }

  /**
   * czyta dane az do napotkania newline
   */
  private async readUntilNewline(timeout: number = 5000): Promise<string> {
    if (!this.reader) {
      throw new Error("Not connected");
    }

    const startTime = Date.now();

    while (true) {
      if (Date.now() - startTime > timeout) {
        throw new Error("Read timeout");
      }

      // czy w buforze jest juz newline
      const newlineIndex = this.readBuffer.indexOf("\n");
      if (newlineIndex !== -1) {
        const line = this.readBuffer.substring(0, newlineIndex).trim();
        this.readBuffer = this.readBuffer.substring(newlineIndex + 1);
        return line;
      }

      // czytaj wiecej danych
      const { value, done } = await this.reader.read();
      if (done) {
        throw new Error("Stream closed");
      }

      const decoder = new TextDecoder();
      this.readBuffer += decoder.decode(value);
    }
  }

  /**
   * pobiera konfiguracje z Pico
   */
  async readConfig(): Promise<GlobalConfig> {
    console.log("📤 Sending GET_CONF...");
    const response = await this.sendCommand("GET_CONF");
    console.log("📥 Response:", response);

    if (response !== "CONF_START") {
      throw new Error(`Expected CONF_START, got: ${response}`);
    }

    const config: GlobalConfig = {
      layers: Array.from({ length: FIRMWARE_CONSTANTS.MAX_LAYERS }, (_, i) => ({
        name: `Layer ${i + 1}`,
        emoji: DEFAULT_LAYER_EMOJIS[i] || "⚙️",
        macros: Array.from({ length: FIRMWARE_CONSTANTS.NUM_BUTTONS }, () => ({
          type: MacroType.KEY_PRESS,
          value: 0,
          macroString: "",
          name: "Empty",
          emoji: "",
        })),
      })),
    };

    console.log("📖 Parsing configuration lines...");
    let lineCount = 0;

    while (true) {
      try {
        const line = await this.readUntilNewline(10000);
        lineCount++;
        console.log(`Line ${lineCount}:`, line);

        if (line === "CONF_END") {
          console.log("✅ Configuration parsing complete");
          break;
        }

        // LAYER_NAME|layer|name|emoji
        if (line.startsWith("LAYER_NAME|")) {
          const parts = line.split("|");
          const layerIndex = parseInt(parts[1]);
          const layerName = parts[2] || `Layer ${layerIndex + 1}`;
          const layerEmoji =
            parts[3] || DEFAULT_LAYER_EMOJIS[layerIndex] || "⚙️";

          config.layers[layerIndex].name = layerName;
          config.layers[layerIndex].emoji = layerEmoji;

          console.log(`  ➜ Layer ${layerIndex}: ${layerEmoji} ${layerName}`);
        }

        // MACRO|layer|button|type|value|string|name|emoji
        if (line.startsWith("MACRO|")) {
          const parts = line.split("|");
          const layerIndex = parseInt(parts[1]);
          const buttonIndex = parseInt(parts[2]);
          const type = parseInt(parts[3]) as MacroType;
          const value = parseInt(parts[4]);
          const macroString = parts[5] || "";
          const name = parts[6] || "Empty";
          const emoji = parts[7] || "";

          config.layers[layerIndex].macros[buttonIndex] = {
            type,
            value,
            macroString,
            name,
            emoji,
          };

          console.log(
            `  ➜ Macro L${layerIndex}B${buttonIndex}: ${emoji} ${name}`,
          );
        }
      } catch (error) {
        if (error instanceof Error && error.message.includes("timeout")) {
          console.log("⏱️ Timeout waiting for more data");
          break;
        }
        throw error;
      }
    }

    console.log("📊 Total lines parsed:", lineCount);
    console.log("✅ Configuration loaded successfully");
    return config;
  }
  /**
   * zapisuje pojedyncze makro
   */
  async setMacro(
    layer: number,
    button: number,
    type: MacroType,
    value: number,
    macroString: string,
    name: string,
    emoji: string,
  ): Promise<void> {
    console.log(`📤 Setting macro L${layer}B${button}: ${emoji} ${name}`);

    const command = `SET_MACRO|${layer}|${button}|${type}|${value}|${macroString}|${name}|${emoji}`;
    const response = await this.sendCommand(command);

    if (response !== "OK") {
      throw new Error(`Failed to set macro: ${response}`);
    }

    console.log("✅ Macro set successfully");
  }

  /**
   * ustawia nazwe warstwy
   */
  async setLayerName(
    layer: number,
    name: string,
    emoji: string,
  ): Promise<void> {
    console.log(`📤 Setting layer ${layer} name: ${emoji} ${name}`);

    const command = `SET_LAYER_NAME|${layer}|${name}|${emoji}`;
    const response = await this.sendCommand(command);

    if (response !== "OK") {
      throw new Error(`Failed to set layer name: ${response}`);
    }

    console.log("✅ Layer name set successfully");
  }

  /**
   * zapisuje konfiguracje do flash
   */
  async saveFlash(): Promise<void> {
    const response = await this.sendCommand("SAVE_FLASH");

    if (response !== "OK") {
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
