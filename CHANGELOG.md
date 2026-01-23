# CHANGELOG
Projekt: Talos 7
Autor: Szymon Wilczek

## [2026-01-22]
- docs: fix links for preview
- docs: update README formatting
- docs: update README and add board assets
- docs: fix video for atari preview
- docs: update grid photos
- docs: fix position of video preview
- docs: adjust preview layout in README
- docs: add project preview media to README
- chore(firmware): Hephaestus forged v2.8.0 [skip ci]
- feat(web): support Atari Breakout game macro
- feat(firmware): add configurable Atari Breakout easter egg

## [2026-01-21]
- chore(firmware): Hephaestus forged v2.7.0 [skip ci]
- feat(web): send script metadata and flush serial buffer
- feat(firmware): parse script metadata and improve saving sync
- fix(firmware): increase typing delay to 5ms for reliability
- chore(firmware): Hephaestus forged v2.6.3 [skip ci]
- fix(firmware): prevent off-by-one buffer overflow in script reception
- feat(web): update oled slider step to 10s
- chore(firmware): Hephaestus forged v2.6.2 [skip ci]
- fix(firmware): flush rx buffer before script reception

## [2026-01-18]
- chore(firmware): Hephaestus forged v2.6.1 [skip ci]
- fix: updated OS colors

## [2026-01-11]
- fix: udev rules for linux troubleshooting

## [2025-12-14]
- feat: complex tests for macro_config
- feat: readme for tests
- feat: main unit tests runner
- feat: unit tests for crc32, flash storage
- feat: unit tests for hardware interface
- feat: unit tests for midi actions
- feat: unit tests for cdc dispatcher
- feat: unity framework for unit tests
- feat: new mocks for testing
- feat: main tests configuration
- chore(ci): exclude tests folder from release triggers
- feat: bumped next version
- chore(firmware): Hephaestus forged v2.6.0 [skip ci]
- feat: add retry mechanism to serial communication
- fix: prevent script data from being parsed as commands
- feat: increase timeout and feed during flash operations
- fix: add layer index validation in display function
- fix: fix unused macro_count variable in GET_CONF
- fix: remove duplicate function declaration
- fix: replace silent return with continue in handleSaveChanges
- fix: add missing dependency to useEffect in configurator
- fix: add buffer size limit to prevent memory leak in readLine
- fix: add NaN validation for parseInt in config parsing
- fix: increase response format buffer size
- fix: add explicit sequence_length reset in factory defaults
- fix: remove duplicate boot log messages
- fix: remove unnecessary flash write on boot
- fix: add error handling to serial disconnect
- fix: apply MIDI fallback values consistent with firmware
- fix: add input validation to cdc_receive_sequence
- fix: prevent buffer overflow in text preview

## [2025-12-01]
- chore(firmware): Hephaestus forged v2.5.0 [skip ci]
- feat: screen saver manager for managing animations
- feat: new animations for screen saver
- feat: new graphics for screen saver animations
- feat: moved oled display to proper directory
- fix: matrix rain oled screen saver

## [2025-11-30]
- feat: web interface vercel push
- Merge branch 'main' of https://github.com/szymonwilczek/talos-7
- feat: afk jiggler for mouse move action in WI
- chore(firmware): Hephaestus forged v2.4.1 [skip ci]
- fix: feeding watchdog in macro actions

## [2025-11-28]
- feat: electron release push
- fix: permissions for electron release
- feat: electron wrapper for Talos configurator
- feat: electron automatic workflow
- chore(firmware): Hephaestus forged v2.4.0 [skip ci]
- feat: hardware watchdog for system stability

## [2025-11-27]
- chore(firmware): Hephaestus forged v2.3.0 [skip ci]
- feat: rgb led os indicator
- chore(firmware): Hephaestus forged v2.2.0 [skip ci]
- feat: possibility of changing platform via external button
- chore(firmware): Hephaestus forged v2.1.1 [skip ci]
- fix: proper windows unicode recognition and typing
- fix: removed usbProductId from port filters

## [2025-11-26]
- chore(firmware): Hephaestus forged v2.1.0 [skip ci]
- feat: windows script execution in powershell file
- Fix LED resistor notation in README
- Revise BOM with additional details and clarifications

## [2025-11-25]
- Update compatibility information in README
- Add thank you note to README
- feat: connection pannel error update (WI)
- Merge branch 'main' of https://github.com/szymonwilczek/talos-7
- fix: missing paragraph for BROWSER_NOT_SUPPORTED error
- feat: ignoring firmware updates for webpage
- chore(firmware): Hephaestus forged v2.0.0 [skip ci]
- feat!: complete firmware for windows & linux descriptors
- fix: filters in serial.transport.ts
- reverted to original descriptors configuration
- chore(firmware): Hephaestus forged v1.5.6 [skip ci]
- fix: forgotten definitions for IAD protocol
- chore(firmware): Hephaestus forged v1.5.5 [skip ci]

## [2025-11-23]
- feat: MIDI CC funcionality
- chore(firmware): Hephaestus forged v1.2.0 [skip ci]
- feat: MIDI notes functionality
- chore(firmware): Hephaestus forged v1.1.2 [skip ci]
- fix: assets graphic comments in english
- fix: testing automatic workflow
- Merge branch 'main' of https://github.com/szymonwilczek/talos-7
- feat: hephaestus workflow now increments all version numbers
- chore(firmware): Hephaestus forged patch version bump [skip ci]
- feat: workflow for automatic bumping up versions
- feat: included firmware version before sending config
- feat: uploading firmware not possible via web interface
- feat: usb descriptors registering with proper author and project name
- feat: importing/exporting talos configs
- fix: preview badge in oled display (web interface)
- feat: new favicon and project logo
- feat: documentation for oled display
- feat: boot up logo for oled display (firmware)

