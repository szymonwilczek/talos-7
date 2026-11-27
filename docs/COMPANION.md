# Companion Mode

Companion Mode turns Talos 7 into a desktop pet that reflects your Fedora/Sway
desktop state on its dual-color 128x64 OLED and 7 red LEDs via a one-way CDC
bridge.

## Architecture

**Key guarantee:** Communication is strictly one-way (`Host -> Talos`). The
bridge never reads from the serial port, never executes commands from Talos, and
never touches `systemd` suspend, screen lock, or any host power state.

## Building the Host Bridge

```bash
cd host-bridge
cmake -S . -B build
cmake --build build -j4

# Install binaries (optional)
sudo install build/talos-bridge build/talos-evt /usr/local/bin/

# Install systemd user service
mkdir -p ~/.config/systemd/user
cp systemd/talos-bridge.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now talos-bridge
```

### CLI Usage

```bash
# Send events from scripts, git hooks, or AI agent wrappers
talos-evt agent-start "Building firmware"
talos-evt agent-done "Build Green"
talos-evt agent-err "Compile failed"
talos-evt agent-input "Needs approval"
talos-evt git-commit "feat: add parser"
talos-evt typing
talos-evt ping "Hello Mochi"
talos-evt pet
talos-evt coffee

# Pause bridge so web configurator can claim the serial port
talos-evt pause
# Resume after web configurator disconnects
talos-evt resume
```

Use `--quiet` (`-q`) to suppress errors when the daemon isn't running (useful
in hooks that should never block).

## Emacs Integration

Add to your Emacs config:

```elisp
(add-to-list 'load-path "/path/to/talos-7/emacs")
(require 'talos-companion)
(talos-companion-mode 1)
```

**What it reports:**

- Buffer name + major-mode on window/buffer switch
- Throttled typing pulses (>= 3s apart, configurable)
- Magit commit events via `magit-post-commit-hook`

**What it deliberately omits:**

- `after-save-hook` - Evil `:w` saves frequently; no OLED spam
- `compilation-finish-functions` - builds run via agents/CI, not Emacs

## Firmware Controls

### Submodes (cycled with `[7] esc` short press)

| Submode      | Keys 1–6                                                        |
| :----------- | :-------------------------------------------------------------- |
| **HOME**     | Pet · Coffee · Start Pomodoro · Mute Mic · Play/Pause · Sleep   |
| **POMODORO** | Start/Pause · Reset · Skip Phase · −5 min · +5 min · Deep Focus |
| **MEDIA**    | Prev · Play/Pause · Next · Vol- · Mute · Vol+                   |
| **VOICE**    | Mute Mic · Deafen · Play/Pause · Vol− · Vol+ · BRB              |

Long-hold `[7] esc` (1.5s) exits Companion Mode back to normal macropad layers.

### Sleep & Wake

- **Manual sleep:** `[6]` in HOME submode
- **Auto-sleep:** 30 minutes of true idle (inhibited by active media/voice)
- **Late night:** `01:00–04:00` flag dims the HUD
- **OLED off:** Prolonged sleep sends `DISPLAY_OFF`; first button press or new
  host activity wakes the display (first press is consumed, not forwarded)

### Autostart

Enable "Start in Companion Mode on boot" in the web configurator under
Device Settings. The preference is persisted in flash past the CRC32 guard.

## Vesktop / Discord on Sway

Talos sends USB HID keycodes for Mute Mic (`F13`) and Deafen (`F14`). On
Sway, bind these as global shortcuts via `xdg-desktop-portal`:

1. Open Vesktop -> Settings -> Keybinds
2. Set "Toggle Mute" to `F13` and "Toggle Deafen" to `F14`
3. Vesktop on Wayland uses `xdg-desktop-portal` GlobalShortcuts automatically

The bridge monitors PipeWire for active Vesktop voice streams and
`pactl get-source-mute` for microphone state, updating LEDs accordingly:

- **LED #1 ON:** Mic muted
- **LED #1 + #2 ON:** Deafened

## Socket Protocol

The Unix domain socket at `$XDG_RUNTIME_DIR/talos-bridge.sock` accepts
newline-terminated text commands:

| Command                       | Fields | Effect                      |
| :---------------------------- | :----- | :-------------------------- |
| `emacs-buf\|<buffer>\|<mode>` | 3      | Update Emacs context        |
| `typing`                      | 1      | Throttled typing pulse      |
| `agent-start\|<task>`         | 2      | Enter AI thinking state     |
| `agent-done\|<label>`         | 2      | Flash success event         |
| `agent-err\|<label>`          | 2      | Flash error event           |
| `agent-input\|<label>`        | 2      | Flash needs-input event     |
| `git-commit\|<msg>`           | 2      | Flash commit event          |
| `ping\|<text>`                | 2      | Friendly ping               |
| `pet` / `coffee`              | 1      | Pet or coffee animation     |
| `pause` / `resume`            | 1      | Release/reclaim serial port |
| `idle-30m` / `idle-resume`    | 1      | Force idle sleep/wake       |
