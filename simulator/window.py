"""Loopback TCP server and operator controls for the virtual device."""
from PySide6.QtCore import Qt, QTimer, Signal
from PySide6.QtNetwork import QHostAddress, QTcpServer
from PySide6.QtWidgets import (
    QCheckBox, QComboBox, QFormLayout, QGroupBox, QHBoxLayout, QLabel,
    QPlainTextEdit, QPushButton, QSpinBox, QVBoxLayout, QWidget,
)

from ui.device_faults import FAULTS, fault_summary, fault_tooltip
from .device import PHASE_LABELS, SimulatedDevice


MAX_COMMAND = 253
POLL_COMMANDS = {"GSTATE", "GSTA", "GSENSOR", "GLR"}


class SimulatorServer(QTcpServer):
    exchange = Signal(str, str)
    clients_changed = Signal(int)

    def __init__(self, device, parent=None):
        super().__init__(parent)
        self.device = device
        self.clients = {}
        self.newConnection.connect(self._accept)

    def _accept(self):
        while self.hasPendingConnections():
            client = self.nextPendingConnection()
            client.setReadBufferSize(4096)
            self.clients[client] = bytearray()
            client.readyRead.connect(lambda client=client: self._receive(client))
            client.disconnected.connect(lambda client=client: self._remove(client))
            self._receive(client)
        self.clients_changed.emit(len(self.clients))

    def _remove(self, client):
        if client in self.clients:
            del self.clients[client]
            client.deleteLater()
            self.clients_changed.emit(len(self.clients))

    def _receive(self, client):
        data = bytes(client.readAll())
        for byte in data:
            if client not in self.clients:
                return
            frame = self.clients[client]
            if byte != 10:
                if frame is not None:
                    if len(frame) < MAX_COMMAND + 1:
                        frame.append(byte)
                    else:
                        self.clients[client] = None
                continue
            self.clients[client] = bytearray()
            if not frame or frame[-1] != 13:
                continue
            try:
                line = frame[:-1].decode("ascii")
            except UnicodeDecodeError:
                continue
            if not line.isprintable():
                continue
            response = self.device.command(line)
            if response is not None:
                client.write((response + "\r\n").encode("ascii"))
                self.exchange.emit(line, response)

    def stop(self):
        self.close()
        for client in list(self.clients):
            client.abort()
            self._remove(client)


class SimulatorWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Vial Decapper Simulator")
        self.resize(620, 505)
        self.device = SimulatedDevice()
        self.server = SimulatorServer(self.device, self)
        self.server.exchange.connect(self._exchange)
        self.server.clients_changed.connect(self._show_clients)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(18, 16, 18, 16)
        layout.setSpacing(10)
        connection = QGroupBox("로컬 TCP")
        connection_layout = QHBoxLayout(connection)
        self.host = QComboBox()
        self.host.addItems(f"127.0.0.{index}" for index in range(1, 10))
        self.port = QSpinBox()
        self.port.setRange(1024, 65535)
        self.port.setValue(18000)
        self.listen_button = QPushButton("서버 시작")
        self.listen_button.clicked.connect(self._toggle_server)
        self.clients_label = QLabel("연결 0개")
        connection_layout.addWidget(self.host, 1)
        connection_layout.addWidget(self.port)
        connection_layout.addWidget(self.listen_button)
        connection_layout.addWidget(self.clients_label)
        layout.addWidget(connection)

        state = QGroupBox("가상 장비 상태")
        state_layout = QFormLayout(state)
        self.state_label = QLabel()
        self.state_label.setObjectName("deviceState")
        self.position_label = QLabel()
        self.progress_label = QLabel()
        self.vial_checkbox = QCheckBox("바이알 감지")
        self.vial_checkbox.setChecked(self.device.vial_present)
        self.vial_checkbox.toggled.connect(self.device.set_vial)
        state_layout.addRow("상태", self.state_label)
        state_layout.addRow("위치", self.position_label)
        state_layout.addRow("Longrun", self.progress_label)
        state_layout.addRow(self.vial_checkbox)
        layout.addWidget(state)

        fault = QGroupBox("고장 주입")
        fault_layout = QVBoxLayout(fault)
        fault_controls = QHBoxLayout()
        self.fault_choice = QComboBox()
        for code, (label, _) in FAULTS.items():
            self.fault_choice.addItem(f"{code} · {label}", code)
        self.fault_choice.currentIndexChanged.connect(self._update_fault_description)
        self.fault_button = QPushButton("고장 발생")
        self.fault_button.setObjectName("injectFault")
        self.fault_button.clicked.connect(self._inject_fault)
        self.clear_button = QPushButton("고장 해제")
        self.clear_button.clicked.connect(self._clear_fault)
        fault_controls.addWidget(self.fault_choice, 1)
        fault_controls.addWidget(self.fault_button)
        fault_controls.addWidget(self.clear_button)
        fault_layout.addLayout(fault_controls)
        self.fault_description = QLabel()
        self.fault_description.setTextFormat(Qt.TextFormat.PlainText)
        self.fault_description.setWordWrap(True)
        fault_layout.addWidget(self.fault_description)
        layout.addWidget(fault)

        self.poll_checkbox = QCheckBox("주기 조회 로그 포함")
        layout.addWidget(self.poll_checkbox)
        self.log = QPlainTextEdit(readOnly=True)
        self.log.setMaximumBlockCount(300)
        self.log.setPlaceholderText("서버 상태와 송수신 기록")
        layout.addWidget(self.log, 1)

        self.setStyleSheet("""
            SimulatorWindow { background: #101b25; }
            QLabel, QCheckBox, QGroupBox { color: #d6e5ed; }
            QGroupBox {
                border: 1px solid #365364; border-radius: 7px; margin-top: 10px; padding: 7px;
            }
            QGroupBox::title { subcontrol-origin: margin; left: 9px; padding: 0 4px; color: #a5dfd4; }
            QPushButton, QComboBox, QSpinBox, QPlainTextEdit {
                background: #1b2e3b; color: #e4edf3; border: 1px solid #365364;
                border-radius: 5px; padding: 5px 8px;
            }
            QPushButton:hover { background: #28505b; }
            QPushButton:disabled, QComboBox:disabled, QSpinBox:disabled { color: #8899a5; }
            QPushButton#injectFault { background: #8c3e4e; border-color: #bd6675; }
            QPushButton#injectFault:hover { background: #a84b5e; }
            QLabel#deviceState { font-weight: 700; }
            QPlainTextEdit { background: #0c151d; font-family: Consolas, 'Malgun Gothic'; }
        """)
        self.timer = QTimer(self)
        self.timer.setInterval(50)
        self.timer.timeout.connect(self._update_state)
        self.timer.start()
        self._update_state()

    def _show_clients(self, count):
        self.clients_label.setText(f"연결 {count}개")

    def _toggle_server(self):
        if self.server.isListening():
            self.server.stop()
            self.log.appendPlainText("서버 정지")
        else:
            address = self.host.currentText()
            if not self.server.listen(QHostAddress(address), self.port.value()):
                self.log.appendPlainText(f"서버 시작 실패 · {self.server.errorString()}")
                return
            self.log.appendPlainText(f"서버 시작 · {address}:{self.server.serverPort()}")
        listening = self.server.isListening()
        self.listen_button.setText("서버 정지" if listening else "서버 시작")
        self.host.setEnabled(not listening)
        self.port.setEnabled(not listening)

    def _inject_fault(self):
        code = self.fault_choice.currentData()
        if self.device.error != "E0000":
            return
        self.device.inject_fault(code)
        self.log.appendPlainText(f"고장 주입 · {code} {FAULTS[code][0]}")
        self.log.appendPlainText(f"GERD {self.device.fault_detail}")
        self._update_state()

    def _clear_fault(self):
        self.device.clear_fault()
        self.log.appendPlainText("고장 해제")
        self._update_state()

    def _exchange(self, command, response):
        if command.upper().split()[0] not in POLL_COMMANDS or self.poll_checkbox.isChecked():
            self.log.appendPlainText(f"RX {command}    TX {response}")

    def _update_fault_description(self):
        code = self.device.error
        if code in FAULTS:
            detail = self.device.fault_detail
            summary = fault_summary(code, detail)
            text = summary.partition("\n")[2]
            tooltip = fault_tooltip(summary, detail)
        else:
            _, text = FAULTS[self.fault_choice.currentData()]
            tooltip = text
        if self.fault_description.text() != text:
            self.fault_description.setText(text)
        if self.fault_description.toolTip() != tooltip:
            self.fault_description.setToolTip(tooltip)

    def _update_state(self):
        device = self.device
        device.tick()
        if device.error in FAULTS:
            status = f"{device.error} · {FAULTS[device.error][0]}"
            color = "#ffb6bf"
        elif device.paused:
            status, color = "일시정지", "#f3d49c"
        elif device.busy:
            phase = device.motion.phase
            status = ("Longrun · " + ("DECAP" if device.lr_phase == 0 else "CAP")
                      if phase == 16 else PHASE_LABELS[phase])
            color = "#acd7f1"
        else:
            status, color = "대기 · " + ("HOME 완료" if device.homed else "HOME 필요"), "#a9e9dc"
        if self.state_label.text() != status:
            self.state_label.setText(status)
            self.state_label.setStyleSheet(f"color: {color};")
        self.position_label.setText(
            f"Z {device.position['Z']} pulse   R {device.position['R']} pulse   "
            f"Y {'Forward' if device.y_forward else 'Back'}")
        ran = device.lr_limit or device.lr_count or (device.busy and device.motion.phase == 16)
        limit = (device.lr_limit or "∞") if ran else "—"
        self.progress_label.setText(f"{device.lr_count} / {limit}")
        fault_active = device.error in FAULTS
        self.fault_choice.setEnabled(not fault_active)
        self.fault_button.setEnabled(not fault_active)
        self.clear_button.setEnabled(fault_active)
        self._update_fault_description()

    def closeEvent(self, event):
        self.timer.stop()
        self.server.stop()
        super().closeEvent(event)
