# TRAM8 Module 4x4 Firmware

Based on Stock Firmware 1.3
Timer code for 1ms ticks from https://github.com/thorinf/tram8-plus/

4x4 means:
4 trigger outputs for notes 36, 38, 42, 46 which correspond to TR-8S BD, SD, CH, OH
4 velocity CV outputs for these notes, 0-8V
4 clock outputs: tripets, quarters, Run gate, Reset trigger
4 CC to 0-8V CV for controllers 24, 29, 63, 82 which correspond to TR-8S BD, SD, CH, OH levels

Firmware supports both gate and trigger outputs with custom ppqn, CC, and Note support.
At the moment the configuration is hardcoded. An HTML-based SysEx editor is planned.

Hacked by Dmitry Baikov.
Built on MacOSX using avr-gcc and CMake.
Tested with Squarp Hapax and Nano modules Octa

Default configuration:

```c
struct GateState gates[NUM_OUT_GATES] = {
    {GateMode_Trigger | GateSource_Note, 36, {}, 0}, // TR-8S BD
    {GateMode_Trigger | GateSource_Note, 38, {}, 0}, // TR-8S SD
    {GateMode_Trigger | GateSource_Note, 42, {}, 0}, // TR-8S CH
    {GateMode_Trigger | GateSource_Note, 46, {}, 0}, // TR-8S OH
    {GateMode_Trigger | GateSource_Clock, 16, {}, 0}, // 2/3 ppqn
    {GateMode_Trigger | GateSource_Clock, 24, {}, 0}, // 1 ppqn
    {GateMode_Gate    | GateSource_Clock,  0, {}, 0}, // RUN gate
    {GateMode_Trigger | GateSource_Clock,  0, {}, 0}, // RESET trigger
};

struct VoltageState voltages[NUM_OUT_VOLTAGES] = {
    {VoltageSource_Note | 36}, // TR-8S BD
    {VoltageSource_Note | 38}, // TR-8S SD
    {VoltageSource_Note | 42}, // TR-8S CH
    {VoltageSource_Note | 46}, // TR-8S OH
    {VoltageSource_ControlChange | 24}, // TR-8S BD Level
    {VoltageSource_ControlChange | 29}, // TR-8S SD Level
    {VoltageSource_ControlChange | 63}, // TR-8S CH Level
    {VoltageSource_ControlChange | 82}, // TR-8S OH Level
};
```

# CC BY-NC-ND 4.0 KAY KNOFE OF LPZW.modules


