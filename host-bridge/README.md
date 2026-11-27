# Talos 7 Host Bridge (C11)

Lightweight one-way Fedora / Sway context bridge for Talos 7 Companion Mode.

## Design Guarantees

- **Pure C11 + POSIX libc**: zero external `-dev` libraries required.
- **Strictly one-way (`Host -> Talos`)**: `/dev/ttyACM*` is opened with `O_WRONLY`
  (`VID:PID = 2e8a:0032`). The bridge never reads or executes anything from USB.
- **Zero host power control**: never triggers `systemd` suspend or session lock.
- **Diff-only updates**: caches last transmitted state/time frames and releases
  the serial port immediately on `pause` or `SIGTERM` so the web configurator
  can connect over Web Serial.

## Build

```bash
cmake -S . -B build
cmake --build build -j4
```
