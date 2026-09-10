# TCP Command Client Design

## Goal

Turn the existing Vial Decapper command panel into a working TCP client that
connects to the device, sends the entered command, and displays TX, RX, and
connection events.

## Firmware Protocol

The Vial Decapper firmware listens for application commands on TCP port 8000.
Commands are ASCII text. Its network receive loop recognizes a command frame
only when it receives CR followed immediately by LF (`\r\n`), although the PC
application must send exactly the terminators selected by the operator.

Examples are `GSTA`, `SPEED 50`, `HOME`, `DECAP`, and `CAP`. The device returns
the command result over the same TCP connection.

Source references:

- `Vial_Decapper/App/Task/_10_XNetworkMsg_Process.c`: TCP command frames are
  split on `\r\n`, and responses are sent through port 8000.
- `Vial_Decapper/App/System/XEEPROMParam.c`: factory default port is 8000.

## Architecture

Create `ui/tcp_command_client.py` containing a `TcpCommandClient` QObject that
owns one `QTcpSocket`. It exposes connect, disconnect, and send operations and
emits signals for connection state, received bytes, sent bytes, and errors.
The QObject keeps socket activity asynchronous, so the GUI remains responsive.

`MainWindow` creates this client, connects its signals to the communication
log, and owns only UI validation and control state. The client is not aware of
widgets or log formatting.

## UI Behavior

- The port field starts with `8000`.
- Connect validates a non-empty host and an integer port from 1 through 65535.
  It connects when disconnected; when connected, the button disconnects.
- Send is enabled only while the TCP socket is connected.
- Send reads the command input. Empty or whitespace-only values are not sent
  and are recorded in the log.
- The command is ASCII-encoded. A non-ASCII input is rejected and logged.
- A checked CR box appends byte `0x0D`; a checked LF box appends byte `0x0A`.
  They are independent: neither, either one, or both can be sent. The UI never
  replaces or forces the selected terminators.
- TX log entries render CR and LF visibly as `\\r` and `\\n`; RX entries decode
  bytes as UTF-8 using replacement for malformed sequences.
- Socket errors, connected, and disconnected events are shown in the log.
- Every received TCP chunk is appended as a new `RX:` entry at the end of the
  existing log. The log is never cleared automatically, so the operator can
  review the full connection and command history and read only the entries
  needed.
- A `Save` button immediately beside `Send` opens a file-save dialog for the
  complete communication log. Its suggested filename is
  `vial_decapper_log_YYYYMMDD_HHMMSS.txt`; accepted files are UTF-8 text and
  preserve every log line. Cancelling the dialog leaves the log unchanged.

## Testing

Use a local `QTcpServer` in tests. Verify that the client sends exact byte
sequences for all four CR/LF choices and emits incoming data. Verify the window
defaults to port 8000, passes a command through the connected client, and
shows the visible TX representation. Existing headless Qt setup remains in
use.
