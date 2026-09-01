# Vial Decapper PySide6 Template

This project is a minimal PySide6 application template with a main window in `ui/`. UI source belongs in `ui/`.

## Windows setup and run

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## Windows path length note

If PySide6 installation fails with a path-length error, move or clone the project to a shorter path (for example `C:\dev\vial-decapper`) or enable Windows long-path support, recreate `.venv`, and run the installation command again.
