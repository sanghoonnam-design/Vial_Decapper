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
| RPOS / RPOS Z / RPOS R | Captures both teaching coordinates or the specified axis |
| PGET index | index,value from current RAM; float values retain 32-bit precision |
| TEACH index,pulse | OK after changing a whitelisted RAM target; validates idle state and limits |
| SAVE 1 | OK / FAIL after backup/main EEPROM CRC and exact readback; ignores legacy RPOS candidate |
| SAVE Z / SAVE R | Saves the corresponding RPOS capture as Z CAP UP or R DECAP; the app follows OK with PGET |
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
CD target fields contain the last MOVE request values. They can be relative
increments, and CD does not include the MOVE mode.
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
| 21 | R decapping target (pulse) on current firmware |

TEACH permits offsets 16,17,18,21 only. Status shows the 22 Decapper fields.
Teaching provides dedicated controls for the four targets and a parameter editor
for the position limits, acceleration and velocity. The F/W validates parameter values.
Position preflight uses current RAM basic limits intersected with enabled software
limits. REL MOVE also requires a recently read position to calculate the endpoint;
firmware remains the final validator.
Relative movement and copying a teaching position require a position read while
idle. Motion and STOP invalidate previous readings. A limit with an unverified
write cannot be used for a move's position check. An uncertain LOAD or factory
reset clears the cached settings. The app reads PL again after idle is confirmed.

Applying a parameter changes RAM. SAVE 1 confirms saving the whole configuration.
Current/resolution changes may require SAVE 1 followed by LOAD or reboot and a new HOME.
LOAD reinitializes driver coordinates, so the previous home reference is invalid.
On current firmware, Teaching uses RPOS while homed and idle, and GPOS for other
axis reads. Each SAVE button persists its captured axis.

## Screen and verification
Tabs are Test / Teaching / Status / Longrun. Status contains only SL/CD/PL.
The Longrun tab offers separate buttons: Unlimited Start sends plain LR and works
with legacy firmware; Count Start sends LR count and requires schema 1. STOP ends
either mode. Completed count is read from GLR on schema-1 firmware, increasing
once per finished DECAP/CAP pair; legacy firmware still counts internally but cannot
provide the new GLR display.
Test contains individual Y/gripper/unit tests and sensor reads above the automatic
controls. Y Forward and Y Backward send READY 0 and READY 1.

Longrun has editable IPv4 rows and a shared port (default 8000). Check modules,
connect them, and wait for firmware readiness before starting. Each endpoint has
its own TCP socket, state tracker, receive buffer, progress and history. Every
checked module must be ready before batch start; transmission failures are reported
per module and never trigger motion replay. Starts are not mechanically synchronized.
Unchecked connected modules continue to be monitored. Selected STOP targets checked
connections; All STOP, including the shared top STOP while this tab is selected,
targets every connected Longrun module. Main single-device connection is separate.
Duplicate endpoints, including one connecting in the main bar, are rejected:
release the existing connection before changing control workspaces.
Connection loss leaves stopping unconfirmed. Reconnect sends only read-only
discovery. Module-tagged logs and CSV records identify IP and port.
The upper Teaching & Motion panel uses a 60:40 starting ratio with Command,
subject to the page minimum sizes. Motion tabs have no scroll areas. State/F/W/API appear in a small
single line; full state is also available as a tooltip. Minimum window size is 1000x820.

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
