# TRAM8 Module 4x4 Firmware

Go to SysEx configurator: https://c0ff.github.io/TRAM8-4x4/tram8_4x4_fw7.html

## Features:
 - Offline HTML-based configuration editor
 - Configurable global MIDI channel
 - Optional per-output MIDI channel selection
 - Global CV range selection: 0-5V or 0-8V
 - Per-gate output modes: Gate or Trigger
 - Configurable global trigger length (1 - 120 ms)
 - Gate outputs can work as square LFOs
 - Global Clock Divider Scale. Allows to set clock dividers in multiples of MIDI pulses.
 - Universal setting for Gate and CV outputs. For continuous sources gates output high value when source >= 64.
 - Sources: Note On, After Touch, Control Change, Channel Pressure, Clock (MIDI Clock divider, RUN gate, Reset trigger).
 - Note output modes: Trigger, Gate, Velocity, Dynamic Gate (Gate x Velocity)
 - Supports All Sound Off and All Notes Off messages
 - Global option to reset clock dividers on MIDI Continue command (enabled by default)

## Known issues:
 - **No learn mode at the moment**


Based on Stock Firmware 1.3

Timer code for 1ms ticks from https://github.com/thorinf/tram8-plus/


## To use:
 - Download and open locally `FW-4x4 HTML FILES/tram8_4x4_fw7.html`
 - Edit the settings to your liking and get the `tram8_4x4_fw7_edit.syx`
 - Send the generated .syx file to your Tram8 module in the update mode
 - Enjoy!


## Why 4x4

4x4 comes from the initial idea:
- 4 trigger outputs for notes 36, 38, 42, 46 which correspond to TR-8S BD, SD, CH, OH
- 4 velocity CV outputs for these notes, 0-8V
- 4 clock outputs: beat clock (1 ppqn)), bar clock (4 beats), Run gate, Reset trigger
- 4 CC to 0-8V CV for controllers 24, 29, 63, 82 which correspond to TR-8S BD, SD, CH, OH levels

But now it's just because it can ride everywhere.

Firmware supports both gate and trigger outputs with custom ppqn, CC, and Note support.

An offline HTML-based SysEx editor is available.
The editor is heavily inspired by http://mutantbrainsurgery.hexinverter.net
but mine is completely offline - one standalone HTML page with no dependencies.


## Whom-how

Hacked (in the old school meaning, i.e. creating something quickly just for fun) by Dmitry Baikov.
The editor was created from scratch in a long one-day 16-hour session. It was fun!

The history and some details of the implemenation are documented in `FourByFour/HTML-UI/README.md`.

Built on MacOSX using avr-gcc and CMake.

Tested with Squarp Hapax and Nano modules Octa


## Note about the editor.
The editor expects you to load and modify a SysEx file. After you made your edits, download a new SysEx
and update the module as usual. It is **NOT** a live MIDI editor.

Starting from editor **1 rev.A** the default 4x4 firmware is embedded into HTML and preloaded automatically.


## Default configuration:

### Global:
 - MIDI Channel: 10
 - Trigger length: 10ms
 - CV range: 0-8V
 - Reset on MIDI Continue: Yes
 - Clock Divider Scale: 1/4 note (1 ppqn)
 
### Gates:
 1. Trigger on NoteOn 36 // TR-8S BD
 2. Trigger on NoteOn 38 // TR-8S SD
 3. Trigger on NoteOn 42 // TR-8S CH
 4. Trigger on NoteOn 46 // TR-8S OH
 5. Trigger on NoteOn 39 // TR-8S BD ALT
 6. Trigger on NoteOn 35 // TR-8S SD ALT
 7. Trigger on NoteOn 40 // TR-8S CP
 8. Trigger on quarter notes (24 MIDI pulses) // beat clock

### Voltages:
 1. Velocity of Note 36 // TR-8S BD
 2. Velocity of Note 38 // TR-8S SD
 3. Velocity of Note 42 // TR-8S CH
 4. Velocity of Note 46 // TR-8S OH
 5. Velocity of Note 39 // TR-8S BD ALT
 6. Velocity of Note 35 // TR-8S SD ALT
 7. Velocity of Note 40 // TR-8S CP
 8. Reset trigger on MIDI START/STOP     // RESET trigger


 ## Ideas for future development:
  - More MIDI messages: Pitchbend, Note Pitch (with live calibration - have ideas how to do it)
  - 8 Preset for live switching (with Program Change support)
  - SysEx for live reconfiguration
  - Add more names for Well-Known Controllers.
  - Support both 8V and 5V ranges at the same time with per-output selection
  - Support All Controllers Off message
  - ... your ideas are welcome!


## CC BY-NC-ND 4.0 KAY KNOFE OF LPZW.modules
## parts CC BY-NC-ND 4.0 DMITRY S. BAIKOV