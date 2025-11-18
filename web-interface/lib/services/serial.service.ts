import {
  GlobalConfig,
  MacroEntry,
  MacroType,
  KeyPress,
  FIRMWARE_CONSTANTS,
  DEFAULT_LAYER_EMOJIS,
} from "../types/config.types";

// script platform enum (0=linux, 1=windows, 2=macos)
enum ScriptPlatform {
  LINUX = 0,
  WINDOWS = 1,
  MACOS = 2,
}

const MAX_SCRIPT_SIZE = 2048;

const emoji_strings = [
  "🎮",
  "💼",
  "🏠",
  "⚙️",
  "⚡",
  "📧",
  "💻",
  "🎵",
  "📝",
  "☕",
  "🗡️",
  "❤️",
  "🔔",
  "🧪",
  "🔒",
  "☂️",
  "🦕",
  "👻",
  "🔧",
];

/**
 * service for communication with pico via web serial api
 */
export class SerialService {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private readBuffer: string = "";

  /**
   * checks if browser supports web serial api
   */
  static isSupported(): boolean {
    return typeof navigator !== "undefined" && "serial" in navigator;
  }

  /**
   * establishes connection with pico
   */
  async connectPico(): Promise<void> {
    if (!SerialService.isSupported()) {
      throw new Error("Web Serial API is not supported in this browser");
    }

    try {
      // device selection popup
      this.port = await navigator.serial.requestPort({
        filters: [
          { usbVendorId: 0x2e8a, usbProductId: 0x000a }, // raspberry pi pico
        ],
      });

      // open port with baudrate 115200
      await this.port.open({ baudRate: 115200 });

      // setup reader and writer
      if (this.port.readable) {
        this.reader = this.port.readable.getReader();
      }
      if (this.port.writable) {
        this.writer = this.port.writable.getWriter();
      }

      // connection stabilization
      await this.delay(1000);

      // clear any residual data in buffer
      this.readBuffer = "";

      console.log("✅ Connection stabilized");
    } catch (error) {
      throw new Error(`Failed to connect: ${error}`);
    }
  }

  /**
   * uploads script to specific button
   */
  async setScript(
    layer: number,
    button: number,
    platform: number,
    scriptContent: string,
  ): Promise<void> {
    if (!this.port || !this.writer || !this.reader) {
      throw new Error("Not connected");
    }

    const encoder = new TextEncoder();
    const scriptBytes = encoder.encode(scriptContent);
    const scriptSize = scriptBytes.length;

    if (scriptSize > MAX_SCRIPT_SIZE) {
      throw new Error(
        `Script too large (${scriptSize} > ${MAX_SCRIPT_SIZE} bytes)`,
      );
    }

    const command = `SET_SCRIPT|${layer}|${button}|${platform}|${scriptSize}\n`;
    await this.writeCommand(command);

    const readyResponse = await this.readLine(2000);
    if (!readyResponse.startsWith("READY")) {
      throw new Error("Device not ready to receive script");
    }

    // send raw script bytes
    await this.writer.write(scriptBytes);
    const response = await this.readLine(5000);
    if (!response.startsWith("OK")) {
      throw new Error(`Script upload failed: ${response}`);
    }

    console.log("✅ Script uploaded successfully");
  }

  /**
   * disconnects from pico
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
   * sends command and waits for response
   */
  private async sendCommand(command: string): Promise<string> {
    if (!this.writer) {
      throw new Error("Not connected");
    }

    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(command + "\n"));

