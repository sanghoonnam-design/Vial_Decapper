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
    QSpinBox,
    QTabWidget,
    QScrollArea,
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
            QMainWindow { background: #202a33; color: #e2e8ec; }
            QWidget#connection_bar {
                background: #131d25;
                border-bottom: 1px solid #354552;
            }
            QLabel#app_title { color: #f8fafc; font-size: 24px; font-weight: 700; }
            QLabel#brand_mark {
                background: #2b4b56;
                border: 1px solid #86aeb7;
                border-radius: 10px;
                color: #dceff2;
                font-size: 26px;
            }
            QGroupBox {
                background: #18222b;
                border: 1px solid #354552;
                border-radius: 10px;
                margin-top: 14px;
                padding: 12px;
                font-weight: 600;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
                color: #86aeb7;
            }
            QLineEdit, QPlainTextEdit {
                background: #111a21;
                border: 1px solid #354552;
                border-radius: 6px;
                padding: 7px;
                color: #e2e8ec;
                selection-background-color: #3d6975;
            }
            QLineEdit:focus { border-color: #6f9ca7; }
            QPlainTextEdit#communication_log {
                background: #0d151b;
                color: #d4e1e5;
                font-family: 'Consolas', 'Courier New', monospace;
            }
            QPushButton {
                background: #273540;
                border: 1px solid #4a5d6c;
                border-radius: 6px;
                color: #e2e8ec;
                font-weight: 600;
                padding: 8px 12px;
            }
            QPushButton#connect_button, QPushButton#command_send_button {
                background: #3d6975;
                color: #ffffff;
            }
            QPushButton:hover { background: #334653; color: #ffffff; }
            QPushButton#connect_button:hover, QPushButton#command_send_button:hover {
                background: #527f89;
            }
            QPushButton:disabled { background: #202a33; color: #68777d; border-color: #354552; }
            QComboBox, QSpinBox {
                background: #111a21; color: #e2e8ec;
                border: 1px solid #354552; border-radius: 5px; padding: 5px 7px;
            }
            QComboBox:focus, QSpinBox:focus { border-color: #6f9ca7; }
            QSplitter::handle { background: #354552; }
            QLabel#visual_caption { color: #aebdc5; font-size: 12px; font-weight: 600; }
            """
        )

        self.command_client = TcpCommandClient(self)
        self.serial_client = SerialCommandClient(self)
        self._receive_buffers = {"TCP": bytearray(), "UART": bytearray()}
        self._discarding_receive_line = set()
        self.command_client.connected.connect(self._on_connected)
        self.command_client.disconnected.connect(self._on_disconnected)
        self.command_client.received.connect(lambda data: self._log_received(data, "TCP"))
        self.command_client.sent.connect(lambda data: self._log_sent(data, "TCP"))
        self.command_client.error.connect(lambda message: self._log_error(f"[TCP] {message}"))
        self.serial_client.connected.connect(self._on_uart_connected)
        self.serial_client.disconnected.connect(self._on_uart_disconnected)
        self.serial_client.received.connect(lambda data: self._log_received(data, "UART"))
        self.serial_client.sent.connect(lambda data: self._log_sent(data, "UART"))
        self.serial_client.error.connect(lambda message: self._log_error(f"[UART] {message}"))

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
        input_area = QGroupBox("Teaching && Motion")
        input_area.setObjectName("input_area")
        input_area.setStyleSheet("""
            QGroupBox#input_area QLabel,
            QGroupBox#input_area QComboBox,
            QGroupBox#input_area QLineEdit,
            QGroupBox#input_area QPushButton { font-size: 14px; }
            QTabWidget::pane { border: 1px solid #354552; border-radius: 6px; }
            QTabBar::tab { background: #273540; color: #b7c5cb; padding: 8px 12px; border: 1px solid #354552; }
            QTabBar::tab:selected { background: #2b4b56; border-color: #6f9ca7; color: #dceff2; }
            QScrollArea { border: none; background: #18222b; }
            QWidget#motion_page { background: #18222b; }
            QSpinBox, QComboBox { background: #111a21; color: #e2e8ec;
                border: 1px solid #354552; border-radius: 4px; padding: 5px; }
            QPushButton:disabled { background: #202a33; color: #68777d; border-color: #354552; }
            QPushButton#motion_stop { background: #8f3f4a; color: white; }
            QPushButton#motion_stop:hover { background: #aa4b58; }
            QPushButton#motion_stop:disabled { background: #4a3037; color: #9b7a80; }
            QLabel { color: #aebdc5; }
            QLabel#move_command_label, QLabel#rpos_command_label {
                color: #86aeb7;
                font-weight: 700;
            }
            QLabel#current_z_position_value {
                color: #dceff2;
                font-size: 22px;
                font-weight: 700;
            }
        """)
        outer = QVBoxLayout(input_area)
        outer.setContentsMargins(10, 22, 10, 10)
        self.motion_buttons = []
        controls = QHBoxLayout()
        for title, command in (("정지 · STOP", "STOP"), ("일시정지", "PAUSE"), ("재개", "RESUME")):
            controls.addWidget(self._motion_button(title, command))
        outer.addLayout(controls)
        self.motion_tabs = QTabWidget()
        self.motion_tabs.setObjectName("motion_tabs")
        outer.addWidget(self.motion_tabs)

        def page(title):
            content = QWidget()
            content.setObjectName("motion_page")
            content.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
            rows = QVBoxLayout(content)
            rows.setContentsMargins(10, 10, 10, 10)
            rows.setSpacing(10)
            scroll = QScrollArea()
            scroll.setWidgetResizable(True)
            scroll.setWidget(content)
            self.motion_tabs.addTab(scroll, title)
            return rows

        def command_row(rows, title, commands):
            row = QHBoxLayout()
            row.addWidget(QLabel(title))
            for label, command in commands:
                row.addWidget(self._motion_button(label, command))
            rows.addLayout(row)

        automatic = page("Test")
        command_row(automatic, "원점", (("홈 찾기 · HOME", "HOME"), ("원위치 · ORG", "ORG")))
        command_row(automatic, "자동", (("캡 열기 · DECAP", "DECAP"), ("캡 닫기 · CAP", "CAP")))
        speed_row = QHBoxLayout()
        speed_row.addWidget(QLabel("속도"))
        self.speed_input = QSpinBox()
        self.speed_input.setObjectName("speed_input")
        self.speed_input.setRange(1, 100)
        self.speed_input.setValue(100)
        self.speed_input.setSuffix(" %")
        self.speed_input.setToolTip("설정할 속도 비율입니다. 적용을 눌러 전송합니다.")
        speed_row.addWidget(self.speed_input)
        speed_row.addWidget(self._motion_button("조회", "SPEED"))
        speed_row.addWidget(self._action_button("적용", "speed_apply_button", self._send_speed))
        automatic.addLayout(speed_row)
        command_row(automatic, "상태", (("상태 조회 · GSTA", "GSTA"),))
        note = QLabel("명령 응답과 BUSY·오류는 아래 통신 로그에서 확인합니다.")
        note.setWordWrap(True)
        automatic.addWidget(note)
        automatic.addStretch()

        layout = page("Teaching")

        move_row = QHBoxLayout()
        move_row.setSpacing(8)
        layout.addLayout(move_row)

        move_label = QLabel("MOVE")
        move_label.setObjectName("move_command_label")
        move_row.addWidget(move_label)

        mode_combo = QComboBox()
        mode_combo.setObjectName("move_mode_combo")
        # Firmware A is absolute position, not velocity.
        mode_combo.addItem("REL", "R")
        mode_combo.addItem("ABS", "A")
        mode_combo.setToolTip("REL: 상대 이동 / ABS: 절대 위치 이동")
        move_row.addWidget(mode_combo)

        axis_combo = QComboBox()
        axis_combo.setObjectName("move_axis_combo")
        axis_combo.addItem("Z", "Z")
        axis_combo.addItem("R", "R")
        move_row.addWidget(axis_combo)

        position_input = QLineEdit()
        position_input.setObjectName("move_position_input")
        position_input.setMaxLength(11)
        position_input.setPlaceholderText("Position (pulse)")
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
        command_row(layout, "EEPROM", (("티칭 저장 · SAVE", "SAVE"),))
        teaching_note = QLabel("Read로 읽은 Z 위치를 SAVE로 ZCap_UpPos에 저장합니다.\nSAVE는 장비 파라미터 전체를 EEPROM에 저장합니다.")
        teaching_note.setWordWrap(True)
        layout.addWidget(teaching_note)

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
        self.motion_buttons.extend((send_button, rpos_button))

        manual = page("Debug")
        command_row(manual, "Y축", (("High · READY 0", "READY 0"), ("Low · READY 1", "READY 1")))
        command_row(manual, "바디 그립", (("ON", "BGRIP 1"), ("OFF", "BGRIP 0")))
        command_row(manual, "캡 그립", (("ON", "CGRIP 1"), ("OFF", "CGRIP 0")))
        command_row(manual, "유닛 시험", (("UDECAP", "UDECAP"), ("UCAP", "UCAP")))
        command_row(manual, "반복 시험", (("시작 · LR", "LR"),))
        manual.addWidget(QLabel("반복 시험은 상단 STOP으로 종료합니다."))
        manual.addStretch()

        diagnostics = page("Status")
        command_row(diagnostics, "조회", (("SL", "SL"), ("CD", "CD"), ("PL", "PL")))
        diagnostics.addStretch()
        return input_area

    def _action_button(self, title, name, handler):
        button = QPushButton(title)
        button.setObjectName(name)
        button.setEnabled(False)
        button.setMinimumHeight(34)
        button.clicked.connect(handler)
        self.motion_buttons.append(button)
        return button

    def _motion_button(self, title, command):
        button = self._action_button(title, "motion_" + command.replace(" ", "_").lower(),
                                     lambda checked=False: self._send_motion_command(command))
        button.setToolTip(command)
        return button

    def _send_motion_command(self, command):
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command(command, self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked())

    def _send_speed(self):
        self._send_motion_command(f"SPEED {self.speed_input.value()}")

    def _set_command_controls_enabled(self, enabled):
        self.command_send_button.setEnabled(enabled)
        for button in self.motion_buttons:
            button.setEnabled(enabled)

    def _create_command_area(self) -> QWidget:
        command_area = QGroupBox("Command")
        command_area.setObjectName("command_area")
        layout = QVBoxLayout(command_area)

        transport_row = QHBoxLayout()
        transport_row.addWidget(QLabel("전송 경로"))
        self.transport_combo = QComboBox()
        self.transport_combo.setObjectName("transport_combo")
        self.transport_combo.addItem("TCP/IP", "TCP")
        self.transport_combo.addItem("UART", "UART")
        self.transport_combo.setToolTip("모든 명령 버튼에 적용됩니다. 연결이 끊겨도 다른 경로로 자동 전환하지 않습니다.")
        self.transport_combo.currentIndexChanged.connect(self._on_transport_changed)
        transport_row.addWidget(self.transport_combo)
        transport_note = QLabel("현재 펌웨어의 TCP 명령은 CR·LF 모두 체크해야 실행됩니다.")
        transport_note.setWordWrap(True)
        transport_row.addWidget(transport_note, 1)
        layout.addLayout(transport_row)

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
        communication_log.setMaximumBlockCount(10000)
        communication_log.setToolTip("최근 10,000줄을 표시·저장합니다. TX는 전송 버퍼 접수이며, 실행 결과는 RX에서 확인합니다.")
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
        ip_address_input.setText("192.168.0.")
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
        if not re.fullmatch(r"[+-]?[0-9]+", position):
            self._append_log("ERROR: Position must be an integer.")
            return

        if not -2147483648 <= int(position) <= 2147483647:
            self._log_error("Position must fit a signed 32-bit pulse value.")
            return

        mode = self.move_mode_combo.currentData()
        axis = self.move_axis_combo.currentData()
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command(f"MOVE {mode} {axis} {position}", self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked())

    def _send_rpos_command(self) -> None:
        sender = self._active_command_client()
        if sender is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return
        sender.send_command("RPOS", self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked())

    def _active_command_client(self):
        client = self.command_client if self.transport_combo.currentData() == "TCP" else self.serial_client
        return client if client.is_connected() else None

    def _on_transport_changed(self, _index):
        self.current_z_position_value.setText("--")
        self._set_command_controls_enabled(self._active_command_client() is not None)
        self._append_log(f"전송 경로: {self.transport_combo.currentText()}")

    def _reset_receive_state(self, source):
        self._receive_buffers[source].clear()
        self._discarding_receive_line.discard(source)
        if self.transport_combo.currentData() == source:
            self.current_z_position_value.setText("--")

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
        self._reset_receive_state("TCP")
        if self.transport_combo.currentData() == "TCP":
            self._set_command_controls_enabled(True)
        if self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Disconnect")
        self._append_log("[TCP] Connected.")

    def _on_disconnected(self) -> None:
        self.connect_button.setText("Connect")
        self._reset_receive_state("TCP")
        if self.transport_combo.currentData() == "TCP":
            self._set_command_controls_enabled(False)
        if not self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Connect")
        self._append_log("[TCP] Disconnected.")

    def _on_uart_connected(self) -> None:
        self.uart_connect_button.setText("UART Disconnect")
        self._reset_receive_state("UART")
        if self.transport_combo.currentData() == "UART":
            self._set_command_controls_enabled(True)
        self._append_log("UART connected.")

    def _on_uart_disconnected(self) -> None:
        self.uart_connect_button.setText("UART Connect")
        self._reset_receive_state("UART")
        if self.transport_combo.currentData() == "UART":
            self._set_command_controls_enabled(False)
        self._append_log("UART disconnected.")

    def _log_sent(self, payload: bytes, source="TCP") -> None:
        self._append_log(f"[{source}] TX: {self._render_bytes(payload)}")

    def _log_received(self, payload: bytes, source="TCP") -> None:
        # A readyRead event can contain part of a line or several lines.
        # Keep bytes until LF so split UTF-8 and CRLF sequences stay intact.
        parts = payload.split(b"\n")
        buffer = self._receive_buffers[source]
        for index, part in enumerate(parts):
            if source not in self._discarding_receive_line:
                if len(buffer) + len(part) > 65536:
                    buffer.clear()
                    self._discarding_receive_line.add(source)
                    self._log_error(f"[{source}] RX line exceeded 64 KiB; discarding until newline.")
                else:
                    buffer.extend(part)
            if index == len(parts) - 1:
                break
            if source in self._discarding_receive_line:
                self._discarding_receive_line.discard(source)
                buffer.clear()
                continue
            text = self._clean_received_text(bytes(buffer))
            buffer.clear()
            for line in text.splitlines():
                match = re.fullmatch(r"RPOS\s*[, ]\s*([+-]?\d+)\s*,?", line, re.IGNORECASE)
                if match and source == self.transport_combo.currentData():
                    self.current_z_position_value.setText(match.group(1))
                self._append_log(f"[{source}] RX: {line}")

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
            try:
                Path(filename).write_text(self.communication_log.toPlainText(), encoding="utf-8")
            except OSError as error:
                self._log_error(f"Log save failed: {error}")

    def _clear_communication_log(self) -> None:
        self.communication_log.clear()
