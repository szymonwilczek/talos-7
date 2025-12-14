import {
  GlobalConfig,
  MacroEntry,
  MacroType,
  ScriptPlatform,
  KeyPress,
  FIRMWARE_CONSTANTS,
  DEFAULT_LAYER_EMOJIS,
} from "../types/config.types";
import { SerialTransport } from "./serial.transport";
import { SerialProtocol } from "./serial.protocol";
import {
  getEmojiString,
  compileKeySequence,
  MAX_SCRIPT_SIZE,
} from "./serial.utils";

export class SerialService {
  private transport: SerialTransport;

  constructor() {
    this.transport = new SerialTransport();
  }

  /**
   * Check if Web Serial API is supported
   */
  static isSupported(): boolean {
    return SerialTransport.isSupported();
  }

  /**
   * Establishes connection to the Pico device
   */
  async connectPico(): Promise<void> {
    try {
      await this.transport.connect();
      await this.delay(1000); // connection stabilization delay
      console.log("✅ Connection stabilized");
    } catch (error) {
      throw new Error(`Failed to connect: ${error}`);
    }
  }

  /**
   * Disconnects from the device
   */
  async disconnect(): Promise<void> {
    await this.transport.disconnect();
  }

  /**
   * Reads the entire configuration from the device
   */
  async readConfig(): Promise<GlobalConfig> {
    console.log("📤 Sending GET_CONF...");
    await this.transport.writeLine("GET_CONF");

    const startResponse = await this.transport.readLine();
    if (startResponse !== "CONF_START") {
      throw new Error(`Expected CONF_START, got: ${startResponse}`);
    }

    const config = this.createEmptyConfig();

    console.log("📖 Parsing configuration lines...");
    let pendingScriptMacro: { layer: number; button: number } | null = null;
    let lineCount = 0;

    while (true) {
      const line = await this.transport.readLine(10000);
      lineCount++;

      if (line === "CONF_END") break;

      try {
        if (line.startsWith("VERSION|")) {
          config.firmwareVersion = line.split("|")[1];
        } else if (line.startsWith("SETTINGS|")) {
          config.oledTimeout = parseInt(line.split("|")[1]);
        } else if (line.startsWith("LAYER_NAME|")) {
          this.parseLayerName(line, config);
        } else if (line.startsWith("MACRO|")) {
          const scriptTarget = this.parseMacroLine(line, config);
          if (scriptTarget) pendingScriptMacro = scriptTarget;
        } else if (line.startsWith("MACRO_SEQ|")) {
          this.parseMacroSeqHeader(line, config);
        } else if (line.startsWith("SEQ_STEP|")) {
          this.parseSeqStep(line, config);
        } else if (line.startsWith("SCRIPT_SHORTCUT|")) {
          this.parseScriptShortcut(line, config);
        } else if (line.startsWith("SCRIPT_DATA|")) {
          this.parseScriptData(line, config, pendingScriptMacro);
          pendingScriptMacro = null;
        }
      } catch (err) {
        console.warn(`⚠️ Error parsing line ${lineCount}: ${line}`, err);
      }
    }

    console.log("✅ Configuration loaded successfully");
    return config;
  }

