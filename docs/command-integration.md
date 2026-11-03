# Decapper command API schema 1

## Connection and execution
Python selects one TCP or UART route. Each route has its own state, response queue,
timeout and position cache. Connecting sends VERSION, GSTA, SPEED, then PCONFIG.
PCONFIG replies with `1,<Decapper PL base index>`; software uses that base rather
than assuming index 35. An unsupported reply or capability timeout falls back to
legacy queries. Extended teaching, parameter editing and finite LR require schema 1.

CR and LF are checked initially. Each selected terminator is appended to every
command, including handshake and queries. TCP and RS232C commands require CRLF.
TCP/RS232C frames accept at most 253 command bytes plus CRLF; longer frames are
discarded without executing a truncated prefix. USB CLI reserves terminator space
in its existing 50-byte buffer (47 command bytes); either Enter form becomes CRLF.

A motion response acknowledges registration, not mechanical completion. GSTATE
operation/result correlates acceptance and completion. Three-second response timeouts
never resend motion. STOP preempts a pending request. State older than three seconds,
unknown outcomes, Busy and faults block new motion. There is no automatic fault clear,
SAVE, reconnect replay, or transport fallback.

Legacy GSTA has no operation identity. A fast motion may finish between samples;
when completion cannot be established, the UI explicitly asks for STOP then a fresh
state query. It never silently assumes completion or retries the motion. Updating
firmware to schema 1 enables normal completion correlation.

## Commands
All replies start with the received command name and end in CRLF.
Failures use INVALID, UNSUPPORTED, RANGE, BUSY, FAIL or an E#### code.

| Request | Reply body / behavior |
|---|---|
| PCONFIG | 1,base |
| GPOS Z / GPOS R | axis,pulse; pure read, never changes the legacy teaching candidate |
| PGET index | index,value from current RAM; float values retain 32-bit precision |
| TEACH index,pulse | OK after changing a whitelisted RAM target; validates idle state and limits |
| SAVE 1 | OK / FAIL after backup/main EEPROM CRC and exact readback; ignores legacy RPOS candidate |
| LOAD | OK HOME_REQUIRED / FAIL; validated settings apply to RAM/drivers under maintenance lock |
| GSTATE | busy,homed,error,paused,phase,idle,error_code,operation_id,result |
| GSTA / ST | busy,enable,homed,error,error_code[,isGripCap,isGripBody] |
| GSENSOR | CT,ZHigh,ZLow,YForward,YBack,CapOpen,CapClose,BodyOpen,BodyClose (raw bits) |
| LR count | LR acknowledgement; count 1..1000000, completes after the final full CAP cycle |
| GLR | completed_count,limit,cap_decap_phase,paused,busy |
| SL | STATE=<GSTATE fields>;SENSOR=<GSENSOR fields> |
| CD | TARGET_Z=value;TARGET_R=value;SPEED=value;LR=<GLR fields> |
| PL | All 22 Decapper index,value pairs separated by semicolons |
| PL index,value | OK, then Python requests PGET index to read back RAM |

GSTATE result: 0 none, 1 accepted/running, 2 completed, 3 stopped, 4 fault.
Operation id is incremented at request registration. Phase numbers are the firmware
FSM enums, not percentages. Sensor bits are electrical observations, not inverted
logical ON values. GLR counts complete DECAP/CAP pairs. Limit 0 means legacy unlimited LR.
USB CLI keeps its detailed SL/CD/PL console output and also returns the summary.
The PC reads GSTA grip flags separately from GSTATE, so GSTA does not overwrite
motion identity or idle status when schema 1 is active. The firmware infers cap
held from both cap switches reading 0 and retains the last CT reading made at
Y Forward as the body flag. The PC does not infer vial presence from gripper
output. Y arrival is active-low; READY 0 is Forward and READY 1 is Back.
The UART control can open a COM port, but separate RS232 command handling is not
started by the current firmware. Check for a valid device handshake.

## Decapper parameter offsets from advertised base

| Offset | Field / unit |
|---|---|
| 0,1 | Z/R running current (A, float) |
| 2,3 | Z/R maximum current code: 0=3.1 A range, 1=1.7 A range |
| 4,5 | Z/R hold current percentage, 0..100 |
| 6 | Resolution code 0..8: 51200,25600,12800,6400,3200,1600,800,400,200 pulses/revolution |
| 7,8 | Z/R basic position limits (pulse) |
| 9,10 | Z negative/positive software limits (pulse) |
| 11,12 | R negative/positive software limits (pulse) |
| 13 | Software limits enabled, 0/1 |
| 14,15 | Z acceleration / velocity |
| 16,17,18 | Z cap upper / side / origin position (pulse) |
| 19,20 | R acceleration / velocity |
| 21 | R rotation amount (pulse), not an absolute position |

TEACH permits offsets 16,17,18,21 only. The editor exposes the 22 actual Decapper
fields; unrelated legacy PL slots that do not change RAM are not offered.
Resolution and current-selection codes are checked in both UI and firmware.
Position preflight uses current RAM basic limits intersected with enabled software
limits. REL MOVE also requires a recently read position to calculate the endpoint;
firmware remains the final validator.

Applying a parameter changes RAM. EEPROM Save confirms saving the whole configuration.
Current/resolution changes may require SAVE 1 followed by LOAD or reboot and a new HOME.
LOAD reinitializes driver coordinates, so the previous home reference is invalid.
Legacy RPOS and plain SAVE are preserved for existing clients; RPOS still changes a
legacy save candidate. New UI reads use GPOS and saves use SAVE 1.

## Screen and verification
Test / Teaching / Debug / Status names are preserved. Status contains only SL/CD/PL.
Debug offers separate Longrun buttons: Unlimited Start sends plain LR and works
with legacy firmware; Count Start sends LR count and requires schema 1. STOP ends
either mode. Completed count is read from GLR on schema-1 firmware, increasing
once per finished DECAP/CAP pair; legacy firmware still counts internally but cannot
provide the new GLR display.
The upper Teaching & Motion panel uses a 60:40 starting ratio with Command,
subject to the page minimum sizes. Motion tabs have no scroll areas. State/F/W/API appear in a small
single line; full state is also available as a tooltip. Minimum window size is 1000x720.

The PC separates transmission, acceptance and firmware-reported completion.
Automatic CAP/DECAP is gated on HOME and a recent idle CT sample. Firmware can
still return completion without moving if prerequisites change after the sample,
so the PC labels that result as a firmware reply. GLR supplies the final count
after terminal GSTATE; a lost reply or connection leaves the final count unknown.
The PC never retries motion or sends STOP just because the connection drops.

Software tests run with:
`.venv/Scripts/python.exe -m unittest discover -s tests`.

Firmware host tests run with Python and a C compiler:
set ZIG_EXE to Zig 0.13.0, then run every `tests/safety/test_*.py`.
They exercise real parser/handlers/FSM with mocked hardware, including 1000 long-run
cycles, 28 pause/resume phase cases, serial framing and EEPROM failure paths.
STM32 ARM compile/link is separately checked. These are offline tests: mechanical
motion, live sensor polarity, real EEPROM persistence and flashing require equipment validation.
