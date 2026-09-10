# TCP Command Client Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Vial Decapper command panel connect to the device over TCP, send the entered command with its selected CR/LF bytes, and append received data to the communication log.

**Architecture:** `ui.tcp_command_client.TcpCommandClient` owns the asynchronous `QTcpSocket` and exposes typed operations plus signals. `MainWindow` validates UI values, formats log lines, and delegates all network I/O to that client.

**Tech Stack:** Python 3, PySide6 (`QtCore`, `QtNetwork`, `QtWidgets`), unittest

**Spec:** `docs/superpowers/specs/2026-09-10-tcp-command-client-design.md`

## Global Constraints

- Connect to the Vial Decapper TCP server; the default port is `8000`.
- Encode command text as ASCII.
- Append `0x0D` only when CR is selected and append `0x0A` only when LF is selected; do not force either terminator.
- Keep all GUI and TCP client modules below `ui/`.
- Do not block the Qt GUI thread waiting for a network operation.
- Append connection, TX, RX, and error events without automatically clearing the log.

---

### Task 1: Create the asynchronous TCP command client

**Files:**
- Create: `ui/tcp_command_client.py`
- Create: `tests/test_tcp_command_client.py`

**Interfaces:**
- Produces: `TcpCommandClient(QObject)` with `connect_to_host(host: str, port: int) -> None`, `disconnect_from_host() -> None`, and `send_command(command: str, append_cr: bool, append_lf: bool) -> bytes`.
- Produces signals: `connected()`, `disconnected()`, `received(bytes)`, `sent(bytes)`, and `error(str)`.
- `send_command` returns the byte payload written, or `b""` after emitting `error` when input is empty, non-ASCII, or the socket is not connected.

- [ ] **Step 1: Write the failing client tests**

```python
def test_send_command_writes_exactly_the_selected_terminators(self) -> None:
    server = self._listen_on_localhost()
    client = TcpCommandClient()
    client.connect_to_host("127.0.0.1", server.serverPort())
    peer = self._wait_for_peer(server)

    for append_cr, append_lf, expected in (
        (False, False, b"GSTA"),
        (True, False, b"GSTA\r"),
        (False, True, b"GSTA\n"),
        (True, True, b"GSTA\r\n"),
    ):
        self.assertEqual(client.send_command("GSTA", append_cr, append_lf), expected)
        self.assertEqual(bytes(self._wait_for_ready_read(peer)), expected)

def test_received_socket_bytes_are_emitted(self) -> None:
    server = self._listen_on_localhost()
    client = TcpCommandClient()
    received = []
    client.received.connect(received.append)
    client.connect_to_host("127.0.0.1", server.serverPort())
    peer = self._wait_for_peer(server)
    peer.write(b"GSTA,0")
    self._wait_until(lambda: received)
    self.assertEqual(received, [b"GSTA,0"])
```

The test module defines `_wait_until` with `QEventLoop` plus a 1-second
`QTimer`, `_wait_for_peer` using `server.waitForNewConnection(1000)`, and
`_wait_for_ready_read` using `socket.waitForReadyRead(1000)`; each helper
fails the test if its event does not arrive.

- [ ] **Step 2: Run the new client test to verify it fails**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_tcp_command_client -v`

Expected: FAIL because `ui.tcp_command_client` does not exist.

- [ ] **Step 3: Implement the minimal client**

```python
class TcpCommandClient(QObject):
    connected = Signal()
    disconnected = Signal()
    received = Signal(bytes)
    sent = Signal(bytes)
    error = Signal(str)

    def send_command(self, command: str, append_cr: bool, append_lf: bool) -> bytes:
        try:
            payload = command.strip().encode("ascii")
        except UnicodeEncodeError:
            self.error.emit("Commands must use ASCII characters.")
            return b""
        if not payload or self.socket.state() != QAbstractSocket.SocketState.ConnectedState:
            self.error.emit("Cannot send: TCP socket is not connected.")
            return b""
        payload += b"\r" if append_cr else b""
        payload += b"\n" if append_lf else b""
        self.socket.write(payload)
        self.sent.emit(payload)
        return payload
```

Connect `QTcpSocket.connected`, `disconnected`, `readyRead`, and `errorOccurred` to the client signals. In the `readyRead` handler emit exactly the bytes returned by `readAll()`.

- [ ] **Step 4: Run the client tests to verify they pass**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_tcp_command_client -v`

Expected: PASS.

### Task 2: Wire TCP operations into the command window

**Files:**
- Modify: `ui/main_window.py`
- Modify: `tests/test_main_window.py`