  /**
   * Sets a macro on the device
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
    repeatCount: number = 1,
    repeatInterval: number = 0,
    moveX: number = 0,
    moveY: number = 0,
  ): Promise<void> {
    if (
      type === MacroType.KEY_SEQUENCE &&
      keySequence &&
      keySequence.length > 0
    ) {
      console.log(`📤 Setting Key Sequence L${layer}B${button}`);
      const command = SerialProtocol.buildSequenceCommand(
        layer,
        button,
        name,
        emoji,
        keySequence,
      );
      await this.sendCommandCheckOK(command);
      return;
    }

    console.log(`📤 Setting Macro L${layer}B${button}: ${name}`);
    const command = SerialProtocol.buildSetMacroCommand(
      layer,
      button,
      type,
      value,
      macroString,
      name,
      emoji,
      repeatCount,
      repeatInterval,
      moveX,
      moveY,
    );
    await this.sendCommandCheckOK(command);
  }

  /**
   * Sets a script macro on the device
   */
  async setScript(
    layer: number,
    button: number,
    platform: ScriptPlatform,
    scriptContent: string,
    terminalShortcut: KeyPress[],
  ): Promise<void> {
    // size validation
    const encoder = new TextEncoder();
    const scriptBytes = encoder.encode(scriptContent);
    if (scriptBytes.length > MAX_SCRIPT_SIZE) {
      throw new Error(
        `Script too large (${scriptBytes.length} > ${MAX_SCRIPT_SIZE} bytes)`,
      );
    }

    // keysequence compilation
    const compiledSteps = compileKeySequence(terminalShortcut);
    const shortcutStr = compiledSteps
      .map((s) => `${s.keycode},${s.modifiers}`)
      .join(",");

    // sending header
    const command = `SET_MACRO_SCRIPT|${layer}|${button}|${platform}|${scriptBytes.length}|${compiledSteps.length}|${shortcutStr}\n`;
    console.log(`📤 Sending script header...`);
    await this.transport.writeRaw(command);

    // waiting for READY
    const readyResponse = await this.transport.readLine(2000);
    if (!readyResponse.startsWith("READY")) {
      throw new Error(`Device not ready for script. Got: ${readyResponse}`);
    }

    // sending script content
    await this.transport.writeBytes(scriptBytes);

    // waiting for OK
    const response = await this.transport.readLine(5000);
    if (!response.startsWith("OK")) {
      throw new Error(`Script upload failed: ${response}`);
    }
    console.log("✅ Script uploaded successfully");
  }

  async setLayerName(
    layer: number,
    name: string,
    emoji: string,
  ): Promise<void> {
    console.log(`📤 Setting layer ${layer} name: ${emoji} ${name}`);
    const command = SerialProtocol.buildLayerNameCommand(layer, name, emoji);
    await this.sendCommandCheckOK(command);
  }

  async setOledTimeout(seconds: number): Promise<void> {
    console.log(`📤 Setting OLED timeout: ${seconds}s`);
    await this.sendCommandCheckOK(`SET_OLED_TIMEOUT|${seconds}`);
  }

  async saveFlash(): Promise<void> {
    console.log("📤 Saving to Flash...");
    await this.sendCommandCheckOK("SAVE_FLASH");

    console.log("🔄 Reloading config...");
    try {
      await this.transport.writeLine("RELOAD_CONFIG");
    } catch (e) {
      console.warn(
        "⚠️ Error calling RELOAD_CONFIG (device might be busy), but Flash saved.",
      );
    }
  }

  async setConfigMode(mode: number): Promise<void> {
    await this.sendCommandCheckOK(`SET_CONFIG_MODE|${mode}`);
  }

  async enterBootloader(): Promise<void> {
    console.log("📤 Sending BOOTSEL command...");
    try {
      await this.transport.writeLine("BOOTSEL");
      await this.delay(500);
    } catch (e) {
      // expected erorr on disconnect
      console.log("Device reset initiated.");
    }
    await this.disconnect();
  }

  private delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  /**
   * Helper: sends a command and checks for "OK" response
   */
  private async sendCommandCheckOK(command: string): Promise<void> {
    await this.transport.writeLine(command);
    const response = await this.transport.readLine();
    if (!response.includes("OK")) {
      throw new Error(`Command failed: ${response}`);
    }
  }

  private createEmptyConfig(): GlobalConfig {
    return {
      layers: Array.from({ length: FIRMWARE_CONSTANTS.MAX_LAYERS }, (_, i) => ({
        name: `Layer ${i + 1}`,
        emoji: getEmojiString(DEFAULT_LAYER_EMOJIS[i] ?? 0),
        macros: Array.from({ length: FIRMWARE_CONSTANTS.NUM_BUTTONS }, () => ({
          type: MacroType.KEY_PRESS,
          value: 0,
          macroString: "",
          name: "Empty",
          emoji: "",
        })),
      })),
      oledTimeout: 300,
      firmwareVersion: "unknown",
    };
  }