## [2025-11-22]
- fix: missingimport for macro executor(s)
- feat: excluded logic from executor single file to separate handlers
- fix: imports of new cdc logic
- feat: excluded logic from cdc_protocol to separate handlers
- fix: header definition of format_sequence helper
- feat: oled_display in targets
- fix: missing imports of oled_display
- feat: excluded emoji from oled_display
- feat: excluded font from hardware interface
- feat: excluded oled display logic from hardware interface
- fix: proper assembling terminal shortcut for scripts

## [2025-11-21]
- feat: oled (F/WI) proper display of new mouse macros
- feat: centering layer & button name for oled display (WI)
- feat: new test script
- feat: smoothing out mouse move process
- feat: implementing interrupting spam clicking
- fix: hybrid mouse + keyboard tud hid
- fix: mouse descriptor for usb
- feat: turbo typing for ascii text strings
- feat: fast typing on key pressing
- feat: specifing duration and repeat count on key presses
- feat: waking up screen when plugged in into interface
- fix: updating oled display timeout via web interface
- feat: preventing oled burnout by settings afk timeout
- feat: slider for web interface
- feat: unique id for pico when plugged in

## [2025-11-20]
- fix: displaying sequences on oled display (firmware)
- fix: proper formatting and sending key sequences (WI to F)
- fix: sending full script from firmware to web interface
- feat: executing macro scripts by firmware
- feat: validating for only one layer toggle on layer
- feat: implemented unicode/ascii display in oled-display (WI)

## [2025-11-19]
- fix: waiting for tud HID to be ready after finishing typing
- feat: proper displaying TEXT_STRING macro on oled (hardware)
- feat: dynamic platform (os) detecting
- feat: macro executing for text_strings
- feat: mapping chars for HID in hardware interface
- fix: proper parsing emoji for key sequence
- feat: implemented vim-like sequence for oled display (WI)
- fix: typo
- feat: formatting modificators on oled (firmware) like vim
- feat: new limiters for sequence and button name
- feat: displaying button name on oled during click
- feat: centering buttons in firmware for oled
- feat: new OLEDDisplay look (immersive) (web interface)
- fix: emotes in serial service (web interface)
- feat: new font for oled display (web interface)
- feat: defined const for max_emojis in firmware
- fix: updated default layer emojis to indexes instead of emoticons
- feat: removed hardcoded waiting for cdc connection
- feat: 3 new emojis
- feat: changed max length of layer name to 15
- feat: displaying layer name if provided in config (oled firmware)
- feat: displaying special screen when pending changes
- feat: reloading oled display when changes are saved
- feat: specified new emoji format in web interface

## [2025-11-18]
- feat: changed suggested emojis
- fix: missing import
- feat: implemented emojis into protocol and hardware interface
- feat: emojis on oled display handled by firmware
- fix: oled display layout in hardware interface
- feat: implemented hardware interface into main
- fix: proper oled attributes for mocking hardware
- fix: pins definitions
- feat: implemented macro executor for firmware
- feat: implemented hardware interface for firmware
- feat: implemented logging function in cdc protocol
- feat: included hardware spi configuration for oled display
- feat: implemented logs for hardware debugging
- feat: improved oled display for showing actual button macro
- feat: defined new attributes for key sequences
- feat: implemented new commands into serial service
- feat: new component for key sequences macros
- feat: implemented key sequences in button edit
- feat: new commands for key sequences macros
- feat: test script for scripting macros
- feat: implemented receiving key sequences for macros
- feat: implemented key sequences for macros
- feat: border for oled display

## [2025-11-16]
- feat: removed unused components
- feat: animating saving progress & success/error alert
- fix: proper handling pendingChanges state between saves
- feat: scripts handling (custom shell/bash/...etc.)

## [2025-11-15]
- feat: realistic buttons on web-interface
- feat: formatting improvements on web-interface
- feat: next-themes for theme provider
- feat: better visualisation of project board layout on web interface
- feat: removed unused macro slot component
- feat: added hid keymap dialog cheatsheet
- feat: handling macro/layers emojis on frontend site
- feat: new shadcn components for web-interface
- feat: emojis for layers
- feat: main component for macro configurations
- feat: main serial service for whole pico operations handling
- feat: defined macro types in web-interface
- feat: utils for comparing config (web-interface)
- feat: tailwind utils for web-interface
- feat: added bunch of shadcn components for design
- feat: components for macro configuration (web-interface)
- feat: barebone skeleton of web-interface
- feat: ingoring pico-sdk in git commits
- feat: cmake list to speed up building process
- feat: main file for firmware configuration for pi pico
- feat: implemented usb descriptors
- feat: implemented mocking hardware for now
- feat: implemented macro config
- feat: implemented cdc_protocol
- feat: defined cdc protocol header
- feat: imported pico sdk import cmake library
- feat: mocking hardware for now
- feat: defined tusb config
- feat: defined pin definitions for pi pico
- feat: defined macro config for pi pico
- feat: ignoring build directory
- Initial commit