    return await this.readLine();
  }

  /**
   * writes command without waiting for response
   */
  private async writeCommand(command: string): Promise<void> {
    if (!this.writer) {
      throw new Error("Not connected");
    }

    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(command));
  }

  /**
   * reads data until newline is encountered
   */
  private async readLine(timeout: number = 5000): Promise<string> {
    if (!this.reader) {
      throw new Error("Not connected");
    }

    const startTime = Date.now();

    while (true) {
      if (Date.now() - startTime > timeout) {
        throw new Error("Read timeout");
      }

      // check if buffer already contains newline
      const newlineIndex = this.readBuffer.indexOf("\n");
      if (newlineIndex !== -1) {
        const line = this.readBuffer.substring(0, newlineIndex).trim();
        this.readBuffer = this.readBuffer.substring(newlineIndex + 1);
        return line;
      }

      // read more data
      const { value, done } = await this.reader.read();
      if (done) {
        throw new Error("Stream closed");
      }

      const decoder = new TextDecoder();
      this.readBuffer += decoder.decode(value);
    }
  }

  /**
   * reads configuration from pico
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
    let pendingScriptMacro: { layer: number; button: number } | null = null;

    while (true) {
      try {
        const line = await this.readLine(10000);
        lineCount++;
        console.log(`Line ${lineCount}: ${line}`);

        if (line === "CONF_END") {
          console.log("✅ Configuration parsing complete");
          break;
        }

        // handle SCRIPT_DATA| (must be right after MACRO|...|3|...)
        if (line.startsWith("SCRIPT_DATA|")) {
          if (!pendingScriptMacro) {
            console.warn("⚠️ SCRIPT_DATA without pending macro");
            continue;
          }

          const scriptContent = line.substring(12); // after "SCRIPT_DATA|"
          const { layer, button } = pendingScriptMacro;

          // unescape special characters
          config.layers[layer].macros[button].script = scriptContent
            .replace(/\\n/g, "\n")
            .replace(/\\r/g, "\r")
            .replace(/\\\|/g, "|")
            .replace(/\\\\/g, "\\");

          console.log(
            `  ➜ Script loaded: ${config.layers[layer].macros[button].script?.length || 0} bytes`,
          );
          pendingScriptMacro = null;
          continue;
        }

        // LAYER_NAME|layer|name|emoji
        if (line.startsWith("LAYER_NAME|")) {
          const parts = line.split("|");
          const layerIndex = parseInt(parts[1]);
          const layerName = parts[2] || `Layer ${layerIndex + 1}`;
          const layerEmojiIndex =
            parseInt(parts[3]) || DEFAULT_LAYER_EMOJIS[layerIndex] || 0;
          config.layers[layerIndex].emoji =
            emoji_strings[layerEmojiIndex] || "🎮";

          config.layers[layerIndex].name = layerName;

          console.log(
            `  ➜ Layer ${layerIndex}: ${emoji_strings[layerEmojiIndex]} ${layerName}`,
          );
          continue;
        }

        if (line.startsWith("MACRO_SEQ|")) {
          const parts = line.split("|");
          const layer = parseInt(parts[1]);
          const button = parseInt(parts[2]);
          const name = parts[3];
          const emoji = parts[4];
          const count = parseInt(parts[5]);

          config.layers[layer].macros[button] = {
            type: MacroType.KEY_SEQUENCE,
            value: 0,
            macroString: "",
            name,
            emoji,
            keySequence: [],
          };

          console.log(`  ➜ Key sequence: ${name} (${count} steps)`);
          continue;
        }

        if (line.startsWith("SEQ_STEP|")) {
          const parts = line.split("|");
          const layer = parseInt(parts[1]);
          const button = parseInt(parts[2]);
          const keycode = parseInt(parts[4]);
          const modifiers = parseInt(parts[5]);

          if (!config.layers[layer].macros[button].keySequence) {
            config.layers[layer].macros[button].keySequence = [];
          }

          config.layers[layer].macros[button].keySequence!.push({
            keycode,
            modifiers,
          });
          continue;
        }

        // MACRO|layer|button|type|value|string|name|emoji[|platform|size]
        if (line.startsWith("MACRO|")) {
          const parts = line.split("|");
          if (parts.length >= 8) {
            const layer = parseInt(parts[1]);
            const button = parseInt(parts[2]);
            const type = parseInt(parts[3]) as MacroType;
            const value = parseInt(parts[4]);
            const macroString = parts[5];
            const name = parts[6];
            const emojiIndex = parseInt(parts[7]);
            const emoji = emoji_strings[emojiIndex] || "🎮";

            const macro: MacroEntry = {
              type,
              value,
              macroString,
              name,
              emoji,
            };

            // if SCRIPT type, next line will be SCRIPT_DATA|
            if (type === 3 && parts.length >= 10) {
              macro.scriptPlatform = parseInt(parts[8]);
              const scriptSize = parseInt(parts[9]);
              pendingScriptMacro = { layer, button };
              console.log(
                `  ➜ Expecting script (${scriptSize} bytes) on next line`,
              );
            }

            config.layers[layer].macros[button] = macro;
            console.log(`  ➜ Button ${button}: ${name} (type ${type})`);
          }
          continue;
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
   * sets single macro
   */
  async setMacro(
    layer: number,
    button: number,
    type: MacroType,
    value: number,
    macroString: string,
    name: string,
    emoji: string,
    keySequence?: KeyPress[],
  ): Promise<void> {
    const emojiIndex = emoji_strings.indexOf(emoji);
    console.log(
      `📤 Setting macro L${layer}B${button}: ${emoji_strings[emojiIndex]} ${name}`,
    );

    if (
      type === MacroType.KEY_SEQUENCE &&
      keySequence &&
      keySequence.length > 0
    ) {
      const steps = keySequence.slice(0, 5);
      const stepsStr = steps
        .map((s) => `${s.keycode},${s.modifiers}`)
        .join(",");
      const command = `SET_MACRO_SEQ|${layer}|${button}|${name}|${emojiIndex}|${steps.length}|${stepsStr}`;

      console.log(`  Sending sequence with ${steps.length} steps`);
      const response = await this.sendCommand(command);

      if (response !== "OK") {
        throw new Error(`Failed to set key sequence: ${response}`);
      }

      console.log("✅ Macro set successfully");
      return;
    }

    const command = `SET_MACRO|${layer}|${button}|${type}|${value}|${macroString}|${name}|${emojiIndex}`;
    const response = await this.sendCommand(command);

    if (response !== "OK") {
      throw new Error(`Failed to set macro: ${response}`);
    }

    console.log("✅ Macro set successfully");
  }

  /**
   * sets layer name
   */
  async setLayerName(
    layer: number,
    name: string,
    emoji: string,
  ): Promise<void> {
    const emojiIndex = emoji_strings.indexOf(emoji);
    console.log(`📤 Setting layer ${layer} name: ${emoji} ${name}`);

    const command = `SET_LAYER_NAME|${layer}|${name}|${emojiIndex}`;
    const response = await this.sendCommand(command);

    if (response !== "OK") {
      throw new Error(`Failed to set layer name: ${response}`);
    }

    console.log("✅ Layer name set successfully");
  }

  /**
   * saves configuration to flash
   */
  async saveFlash(): Promise<void> {
    console.log("📤 Sending SAVE_FLASH command...");
    const response = await this.sendCommand("SAVE_FLASH");
    console.log("📥 Received response:", response);

    if (!response.includes("OK")) {
      console.error("❌ Unexpected response:", response);
      throw new Error(`SAVE_FLASH failed: ${response}`);
    }

    console.log("✅ SAVE_FLASH successful");

    // przeladuj config w firmware
    console.log("📤 Sending RELOAD_CONFIG command...");
    const reloadResponse = await this.sendCommand("RELOAD_CONFIG");
    if (!reloadResponse.includes("OK")) {
      console.warn("⚠️ RELOAD_CONFIG failed, but save was successful");
    } else {
      console.log("✅ Config reloaded in firmware");
    }
  }

  /**
   * helper delay function
   */
  private delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }
}
