# Vial Decapper

PySide6 controls for single-device teaching, diagnostics and multi-module Longrun.

## Device preview

The left panel shows a simplified 3D model estimated from the supplied front and
side images. Drag with the left mouse button to rotate, scroll to zoom, and
double-click to restore the default view. Dimensions are illustrative; the preview
is not connected to live machine motion.

The carriage preview slider moves the lower holder along the front/back rails.
The upright at the front of the base represents the fixed vial-presence sensor.
Travel is illustrative, and the slider never sends device commands or simulates
a measured sensor state. Double-click resets the camera only.
Enable **장비 상태 보기** to show the last fresh Y endpoint and cap-held flag
from the selected device. The holder is hidden when neither endpoint is known.

Geometry lives in `ui/device_model.py`; `ui/device_model_view.py` renders it with
PySide6 alone, without an additional 3D engine. The lightweight renderer sorts
faces by depth, so intersecting parts may show minor overlap artifacts at some angles.

## Communication controls

- Select `TCP/IP` or `UART` in the command panel before sending commands. Each
  route has its own device state and response queue. STOP is available on connection;
  motion waits for a fresh idle/error-free state and confirmed completion of the
  previous request. Disconnecting does not switch to the other connection.
- CR and LF start checked. Each checkbox appends only its selected character
  to every command, including Teaching & Motion buttons. The current firmware's
  TCP command parser requires both CR and LF to execute a command.
- TX means the local transport accepted the bytes, not that the device executed
  the command. Check RX for the device response. Logs identify the transport.
- Commands accept one printable ASCII line of up to 253 bytes before CR/LF.
  Complete newline-terminated responses are processed in order. Position values
  are updated only from the selected transport; disconnecting clears the value.
- The communication log displays and saves the most recent 10,000 lines.

Teaching uses `GPOS` for axis reads and `TEACH` for explicit RAM targets. On current
firmware, Read captures the two coordinates with `RPOS` when the device is homed and
idle. `SAVE Z` saves the Z CAP UP capture and `SAVE R` saves the R DECAP capture.
Both saves are followed by a readback. The parameter editor uses the Decapper PL
indexes reported by the firmware. See
[the command contract](docs/command-integration.md) for replies, parameter codes,
legacy behavior and equipment validation limits.

- **Test** shows HOME/CT readiness and the last operation result. Automatic
  CAP/DECAP requires a recent idle CT sample from schema-1 firmware. STOP is
  confirmed after the firmware reports idle; legacy firmware can report Busy
  clear only. Clearing an error never starts motion.
- **Test** combines the former Debug controls above the automatic controls,
  showing Y limit/CT and the firmware's grip flags. Vial detection at
  Forward is retained at Back.
- **Longrun** has its own tab beside Status. Add IPv4 addresses, check modules,
  use **선택 연결**, then **횟수 시작** or **무한 시작**. All checked modules must be
  connected and ready. Each module shows its own status, count and elapsed time.
  Unchecked running modules are still monitored. **전체 STOP** and the top STOP
  on this tab stop all connected Longrun modules. CSV records include IP/port.
  Release a module's single-device TCP connection before connecting it here.
  Reconnect does not restart a test. Disconnect is not a STOP command.
- **Teaching** shows requested changes after RAM readback. Position checks use
  the last verified limits. After an uncertain LOAD or factory reset, a fresh
  state reply is followed by a new read of the firmware settings.
- **Status** renders SL/CD/PL as read-only snapshots. CD target positions do not
  replace positions from GPOS/RPOS.

## Tests

```powershell
.venv\Scripts\python.exe -m unittest discover -s tests -v
```

Tests include a local TCP loopback server and simulated UART writes. They do not
connect to or move a physical device.

## Windows setup and run

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## Windows path length note

If PySide6 installation fails with a path-length error, move or clone the project to a shorter path (for example `C:\dev\vial-decapper`) or enable Windows long-path support, recreate `.venv`, and run the installation command again.
