# Decapper safety regression tests

Run from the repository root with Python 3 and Zig 0.13.0 available on PATH (or set `ZIG_EXE` to zig.exe):

```powershell
python tests/safety/test_motion.py
python tests/safety/test_maintenance.py
```

These host tests compile the actual Decapper C source and selected command handlers with mocked hardware, timer, and storage APIs. They check 15 motion/fault cases and 5 maintenance cases. They do not operate hardware or replace target firmware compilation and physical validation.

Scope: review summary items 2, 3, 4, and 7. The deferred sensor interlock remains commented; TCP/parser and EEPROM CRC behavior are unchanged.
