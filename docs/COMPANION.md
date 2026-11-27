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

Physical buttons `[1..3]` sit behind the OLED without keycaps, so all
Companion interactions live on `[4]`, `[5]`, `[6]` and `[7] esc`:

| Submode      | `[4]` (Left)                | `[5]` (Middle)                   | `[6]` (Right)             |
| :----------- | :-------------------------- | :------------------------------- | :------------------------ |
| **HOME**     | Pet                         | Coffee                           | Sleep OLED                |
| **POMODORO** | Start/Pause _(hold: Reset)_ | Skip Phase _(hold: Deep Focus)_  | +5 min _(hold: −5 min)_   |
| **MEDIA**    | Vol− _(hold: Prev Track)_   | Play/Pause _(hold: System Mute)_ | Vol+ _(hold: Next Track)_ |
| **VOICE**    | Mute Mic (`F13`)            | Deafen (`F14`)                   | BRB _(Deafen + Coffee)_   |

Long-hold `[7] esc` (1.5s) exits Companion Mode back to normal macropad layers.
In Breakout, `[4]`/`[6]` move the paddle and `[7] esc` exits cleanly.

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

Talos sends USB HID keycodes for Mute Mic (`F13`) and Deafen (`F14`). Because
`xdg-desktop-portal-wlr` on Sway does not expose the `GlobalShortcuts` portal
interface, bind them directly to PipeWire via `wpctl` in `~/.config/sway/config`:

```sway
bindsym F13 exec wpctl set-mute @DEFAULT_AUDIO_SOURCE@ toggle
bindsym F14 exec wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle
```

This toggles system mic/output cleanly in Vesktop and every audio app. The
bridge monitors PipeWire `pactl subscribe` events (`source-outputs`, default
source mute, and default sink volume/mute), keeping Companion faces and LEDs in
sync:

- **LED #1 ON:** Mic muted
- **LED #1 + #2 ON:** Deafened (muted headphones + mic)

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
