from datetime import datetime
from pathlib import Path
import re

from PySide6.QtCore import Qt, QTimer
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
    QMessageBox,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QHeaderView,
    QTableWidget,
    QAbstractItemView,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtSerialPort import QSerialPortInfo

from ui.device_controls import DeviceControls
from ui.device_session import DeviceSession, PARAMETERS
from ui.serial_command_client import SerialCommandClient
from ui.device_model_view import DeviceModelView
from ui.tcp_command_client import TcpCommandClient


class MainWindow(DeviceControls, QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Vial Decapper")
        self.setMinimumSize(1000, 720)
        self.setStyleSheet(
            """
            QMainWindow { background: #202a33; color: #e2e8ec; }
            QLabel, QCheckBox { color: #aebdc5; }
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
            QTableWidget#diagnostic_table {
                background: #111a21; alternate-background-color: #19262f;
                color: #e2e8ec; gridline-color: #273742;
                border: 1px solid #354552; border-radius: 5px;
                selection-background-color: #3d6975;
            }
            QTableWidget#diagnostic_table QHeaderView::section {
                background: #273540; color: #bcd8df; padding: 3px 7px;
                border: 0; border-bottom: 1px solid #354552;
            }
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

        self.sessions = {
            source: DeviceSession(lambda command, source=source: self._send_to_route(source, command),
                                  report=lambda message, source=source: self._log_error(f"[{source}] {message}"))
            for source in ("TCP", "UART")
        }
        self._primed_routes = set()
        self._displayed_speed = None
        self._displayed_parameter = None
        self.setMenuWidget(self._create_connection_bar())
        self.setCentralWidget(self._create_workspace())
        self.session_timer = QTimer(self)
        self.session_timer.setInterval(100)
        self.session_timer.timeout.connect(self._tick_sessions)
        self.session_timer.start()
        self._refresh_device_controls()

    def _create_workspace(self) -> QWidget:
        workspace = QWidget()
        layout = QVBoxLayout(workspace)
        layout.setContentsMargins(14, 0, 14, 14)
        layout.setSpacing(8)

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
        carriage_slider = self.carriage_slider = QSlider(Qt.Orientation.Horizontal)
        carriage_slider.setObjectName("carriage_preview_slider")
        carriage_slider.setRange(0, 100)
        carriage_slider.setAccessibleName("하단 이송부 앞뒤 위치 미리보기")
        carriage_slider.setToolTip("3D 미리보기 전용 · 실제 장비는 움직이지 않습니다")
        carriage_slider.valueChanged.connect(self.device_model_view.set_carriage_position)
        preview_row.addWidget(carriage_slider, 1)
        preview_row.addWidget(QLabel("앞"))
        visual_layout.addLayout(preview_row)
        self.live_model_check = QCheckBox("장비 상태 보기 · 읽기 전용")
        self.live_model_check.toggled.connect(self._refresh_device_controls)
        visual_layout.addWidget(self.live_model_check)
        self.live_model_label = self._compact_label("Y / 캡: 확인 안 됨")
        self.live_model_label.hide()
        visual_layout.addWidget(self.live_model_label)

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
        right_splitter.setChildrenCollapsible(False)
        right_splitter.setSizes([600, 400])
        # Apply the ratio after Qt has calculated the actual page minimum sizes.
        QTimer.singleShot(0, lambda: right_splitter.setSizes(
            [int(right_splitter.height()*0.6), int(right_splitter.height()*0.4)]))
        layout.addWidget(content_splitter)

        return workspace

    def _create_teaching_point_area(self) -> QWidget:
        input_area = QGroupBox("Teaching && Motion")
        input_area.setObjectName("input_area")
        input_area.setStyleSheet("""
            QGroupBox#input_area { padding: 6px; }
            QGroupBox#input_area QLabel,
            QGroupBox#input_area QComboBox,
            QGroupBox#input_area QLineEdit,
            QGroupBox#input_area QPushButton { font-size: 13px; }
            QGroupBox#input_area QPushButton { padding: 4px 9px; }
            QTabWidget::pane { border: 1px solid #354552; border-radius: 6px; }
            QTabBar::tab { background: #273540; color: #b7c5cb; padding: 5px 12px; border: 1px solid #354552; }
            QTabBar::tab:selected { background: #2b4b56; border-color: #6f9ca7; color: #dceff2; }
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
        outer.setContentsMargins(8, 16, 8, 8)
        outer.setSpacing(4)
        self.motion_buttons = []
        self.device_state_label = QLabel("연결 후 장비 상태를 확인합니다.")
        self.device_state_label.setWordWrap(False)
        self.device_state_label.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Preferred)
        self.device_state_label.setFont(QFont("Segoe UI", 10))
        self.device_state_label.setStyleSheet("font-size: 11px; color: #8fa7b3;")
        outer.addWidget(self.device_state_label)
        self.device_detail_label = self._compact_label("")
        query_row = QHBoxLayout()
        for title, name, command in (("F/W", "device_version", "VERSION"),
                ("상태", "device_state", "GSTA"), ("오류 상세", "device_error", "GERD"),
                ("오류 해제", "device_clear", "CLR")):
            button = self._action_button(title, name, lambda checked=False, command=command: self._send_motion_command(command))
            button.setProperty("device_command", command)
            query_row.addWidget(button)
        outer.addLayout(query_row)
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
            rows.setContentsMargins(8, 8, 8, 8)
            rows.setSpacing(4)
            self.motion_tabs.addTab(content, title)
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
        self.auto_condition_label = self._compact_label("자동 운전 조건: 확인 안 됨")
        self.operation_label = self._compact_label("최근 동작: --")
        automatic.addWidget(self.auto_condition_label)
        automatic.addWidget(self.operation_label)
        automatic.addWidget(self.device_detail_label)
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

        rpos_label = QLabel("GPOS")
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
        teach_row = QHBoxLayout()
        self.teach_target_combo = QComboBox()
        for title, offset in (("Z 캡 위 위치",16),("Z 캡 옆 위치",17),("Z 원위치",18),("R 회전량",21)):
            self.teach_target_combo.addItem(title, offset)
        self.teach_value_input = QLineEdit()
        self.teach_value_input.setPlaceholderText("pulse")
        teach_row.addWidget(self.teach_target_combo)
        teach_row.addWidget(self.teach_value_input,1)
        teach_row.addWidget(self._action_button("적용", "teach_apply", self._apply_teach))
        layout.addLayout(teach_row)
        save_row = QHBoxLayout()
        save_row.addWidget(self._action_button("Z 읽은 값 복사", "teach_capture", self._capture_teach))
        save_row.addWidget(self._action_button("EEPROM 저장", "motion_save", self._save_device_parameters))
        layout.addLayout(save_row)
        teaching_note = QLabel("위치 조회는 설정을 변경하지 않습니다. 티칭 적용 후 읽기로 검증합니다.\nEEPROM 저장은 장비 파라미터 전체를 저장합니다. 구형 F/W의 RPOS는 저장 후보도 변경합니다.")
        teaching_note.setWordWrap(True)
        self.teach_target_combo.setToolTip(teaching_note.text())
        self.rpos_read_button = rpos_button
        rpos_button.setToolTip(teaching_note.text())
        parameter_row = QHBoxLayout()
        self.parameter_combo = QComboBox()
        for offset,(title,unit) in enumerate(PARAMETERS):
            self.parameter_combo.addItem(f"{title} ({unit})" if unit else title,offset)
        self.parameter_value_input = QLineEdit()
        self.parameter_value_input.setPlaceholderText("장비 값 조회 후 입력")
        parameter_row.addWidget(self.parameter_combo,1)
        parameter_row.addWidget(self.parameter_value_input,1)
        layout.addLayout(parameter_row)
        parameter_actions = QHBoxLayout()
        parameter_actions.addWidget(self._action_button("파라미터 읽기", "parameter_read", self._read_parameter))
        parameter_actions.addWidget(self._action_button("파라미터 적용", "parameter_apply", self._apply_parameter))
        layout.addLayout(parameter_actions)
        self.setting_status_label = self._compact_label("설정: RAM 확인 후 EEPROM에 저장")
        layout.addWidget(self.setting_status_label)
        parameter_note = QLabel("전류·분해능 변경은 저장 후 재기동/LOAD와 HOME이 필요할 수 있습니다.")
        parameter_note.setWordWrap(True)
        self.parameter_combo.setToolTip(parameter_note.text())
        self.parameter_combo.currentIndexChanged.connect(self._refresh_device_controls)

        for label in (move_label, rpos_label):
            label.setMinimumWidth(48)
        for control in (mode_combo, axis_combo, position_input, send_button, rpos_button):
            control.setMinimumHeight(32)
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
        self.move_axis_combo.currentIndexChanged.connect(self._refresh_device_controls)

        manual = page("Debug")
        command_row(manual, "Y축", (("F · READY 0", "READY 0"), ("B · READY 1", "READY 1")))
        command_row(manual, "바디 그립", (("ON", "BGRIP 1"), ("OFF", "BGRIP 0")))
        command_row(manual, "캡 그립", (("ON", "CGRIP 1"), ("OFF", "CGRIP 0")))
        command_row(manual, "유닛 시험", (("UDECAP", "UDECAP"), ("UCAP", "UCAP")))
        lr_row = QHBoxLayout()
        lr_row.addWidget(QLabel("반복 횟수"))
        self.longrun_count_input = QSpinBox()
        self.longrun_count_input.setRange(1,1000000)
        self.longrun_count_input.setValue(100)
        lr_row.addWidget(self.longrun_count_input)
        lr_row.addWidget(self._action_button("횟수 시작", "motion_lr", self._start_longrun))
        unlimited_button = self._action_button("무한 시작", "motion_lr_unlimited",
            lambda checked=False: self._send_motion_command("LR"))
        unlimited_button.setProperty("device_command", "LR")
        unlimited_button.setToolTip("LR만 전송합니다. STOP 또는 오류가 발생할 때까지 반복합니다.")
        lr_row.addWidget(unlimited_button)
        manual.addLayout(lr_row)
        self.longrun_status_label = self._compact_label("완료 횟수: --")
        manual.addWidget(self.longrun_status_label)
        debug_actions = QHBoxLayout()
        debug_actions.addWidget(self._motion_button("센서 조회", "GSENSOR"))
        self.longrun_export_button = QPushButton("Longrun 기록 저장")
        self.longrun_export_button.setMinimumHeight(28)
        self.longrun_export_button.setEnabled(False)
        self.longrun_export_button.clicked.connect(self._export_longrun)
        debug_actions.addWidget(self.longrun_export_button)
        manual.addLayout(debug_actions)
        self.sensor_status_label = self._compact_label("센서 상태: 확인 안 됨")
        manual.addWidget(self.sensor_status_label)
        self.grip_status_label = self._compact_label("캡 / 바이알: 확인 안 됨")
        manual.addWidget(self.grip_status_label)
        longrun_note = QLabel("설정 횟수의 CAP까지 완료하면 종료합니다. STOP으로 중단할 수 있습니다.")
        longrun_note.setWordWrap(True)
        self.longrun_count_input.setToolTip(longrun_note.text())
        manual.addStretch()

        diagnostics = page("Status")
        command_row(diagnostics, "조회", (("SL", "SL"), ("CD", "CD"), ("PL", "PL")))
        self.diagnostic_label = self._compact_label("SL 상태 · CD 제어값 · PL 설정")
        diagnostics.addWidget(self.diagnostic_label)
        self.diagnostic_table = QTableWidget(0, 2)
        self.diagnostic_table.setObjectName("diagnostic_table")
        self.diagnostic_table.setHorizontalHeaderLabels(("항목", "값"))
        self.diagnostic_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.diagnostic_table.verticalHeader().hide()
        self.diagnostic_table.verticalHeader().setDefaultSectionSize(20)
        self.diagnostic_table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.diagnostic_table.setMinimumHeight(100)
        self.diagnostic_table.setAlternatingRowColors(True)
        diagnostics.addWidget(self.diagnostic_table, 1)
        return input_area

    def _compact_label(self, text):
        label = QLabel(text)
        label.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
        label.setStyleSheet("font-size: 11px;")
        return label

    def _action_button(self, title, name, handler):
        button = QPushButton(title)
        button.setObjectName(name)
        button.setEnabled(False)
        button.setMinimumHeight(28)
        button.clicked.connect(handler)
        self.motion_buttons.append(button)
        return button

    def _motion_button(self, title, command):
        button = self._action_button(title, "motion_" + command.replace(" ", "_").lower(),
                                     lambda checked=False: self._send_motion_command(command))
        button.setToolTip(command)
        button.setProperty("device_command", command)
        return button

    def _send_motion_command(self, command):
        if self._active_command_client() is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return False
        result = self._selected_session().request(command)
        self._refresh_device_controls()
        return result

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
        self._send_motion_command(self.command_input.text())

    def _send_teaching_point(self) -> None:
        position = self.move_position_input.text().strip()
        if not re.fullmatch(r"[+-]?[0-9]+", position):
            self._log_error("Position must be an integer.")
            return
        value = int(position)
        if not -2147483648 <= value <= 2147483647:
            self._log_error("Position must fit a signed 32-bit pulse value.")
            return
        mode,axis = self.move_mode_combo.currentData(),self.move_axis_combo.currentData()
        session = self._selected_session()
        if session.extended:
            target = value
            if mode == "R":
                if axis not in session.positions or session.clock()-session.position_at[axis]>3:
                    self._log_error("상대 이동 전에 현재 축 위치를 Read로 확인하세요.")
                    return
                target += session.positions[axis]
            if not session.target_in_range(axis,target):
                self._log_error("장비 리미트 밖이거나 리미트를 확인하지 못했습니다.")
                return
        self._send_motion_command(f"MOVE {mode} {axis} {value}")

    def _send_rpos_command(self) -> None:
        session = self._selected_session()
        axis = self.move_axis_combo.currentData()
        if session.extended:
            self._send_motion_command(f"GPOS {axis}")
        elif axis == "Z":
            self._log_error("구형 F/W: RPOS는 Z 티칭 저장 후보도 변경합니다.")
            self._send_motion_command("RPOS")
        else:
            self._log_error("R 위치 조회에는 확장 F/W가 필요합니다.")

    def _active_command_client(self):
        client = self.command_client if self.transport_combo.currentData() == "TCP" else self.serial_client
        return client if client.is_connected() else None

    def _on_transport_changed(self, _index):
        self.current_z_position_value.setText("--")
        self._displayed_speed = None
        self._displayed_parameter = None
        self._refresh_device_controls()
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
        timestamp = datetime.now().time().isoformat(timespec="milliseconds")
        self.communication_log.appendPlainText(f"[{timestamp}] {message}")

    def _on_connected(self) -> None:
        self.connect_button.setText("Disconnect")
        self._reset_receive_state("TCP")
        if self.transport_combo.currentData() == "TCP":
            self._set_command_controls_enabled(True)
        if self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Disconnect")
        self._append_log("[TCP] Connected.")
        self._primed_routes.discard("TCP")
        self.sessions["TCP"].start()
        self._refresh_device_controls()

    def _on_disconnected(self) -> None:
        self.connect_button.setText("Connect")
        self._reset_receive_state("TCP")
        if self.transport_combo.currentData() == "TCP":
            self._set_command_controls_enabled(False)
        if not self.serial_client.is_connected():
            self.uart_connect_button.setText("UART Connect")
        self._append_log("[TCP] Disconnected.")
        self.sessions["TCP"].stop()
        self._primed_routes.discard("TCP")
        self._refresh_device_controls()

    def _on_uart_connected(self) -> None:
        self.uart_connect_button.setText("UART Disconnect")
        self._reset_receive_state("UART")
        if self.transport_combo.currentData() == "UART":
            self._set_command_controls_enabled(True)
        self._append_log("UART connected.")
        self._primed_routes.discard("UART")
        self.sessions["UART"].start()
        self._refresh_device_controls()

    def _on_uart_disconnected(self) -> None:
        self.uart_connect_button.setText("UART Connect")
        self._reset_receive_state("UART")
        if self.transport_combo.currentData() == "UART":
            self._set_command_controls_enabled(False)
        self._append_log("UART disconnected.")
        self.sessions["UART"].stop()
        self._primed_routes.discard("UART")
        self._refresh_device_controls()

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
                self.sessions[source].receive(line)
                self._prime_device_parameters(source)
                if source == self.transport_combo.currentData():
                    self._refresh_device_controls()

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
