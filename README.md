# TRAM8 Module 4x4 Firmware

## v2 Features:

 - Offline HTML-based configuration editor
 - Configurable global MIDI channel
 - Optional per-output MIDI channel selection
 - Global CV range selection: 0-5V or 0-8V
 - Per-gate output modes: Gate or Trigger
 - Configurable global trigger length (1 - 120 ms)
 - Gate/Trigger sources: Note On, MIDI Clock divisors, RUN gate, Reset trigger
 - Gate output from Clock divisor works as a flip-flop (square LFO)
 - CV sources: Note Velocity, Controller Value (CC)
 - Supports All Sound Off and All Notes Off messages
 - **No learn mode at the moment**

Based on Stock Firmware 1.3

Timer code for 1ms ticks from https://github.com/thorinf/tram8-plus/


## To use:
 - Download and open locally `FW-4x4 HTML FILES/tram8_4x4_fw2.html`
 - Edit the settings to your liking and get the `tram8_4x4_fw2_edit.syx`
 - Send the `tram8_4x4_fw2_edit.syx` to your Tram8 module in the update mode
 - Enjoy!


## Why 4x4

4x4 comes from the initial idea and now the default configuration:
- 4 trigger outputs for notes 36, 38, 42, 46 which correspond to TR-8S BD, SD, CH, OH
- 4 velocity CV outputs for these notes, 0-8V
- 4 clock outputs: tripets, quarters, Run gate, Reset trigger
- 4 CC to 0-8V CV for controllers 24, 29, 63, 82 which correspond to TR-8S BD, SD, CH, OH levels

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
 
### Gates:
 1. Trigger on NoteOn 36 // TR-8S BD
 2. Trigger on NoteOn 38 // TR-8S SD
 3. Trigger on NoteOn 42 // TR-8S CH
 4. Trigger on NoteOn 46 // TR-8S OH
 5. Trigger on triplets (36 MIDI pulses) // 4/3 ppqn
 6. Trigger on quarters (24 MIDI pulses) // 1 ppqn
 7. Gate on MIDI START/CONT until STOP   // RUN gate
 8. Reset trigger on MIDI START/STOP     // RESET trigger

### Voltages:
 1. Velocity of Note 36 // TR-8S BD
 2. Velocity of Note 38 // TR-8S SD
 3. Velocity of Note 42 // TR-8S CH
 4. Velocity of Note 46 // TR-8S OH
 5. Value of Controller 24 // TR-8S BD Level
 6. Value of Controller 29 // TR-8S SD Level
 7. Value of Controller 63 // TR-8S CD Level
 8. Value of Controller 82 // TR-8S OH Level


 ## Ideas for future development:
  - Clock divider in quarters, not only in pulses. Quarters will allow for up to 32 bars for a flip-flop phase, i.e. square LFO with periods up to 64 bars long.
  - Note Gate and Note Dynamic Gate modes for CV outputs, to replicate the SixteenGates firmware
  - More MIDI messages: Pitchbend, Channel Pressure, Aftertouch ...
  - Add names for more Well-Known Controllers. At the moment it is only 6 (ModWheel)
  - Support both 8V and 5V ranges at the same time with per-output selection
  - Support All Controllers Off message
  - ... your ideas are welcome!


# CC BY-NC-ND 4.0 KAY KNOFE OF LPZW.modules
## parts CC BY-NC-ND 4.0 DMITRY S. BAIKOV