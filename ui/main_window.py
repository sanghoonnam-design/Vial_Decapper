from datetime import datetime
from pathlib import Path
import re

from PySide6.QtCore import Qt
from PySide6.QtGui import QFont
from PySide6.QtNetwork import QAbstractSocket
from PySide6.QtWidgets import (
    QFrame,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QCheckBox,
    QComboBox,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QSlider,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtSerialPort import QSerialPortInfo

from ui.serial_command_client import SerialCommandClient
from ui.device_model_view import DeviceModelView
from ui.tcp_command_client import TcpCommandClient


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Vial Decapper")
        self.setMinimumSize(800, 600)
        self.setStyleSheet(
            """
            QMainWindow { background: #0a1020; color: #e6edf7; }
            QWidget#connection_bar {
                background: #0e1930;
                border-bottom: 1px solid #223452;
            }
            QLabel#app_title { color: #f4f8ff; font-size: 24px; font-weight: 700; }
            QLabel#brand_mark {
                background: #102a47;
                border: 1px solid #38bdf8;
                border-radius: 10px;
                color: #67e8f9;
                font-size: 26px;
            }
            QGroupBox {
                background: #111c31;
                border: 1px solid #243858;
                border-radius: 10px;
                margin-top: 14px;
                padding: 12px;
                font-weight: 600;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
                color: #7dd3fc;
            }
            QLineEdit, QPlainTextEdit {
                background: #091323;
                border: 1px solid #2b4266;
                border-radius: 6px;
                padding: 7px;
                color: #dbeafe;
                selection-background-color: #2563eb;
            }
            QPushButton {
                background: #1b3150;
                border: none;
                border-radius: 6px;
                color: #cfe8ff;
                font-weight: 600;
                padding: 10px 16px;
            }
            QPushButton#connect_button, QPushButton#command_send_button {
                background: #0284c7;
                color: #f0f9ff;
            }
            QPushButton:hover { background: #27466f; }
            QPushButton#connect_button:hover, QPushButton#command_send_button:hover {
                background: #0ea5e9;
            }
            QSplitter::handle { background: #223452; }
            QLabel#visual_caption { color: #6b88ad; font-size: 12px; font-weight: 600; }
            """
        )

        self.command_client = TcpCommandClient(self)
        self.serial_client = SerialCommandClient(self)
        self.command_client.connected.connect(self._on_connected)
        self.command_client.disconnected.connect(self._on_disconnected)
        self.command_client.received.connect(self._log_received)
        self.command_client.sent.connect(self._log_sent)
        self.command_client.error.connect(self._log_error)
        self.serial_client.connected.connect(self._on_uart_connected)
        self.serial_client.disconnected.connect(self._on_uart_disconnected)
        self.serial_client.received.connect(self._log_received)
        self.serial_client.sent.connect(self._log_sent)
        self.serial_client.error.connect(self._log_error)

        self.setMenuWidget(self._create_connection_bar())
        self.setCentralWidget(self._create_workspace())

    def _create_workspace(self) -> QWidget:
        workspace = QWidget()
        layout = QVBoxLayout(workspace)
        layout.setContentsMargins(20, 0, 20, 20)
        layout.setSpacing(16)

        separator = QFrame()
        separator.setObjectName("connection_separator")
        separator.setFrameShape(QFrame.Shape.HLine)
        separator.setFrameShadow(QFrame.Shadow.Sunken)
        layout.addWidget(separator)

        content_splitter = QSplitter(Qt.Orientation.Horizontal)
        content_splitter.setObjectName("content_splitter")

        visual_area = QGroupBox("Vial Decapper Image")
        visual_area.setObjectName("visual_area")
        visual_layout = QVBoxLayout(visual_area)
        self.device_model_view = DeviceModelView()
        visual_layout.addWidget(self.device_model_view, 1)

        preview_row = QHBoxLayout()
        preview_row.addWidget(QLabel("이송 미리보기"))
        preview_row.addWidget(QLabel("뒤"))
        carriage_slider = QSlider(Qt.Orientation.Horizontal)
        carriage_slider.setObjectName("carriage_preview_slider")
        carriage_slider.setRange(0, 100)
        carriage_slider.setAccessibleName("하단 이송부 앞뒤 위치 미리보기")
        carriage_slider.setToolTip("3D 미리보기 전용 · 실제 장비는 움직이지 않습니다")
        carriage_slider.valueChanged.connect(self.device_model_view.set_carriage_position)
        preview_row.addWidget(carriage_slider, 1)
        preview_row.addWidget(QLabel("앞"))
        visual_layout.addLayout(preview_row)

        visual_caption = QLabel("간이 3D 모델 · 실제 치수와 다를 수 있습니다\n드래그: 회전 · 휠: 확대 · 더블클릭: 초기화")
        visual_caption.setWordWrap(True)
        visual_caption.setObjectName("visual_caption")
        visual_caption.setAlignment(Qt.AlignmentFlag.AlignCenter)
        visual_layout.addWidget(visual_caption)
        content_splitter.addWidget(visual_area)

        right_splitter = QSplitter(Qt.Orientation.Vertical)
        right_splitter.setObjectName("right_splitter")

        right_splitter.addWidget(self._create_teaching_point_area())
        right_splitter.addWidget(self._create_command_area())

        content_splitter.addWidget(right_splitter)
        content_splitter.setStretchFactor(0, 1)
        content_splitter.setStretchFactor(1, 1)
        content_splitter.setSizes([500, 500])
        right_splitter.setStretchFactor(0, 1)
        right_splitter.setStretchFactor(1, 1)
        right_splitter.setSizes([500, 500])
        layout.addWidget(content_splitter)

        return workspace

    def _create_teaching_point_area(self) -> QWidget:
        input_area = QGroupBox("Teaching Point")
        input_area.setObjectName("input_area")
        input_area.setStyleSheet("""
            QGroupBox#input_area QLabel,
            QGroupBox#input_area QComboBox,
            QGroupBox#input_area QLineEdit,
            QGroupBox#input_area QPushButton { font-size: 14px; }
            QLabel#move_command_label, QLabel#rpos_command_label {
                color: #7dd3fc;
                font-weight: 700;
            }
            QLabel#current_z_position_value {
                color: #e0f2fe;
                font-size: 22px;
                font-weight: 700;
            }
        """)
        layout = QVBoxLayout(input_area)
        layout.setContentsMargins(14, 26, 14, 18)
        layout.setSpacing(22)

        move_row = QHBoxLayout()
        move_row.setSpacing(8)
        layout.addLayout(move_row)

        move_label = QLabel("MOVE")
        move_label.setObjectName("move_command_label")
        move_row.addWidget(move_label)

        mode_combo = QComboBox()
        mode_combo.setObjectName("move_mode_combo")
        # Firmware MOVE accepts R (relative) or A (absolute).  Keep the
        # operator-facing labels requested by the UI and store protocol data
        # separately in the combo-box item data.
        mode_combo.addItem("REL", "R")
        mode_combo.addItem("VEL", "A")
        move_row.addWidget(mode_combo)

        axis_combo = QComboBox()
        axis_combo.setObjectName("move_axis_combo")
        axis_combo.addItem("Z", "Z")
        axis_combo.addItem("R", "R")
        move_row.addWidget(axis_combo)

        position_input = QLineEdit()
        position_input.setObjectName("move_position_input")
        position_input.setPlaceholderText("Position")
        position_input.setClearButtonEnabled(True)
        position_input.setMinimumWidth(80)
        move_row.addWidget(position_input, 1)

        send_button = QPushButton("Send")
        send_button.setObjectName("move_send_button")
        send_button.setEnabled(False)
        send_button.clicked.connect(self._send_teaching_point)
        move_row.addWidget(send_button)

        rpos_row = QHBoxLayout()
        rpos_row.setSpacing(8)
        layout.addLayout(rpos_row)

        rpos_label = QLabel("RPOS")
        rpos_label.setObjectName("rpos_command_label")
        rpos_row.addWidget(rpos_label)

        current_position = QLabel("Current Z Position")
        current_position.setObjectName("current_z_position_label")
        rpos_row.addWidget(current_position)

        current_position_value = QLabel("--")
        current_position_value.setObjectName("current_z_position_value")
        rpos_row.addWidget(current_position_value, 1)

        rpos_button = QPushButton("Read")
        rpos_button.setObjectName("rpos_read_button")
        rpos_button.setEnabled(False)
        rpos_button.clicked.connect(self._send_rpos_command)
        rpos_row.addWidget(rpos_button)

        for label in (move_label, rpos_label):
            label.setMinimumWidth(48)
        for control in (mode_combo, axis_combo, position_input, send_button, rpos_button):
            control.setMinimumHeight(42)
        for button in (send_button, rpos_button):
            button.setMinimumWidth(72)
        current_position_value.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addStretch(1)

        self.move_mode_combo = mode_combo
        self.move_axis_combo = axis_combo
        self.move_position_input = position_input
        self.move_send_button = send_button
        self.current_z_position_label = current_position
        self.current_z_position_value = current_position_value
        self.rpos_read_button = rpos_button
        return input_area

    def _create_command_area(self) -> QWidget:
        command_area = QGroupBox("Command")
        command_area.setObjectName("command_area")
        layout = QVBoxLayout(command_area)

        command_row = QHBoxLayout()

        command_input = QLineEdit()
        self.command_input = command_input
        command_input.setObjectName("command_input")
        command_input.setPlaceholderText("Enter command")
        command_input.setMaximumHeight(32)
        command_row.addWidget(command_input)

        command_send_button = QPushButton("Send")
        self.command_send_button = command_send_button
        command_send_button.setObjectName("command_send_button")
        command_send_button.setEnabled(False)
        command_row.addWidget(command_send_button)

        save_button = QPushButton("Save")
        save_button.setObjectName("log_save_button")
        save_button.clicked.connect(self._save_communication_log)
        command_row.addWidget(save_button)

        clear_button = QPushButton("Clear")
        clear_button.setObjectName("log_clear_button")
        clear_button.clicked.connect(self._clear_communication_log)
        command_row.addWidget(clear_button)
        layout.addLayout(command_row)

        communication_log = QPlainTextEdit()
        self.communication_log = communication_log
        communication_log.setObjectName("communication_log")
        communication_log.setReadOnly(True)
        communication_log.setPlaceholderText("TX / RX communication log")
        layout.addWidget(communication_log)

        command_send_button.clicked.connect(self._send_command)
        command_input.returnPressed.connect(self._send_command)

        return command_area

    def _create_connection_bar(self) -> QWidget:
        connection_bar = QWidget()
        connection_bar.setObjectName("connection_bar")
        layout = QHBoxLayout(connection_bar)
        layout.setContentsMargins(20, 14, 20, 14)
        layout.setSpacing(10)

        title_label = QLabel("Vial Decapper")
        title_label.setObjectName("app_title")
        title_label.setFont(QFont("Segoe UI", 22, QFont.Weight.Bold))
        brand_mark = QLabel("◈")
        brand_mark.setObjectName("brand_mark")
        brand_mark.setAlignment(Qt.AlignmentFlag.AlignCenter)
        brand_mark.setFixedSize(40, 40)
        layout.addWidget(brand_mark)
        layout.addWidget(title_label)
        layout.addStretch()

        layout.addWidget(QLabel("TCP/IP"))

        ip_address_input = QLineEdit()
        ip_address_input.setObjectName("ip_address_input")
        ip_address_input.setPlaceholderText("IP Address")
        ip_address_input.setMaximumWidth(150)
        layout.addWidget(ip_address_input)

        port_input = QLineEdit()
        self.port_input = port_input
        port_input.setObjectName("port_input")
        port_input.setPlaceholderText("Port")
        port_input.setText("8000")
        port_input.setMaximumWidth(90)
        layout.addWidget(port_input)

        connect_button = QPushButton("Connect")
        self.connect_button = connect_button
        connect_button.setObjectName("connect_button")
        layout.addWidget(connect_button)

        cr_checkbox = QCheckBox("CR")
        cr_checkbox.setObjectName("cr_checkbox")
        cr_checkbox.setChecked(True)
        layout.addWidget(cr_checkbox)

        lf_checkbox = QCheckBox("LF")
        lf_checkbox.setObjectName("lf_checkbox")
        lf_checkbox.setChecked(True)
        layout.addWidget(lf_checkbox)

        self.cr_checkbox = cr_checkbox
        self.lf_checkbox = lf_checkbox
        connect_button.clicked.connect(self._toggle_connection)

        layout.addWidget(QLabel("UART"))
        serial_port_combo = QComboBox()
        self.serial_port_combo = serial_port_combo
        serial_port_combo.setObjectName("serial_port_combo")
        serial_port_combo.setMinimumWidth(100)
        for info in QSerialPortInfo.availablePorts():
            serial_port_combo.addItem(info.portName())
        if serial_port_combo.count() == 0:
            serial_port_combo.addItem("COM1")
        layout.addWidget(serial_port_combo)

        serial_baud_combo = QComboBox()
        self.serial_baud_combo = serial_baud_combo
        serial_baud_combo.setObjectName("serial_baud_combo")
        for baud in (9600, 19200, 38400, 57600, 115200):
            serial_baud_combo.addItem(str(baud), baud)
        serial_baud_combo.setCurrentText("115200")
        layout.addWidget(serial_baud_combo)

        uart_connect_button = QPushButton("UART Connect")
        self.uart_connect_button = uart_connect_button
        uart_connect_button.setObjectName("uart_connect_button")
        layout.addWidget(uart_connect_button)
        uart_connect_button.clicked.connect(self._toggle_uart_connection)

        return connection_bar

    def _toggle_connection(self) -> None:
        if self.command_client.socket.state() != QAbstractSocket.SocketState.UnconnectedState:
            self.command_client.disconnect_from_host()
            return
        host = self.findChild(QLineEdit, "ip_address_input").text().strip()
        if not host:
            self._append_log("ERROR: Enter an IP address.")
            return
        try:
            port = int(self.port_input.text())
            if not 1 <= port <= 65535:
                raise ValueError
        except ValueError:
            self._append_log("ERROR: Port must be between 1 and 65535.")
            return
        self.command_client.connect_to_host(host, port)

    def _send_command(self) -> None:
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command(self.command_input.text(), self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked())

    def _send_teaching_point(self) -> None:
        position = self.move_position_input.text().strip()
        if not position or not position.lstrip("+-").isdigit():
            self._append_log("ERROR: Position must be an integer.")
            return

        mode = self.move_mode_combo.currentData()
        axis = self.move_axis_combo.currentData()
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command(f"MOVE {mode} {axis} {position}", True, True)

    def _send_rpos_command(self) -> None:
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command("RPOS", True, True)

    def _active_command_client(self):
        if self.command_client.is_connected():
            return self.command_client
        if self.serial_client.is_connected():
            return self.serial_client
        return None

    def _toggle_uart_connection(self) -> None:
        if self.serial_client.is_connected():
            self.serial_client.close()
            return
        port_name = self.serial_port_combo.currentText().strip()
        baud_rate = int(self.serial_baud_combo.currentData())
        self.serial_client.open(port_name, baud_rate)

    def _append_log(self, message: str) -> None:
        self.communication_log.appendPlainText(message)

    def _on_connected(self) -> None:
        self.connect_button.setText("Disconnect")
        self.command_send_button.setEnabled(True)
        self.move_send_button.setEnabled(True)
        self.rpos_read_button.setEnabled(True)
        if self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Disconnect")
        self._append_log("Connected.")

    def _on_disconnected(self) -> None:
        self.connect_button.setText("Connect")
        self.command_send_button.setEnabled(False)
        self.move_send_button.setEnabled(False)
        self.rpos_read_button.setEnabled(False)
        if not self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Connect")
        self._append_log("Disconnected.")

    def _on_uart_connected(self) -> None:
        self.uart_connect_button.setText("UART Disconnect")
        self.command_send_button.setEnabled(True)
        self.move_send_button.setEnabled(True)
        self.rpos_read_button.setEnabled(True)
        self._append_log("UART connected.")

    def _on_uart_disconnected(self) -> None:
        self.uart_connect_button.setText("UART Connect")
        if not self.command_client.is_connected():
            self.command_send_button.setEnabled(False)
            self.move_send_button.setEnabled(False)
            self.rpos_read_button.setEnabled(False)
        self._append_log("UART disconnected.")

    def _log_sent(self, payload: bytes) -> None:
        self._append_log(f"TX: {self._render_bytes(payload)}")

    def _log_received(self, payload: bytes) -> None:
        text = self._clean_received_text(payload)
        if text:
            match = re.search(r"\bRPOS\s*[, ]\s*(-?\d+)", text, re.IGNORECASE)
            if match:
                self.current_z_position_value.setText(match.group(1))
            self._append_log(f"RX: {text}")

    def _log_error(self, message: str) -> None:
        self._append_log(f"ERROR: {message}")

    @staticmethod
    def _render_bytes(payload: bytes) -> str:
        return payload.decode("ascii", errors="backslashreplace").replace("\r", "\\r").replace("\n", "\\n")

    @staticmethod
    def _clean_received_text(payload: bytes) -> str:
        text = payload.decode("utf-8", errors="replace")
        console_noise = "RND>" in text or "\x1b[" in text
        # The firmware's serial console emits ANSI CSI commands while it
        # redraws the prompt (for example ESC[2K = erase line).
        text = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", text)
        lines = []
        for line in text.replace("\r\n", "\n").replace("\r", "\n").split("\n"):
            stripped = line.strip()
            if not stripped or stripped.startswith("RND>") or stripped in {"[2K", "[K"}:
                continue
            # Ignore the one-character-at-a-time command echo from the CLI.
            if console_noise and len(stripped) == 1 and stripped.isprintable():
                continue
            lines.append(line.strip())
        return "\n".join(lines)

    def _save_communication_log(self) -> None:
        default_name = f"vial_decapper_log_{datetime.now():%Y%m%d_%H%M%S}.txt"
        filename, _ = QFileDialog.getSaveFileName(self, "Save communication log", default_name, "Text files (*.txt)")
        if filename:
            Path(filename).write_text(self.communication_log.toPlainText(), encoding="utf-8")

    def _clear_communication_log(self) -> None:
        self.communication_log.clear()
