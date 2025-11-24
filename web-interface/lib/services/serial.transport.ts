export class SerialTransport {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private readBuffer: string = "";

  static isSupported(): boolean {
    return typeof navigator !== "undefined" && "serial" in navigator;
  }

  isConnected(): boolean {
    return this.port !== null && this.reader !== null && this.writer !== null;
  }

  async connect(baudRate: number = 115200): Promise<void> {
    if (!SerialTransport.isSupported())
      throw new Error("Web Serial API not supported");

    this.port = await navigator.serial.requestPort({
      filters: [{ usbVendorId: 0x2e8a, usbProductId: 0x000a }],
    });

    await this.port.open({ baudRate });

    if (this.port.readable) this.reader = this.port.readable.getReader();
    if (this.port.writable) this.writer = this.port.writable.getWriter();
  }

  async disconnect(): Promise<void> {
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
  }

  async writeLine(text: string): Promise<void> {
    if (!this.writer) throw new Error("Not connected");
    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(text + "\n"));
  }

  async writeRaw(text: string): Promise<void> {
    if (!this.writer) throw new Error("Not connected");
    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(text));
  }

  async writeBytes(bytes: Uint8Array): Promise<void> {
    if (!this.writer) throw new Error("Not connected");
    await this.writer.write(bytes);
  }

  async readLine(timeout: number = 5000): Promise<string> {
    if (!this.reader) throw new Error("Not connected");
    const startTime = Date.now();

    while (true) {
      if (Date.now() - startTime > timeout) throw new Error("Read timeout");

      const newlineIndex = this.readBuffer.indexOf("\n");
      if (newlineIndex !== -1) {
        const line = this.readBuffer.substring(0, newlineIndex).trim();
        this.readBuffer = this.readBuffer.substring(newlineIndex + 1);
        return line;
      }

      const { value, done } = await this.reader.read();
      if (done) throw new Error("Stream closed");

      const decoder = new TextDecoder();
      this.readBuffer += decoder.decode(value);
    }
  }
}