  private parseLayerName(line: string, config: GlobalConfig) {
    const parts = line.split("|");
    const layerIdx = parseInt(parts[1]);
    const name = parts[2] || `Layer ${layerIdx + 1}`;
    const emojiIdx = parseInt(parts[3]);

    if (config.layers[layerIdx]) {
      config.layers[layerIdx].name = name;
      config.layers[layerIdx].emoji = getEmojiString(emojiIdx);
    }
  }

  private parseMacroLine(
    line: string,
    config: GlobalConfig,
  ): { layer: number; button: number } | null {
    // MACRO|layer|button|type|value|string|name|emojiIdx|...
    const parts = line.split("|");
    if (parts.length < 8) return null;

    const layer = parseInt(parts[1]);
    const button = parseInt(parts[2]);
    const type = parseInt(parts[3]) as MacroType;

    const macro: MacroEntry = {
      type,
      value: parseInt(parts[4]),
      macroString: parts[5],
      name: parts[6],
      emoji: getEmojiString(parseInt(parts[7])),
      repeatCount: parseInt(parts[8]) || 1,
      repeatInterval: parseInt(parts[9]) || 0,
      moveX: parseInt(parts[10]) || 0,
      moveY: parseInt(parts[11]) || 0,
    };

    // specific mappings for MIDI
    if (type === MacroType.MIDI_NOTE) {
      macro.midiNote = macro.value;
      macro.midiVelocity = macro.moveX && macro.moveX > 0 ? macro.moveX : 127;
      macro.midiChannel = macro.moveY && macro.moveY > 0 ? macro.moveY : 1;
    } else if (type === MacroType.MIDI_CC) {
      macro.midiCCNumber = macro.value;
      macro.midiCCValue = macro.moveX && macro.moveX >= 0 ? macro.moveX : 127;
      macro.midiChannel = macro.moveY && macro.moveY > 0 ? macro.moveY : 1;
    }

    if (type === MacroType.SCRIPT && parts.length >= 9) {
      macro.scriptPlatform = parseInt(parts[8]);
      config.layers[layer].macros[button] = macro;
      return { layer, button };
    }

    config.layers[layer].macros[button] = macro;
    return null;
  }

  private parseMacroSeqHeader(line: string, config: GlobalConfig) {
    const parts = line.split("|");
    const layer = parseInt(parts[1]);
    const button = parseInt(parts[2]);

    config.layers[layer].macros[button] = {
      type: MacroType.KEY_SEQUENCE,
      value: 0,
      macroString: "",
      name: parts[3],
      emoji: getEmojiString(parseInt(parts[4])),
      keySequence: [],
    };
  }

  private parseSeqStep(line: string, config: GlobalConfig) {
    const parts = line.split("|");
    const layer = parseInt(parts[1]);
    const button = parseInt(parts[2]);

    config.layers[layer].macros[button].keySequence?.push({
      keycode: parseInt(parts[4]),
      modifiers: parseInt(parts[5]),
    });
  }

  private parseScriptShortcut(line: string, config: GlobalConfig) {
    const parts = line.split("|");
    const layer = parseInt(parts[1]);
    const button = parseInt(parts[2]);

    if (!config.layers[layer].macros[button].terminalShortcut) {
      config.layers[layer].macros[button].terminalShortcut = [];
    }
    config.layers[layer].macros[button].terminalShortcut!.push({
      keycode: parseInt(parts[4]),
      modifiers: parseInt(parts[5]),
    });
  }

  private parseScriptData(
    line: string,
    config: GlobalConfig,
    pending: { layer: number; button: number } | null,
  ) {
    if (!pending) return;

    // SCRIPT_DATA|layer|button|platform|CONTENT...
    let pipeCount = 0;
    let contentIndex = 0;
    for (let i = 0; i < line.length; i++) {
      if (line[i] === "|") {
        pipeCount++;
        if (pipeCount === 4) {
          contentIndex = i + 1;
          break;
        }
      }
    }

    const content = line
      .substring(contentIndex)
      .replace(/\\n/g, "\n")
      .replace(/\\r/g, "\r")
      .replace(/\\\|/g, "|")
      .replace(/\\\\/g, "\\");

    config.layers[pending.layer].macros[pending.button].script = content;
  }
}
