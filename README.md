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

## Windows setup and run

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## Windows path length note

If PySide6 installation fails with a path-length error, move or clone the project to a shorter path (for example `C:\dev\vial-decapper`) or enable Windows long-path support, recreate `.venv`, and run the installation command again.
