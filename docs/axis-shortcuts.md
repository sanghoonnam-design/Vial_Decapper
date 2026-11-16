# Axis position shortcuts

The Test tab labels the Y valve commands as Y Forward (READY 0) and Y Backward
(READY 1). The position row below contains Origin Move, CAP UP and CAP SIDE.

- Origin Move sends MOVE A Z 0, waits for its matching operation ID and normal
  idle completion, then sends MOVE A R 0. It never sends HOME or ORG and does
  not redefine the coordinate system.
- CAP UP and CAP SIDE first read PGET base+16 and base+17 respectively and use
  the returned integer Z coordinate in MOVE A Z. Cached teaching values are not
  used as the target. Firmware RAM settings are not changed or saved.
- The advertised parameter base, cached device limits and operation identity
  must be known. The firmware remains the final limit validator.
- STOP, fault, uncertain results, disconnect, tab and transport selection changes
  cancel remaining steps. Moving axes are not retried automatically.
  The upper STOP on Longrun also stops unfinished single-device motion, even if
  there are no Longrun sockets connected.

The existing HOME and ORG buttons remain in the lower automatic row. HOME is a
homing operation; ORG returns Z to zero and Y Forward and does not set R to zero.
These commands are distinct from Origin Move.
