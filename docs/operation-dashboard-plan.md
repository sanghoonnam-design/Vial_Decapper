# Decapper operation dashboard

Keep the four existing tabs and the command console. Put current conditions in
Test/Debug, settings verification in Teaching and the selected SL/CD/PL snapshot
in Status. A separate dashboard would duplicate the controls and take space away
from the console. The 3D panel has an optional read-only device view.

1. Extend each transport session with fresh sensor/grip readings, correlated
   motion and stop results, fault detail, and final long-run records.
2. Parse SL/CD/PL without mistaking targets for measured positions or old
   summaries for motion acknowledgements. Verify settings against PGET and
   acknowledge EEPROM persistence only on SAVE OK.
3. Bind these to compact panels; check minimum size, old replies, blocked
   commands, disconnected/stale readings, and CSV export. Use the current
   firmware API; do not add automatic motion, LOAD, or firmware changes.

Review: STOP before a reply, old operation IDs, lost connection during LR,
legacy GSTA, failed readback or save, and unknown Y position in the live view.