**Interfaces:**
- Consumes: `TcpCommandClient` from Task 1.
- Produces: Connect/Disconnect behavior and log entries driven by the client signals.

- [ ] **Step 1: Write failing window tests**

```python
def test_connection_bar_defaults_to_firmware_port(self) -> None:
    window = MainWindow()
    port_input = window.findChild(QLineEdit, "port_input")
    self.assertEqual(port_input.text(), "8000")

def test_send_appends_visible_tx_entry_for_selected_terminators(self) -> None:
    window = MainWindow()
    window.command_client.sent.emit(b"GSTA\r\n")
    log = window.findChild(QPlainTextEdit, "communication_log")
    self.assertIn("TX: GSTA\\r\\n", log.toPlainText())
```

- [ ] **Step 2: Run the window tests to verify they fail**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_main_window -v`

Expected: FAIL because the port has no default and no send handler exists.

- [ ] **Step 3: Implement window integration**

```python
self.command_client = TcpCommandClient(self)
self.command_client.connected.connect(lambda: self._append_log("Connected."))
self.command_client.received.connect(self._log_received)
self.command_client.sent.connect(self._log_sent)

def _log_sent(self, payload: bytes) -> None:
    self._append_log(f"TX: {self._render_bytes(payload)}")

def _log_received(self, payload: bytes) -> None:
    self._append_log(f"RX: {payload.decode('utf-8', errors='replace')}")
```

Set the port input text to `8000`, connect the Connect button to validation plus `connect_to_host`, and connect the Send button and Return key to `_send_command`. `_render_bytes` must replace actual CR and LF with visible `\\r` and `\\n` sequences. Do not append a second TX line in `_send_command`; the client's `sent` signal owns that responsibility.

- [ ] **Step 4: Run the window tests to verify they pass**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_main_window -v`

Expected: PASS.

### Task 3: Save the communication log

**Files:**
- Modify: `ui/main_window.py`
- Modify: `tests/test_main_window.py`

**Interfaces:**
- Consumes: the read-only `communication_log` and its complete plain-text
  content.
- Produces: a Save button beside Send and `_save_communication_log() -> None`.

- [ ] **Step 1: Write the failing save tests**

```python
@patch("ui.main_window.QFileDialog.getSaveFileName")
def test_save_button_writes_the_full_communication_log(self, dialog_mock) -> None:
    window = MainWindow()
    window.findChild(QPlainTextEdit, "communication_log").setPlainText("TX: GSTA\\r\\n\nRX: GSTA,0")
    dialog_mock.return_value = (str(self.temp_path), "Text files (*.txt)")

    window._save_communication_log()

    self.assertEqual(self.temp_path.read_text(encoding="utf-8"), "TX: GSTA\\r\\n\nRX: GSTA,0")
```

- [ ] **Step 2: Run the save test to verify it fails**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_main_window.MainWindowTests.test_save_button_writes_the_full_communication_log -v`

Expected: FAIL because `_save_communication_log` does not exist.

- [ ] **Step 3: Implement the Save button and UTF-8 write**

```python
save_button = QPushButton("Save")
save_button.setObjectName("log_save_button")
save_button.clicked.connect(self._save_communication_log)
command_row.addWidget(save_button)

def _save_communication_log(self) -> None:
    default_name = f"vial_decapper_log_{datetime.now():%Y%m%d_%H%M%S}.txt"
    filename, _ = QFileDialog.getSaveFileName(self, "Save communication log", default_name, "Text files (*.txt)")
    if filename:
        Path(filename).write_text(self.communication_log.toPlainText(), encoding="utf-8")
```

- [ ] **Step 4: Run the save test to verify it passes**

Run: `.venv\\Scripts\\python.exe -m unittest tests.test_main_window.MainWindowTests.test_save_button_writes_the_full_communication_log -v`

Expected: PASS.

### Task 4: Run the complete verification suite

**Files:**
- Verify: `main.py`, `ui/`, `tests/`

**Interfaces:**
- Verifies: application startup, TCP payload construction, connection UI behavior, and communication-log behavior.

- [ ] **Step 1: Run all tests**

Run: `.venv\\Scripts\\python.exe -m unittest discover -s tests -v`

Expected: PASS with no failures.

- [ ] **Step 2: Check source syntax without writing bytecode**

Run: `.venv\\Scripts\\python.exe -B -m py_compile main.py ui\\main_window.py ui\\tcp_command_client.py`

Expected: exit code 0.

- [ ] **Step 3: Inspect the final diff**

Run: `git diff --check` followed by `git diff -- ui/main_window.py ui/tcp_command_client.py tests`

Expected: no whitespace errors and only TCP command-client changes.
