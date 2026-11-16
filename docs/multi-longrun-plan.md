# Multiple-module Longrun

User requested: remove current controls from Teaching; move Longrun out of Debug
into a new tab beside Status, with checked IPs, connection status and batch start.

Use a separate TCP client and DeviceSession per IP:port. Keep the existing single
device workflow. Reject duplicate endpoints and an endpoint already connected in
single-device control; show that the single connection must first be released.
Do not connect physical modules during development or send automatic HOME/SAVE.

- [x] Test selection/endpoint validation, two independent handshakes, finite and
  unlimited batch starts, partial failures, STOP priority and lost connection.
- [x] Implement ui/longrun_panel.py with independent socket/session lifetimes,
  bounded RX framing, polling and module-labelled logs. Continue monitoring when
  another tab is selected or a running row is unchecked.
- [x] Add the Longrun tab and remove Debug Longrun controls. Provide selected
  connect/disconnect, count/unlimited starts, selected STOP, all STOP and CSV.
  The shared STOP targets all batch modules while on Longrun; pause/resume and
  single-device query buttons remain limited to single-device tabs.
- [x] Keep true parameter offsets after removing Teaching current fields.
- [x] Run offline and local TCP loopback tests, inspect minimum-size captures,
  request read-only review and apply only verified changed files.

No count is assumed: start with one editable IP row and allow additions. IPv4
is required, port defaults to 8000. Starts require every checked module to be
ready; network transmission cannot be atomic or mechanically synchronized.
