# Vial Decapper PySide6 Template

This project is a minimal PySide6 application template with a main window in `ui/`. UI source belongs in `ui/`.

## Device preview

The left panel shows a simplified 3D model estimated from the supplied front and
side images. Drag with the left mouse button to rotate, scroll to zoom, and
double-click to restore the default view. Dimensions are illustrative; the preview
is not connected to live machine motion.

The carriage preview slider moves the lower holder along the front/back rails.
The upright at the front of the base represents the fixed vial-presence sensor.
Travel is illustrative, and the slider never sends device commands or simulates
a measured sensor state. Double-click resets the camera only.

Geometry lives in `ui/device_model.py`; `ui/device_model_view.py` renders it with
PySide6 alone, without an additional 3D engine. The lightweight renderer sorts
faces by depth, so intersecting parts may show minor overlap artifacts at some angles.

## Communication controls

- Select `TCP/IP` or `UART` in the command panel before sending commands. All
  command buttons use that selection and are enabled only when the selected
  connection is open. Disconnecting does not switch to the other connection.
- CR and LF start checked. Each checkbox appends only its selected character
  to every command, including Teaching & Motion buttons. The current firmware's
  TCP command parser requires both CR and LF to execute a command.
- TX means the local transport accepted the bytes, not that the device executed
  the command. Check RX for the device response. Logs identify the transport.
- Complete newline-terminated responses are processed in order. Position values
  are updated only from the selected transport; disconnecting clears the value.
- The communication log displays and saves the most recent 10,000 lines.

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
