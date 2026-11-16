from datetime import datetime
from pathlib import Path
import re
import ipaddress

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont
from PySide6.QtNetwork import QAbstractSocket
from PySide6.QtWidgets import (
    QFrame,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QGridLayout,
    QCheckBox,
    QComboBox,
    QLabel,
    QLineEdit,
    QMainWindow,
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

from ui.command_wire import ResponseStream
from ui.device_controls import DeviceControls
from ui.device_session import DeviceSession, PARAMETERS, TEACH_OFFSETS
from ui.serial_command_client import SerialCommandClient
from ui.device_model_view import DeviceModelView
from ui.tcp_command_client import TcpCommandClient
from ui.longrun_panel import LongrunPanel, MODULE_PRESETS
from ui.position_actions import PositionActions
from ui.theme import WINDOW_STYLE


class MainWindow(DeviceControls, QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Vial Decapper")
        self.setMinimumSize(1000, 820)
        self.setStyleSheet(WINDOW_STYLE)

        self.command_client = TcpCommandClient(self)
        self.serial_client = SerialCommandClient(self)
        self._responses = {
            source: ResponseStream(lambda source=source: self._log_error(
                f"[{source}] RX line exceeded 64 KiB; discarding until newline."))
            for source in ("TCP", "UART")
        }
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
        self.position_actions = {source: PositionActions(session,
            report=lambda message, source=source: self._append_log(f"[{source}] {message}"))
            for source, session in self.sessions.items()}
        self._primed_routes = set()
        self._displayed_speed = None
        self._displayed_parameter = None
        self._teaching_read = None
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
        separator.setFrameShadow(QFrame.Shadow.Plain)
        separator.setFixedHeight(1)
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

        visual_caption = QLabel("간이 3D 모델 · 실제 치수와 다를 수 있습니다")
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
        outer = QVBoxLayout(input_area)
        outer.setContentsMargins(8, 16, 8, 8)
        outer.setSpacing(4)
        self.motion_buttons = []
        self.device_state_label = QLabel("연결 후 장비 상태를 확인합니다.")
        self.device_state_label.setObjectName("device_state")
        self.device_state_label.setWordWrap(False)
        self.device_state_label.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Preferred)
        self.device_state_label.setMinimumHeight(24)
        outer.addWidget(self.device_state_label)
        self.device_detail_label = QLabel("")
        self.device_detail_label.setObjectName("device_fault")
        self.device_detail_label.setTextFormat(Qt.TextFormat.PlainText)
        self.device_detail_label.setWordWrap(True)
        self.device_detail_label.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Minimum)
        outer.addWidget(self.device_detail_label)
        controls = QHBoxLayout()
        controls.setSpacing(5)
        for title, command in (("정지 · STOP", "STOP"), ("일시정지", "PAUSE"), ("재개", "RESUME")):
            button = self._motion_button(title, command)
            button.setProperty("danger", command == "STOP")
            controls.addWidget(button, 2 if command == "STOP" else 1)
        for title, name, command in (("오류 상세", "device_error", "GERD"),
                ("오류 해제", "device_clear", "CLR")):
            button = self._action_button(title, name, lambda checked=False, command=command: self._send_motion_command(command))
            button.setProperty("device_command", command)
            controls.addWidget(button, 1)
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
                button = self._motion_button(label, command)
                if len(commands) > 2:
                    button.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
                row.addWidget(button)
            rows.addLayout(row)

        automatic = page("Test")
        automatic.setSpacing(2)
        grid = QGridLayout()
        grid.setHorizontalSpacing(4)
        grid.setVerticalSpacing(2)
        grid.setColumnMinimumWidth(0, 58)
        for column in range(1, 5):
            grid.setColumnStretch(column, 1)
        automatic.addLayout(grid)

        def test_button(button):
            button.setFixedHeight(30)
            button.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
            button.setStyleSheet("font-size:12px; padding:3px 4px;")
            return button

        def test_row(index, title, commands):
            grid.addWidget(QLabel(title), index, 0)
            for column, (caption, command) in enumerate(commands, 1):
                button = test_button(self._motion_button(caption, command))
                button.setToolTip({"HOME": "홈 찾기 · HOME", "ORG": "원위치 · ORG",
                                   "DECAP": "캡 열기 · DECAP", "CAP": "캡 닫기 · CAP"}.get(command, command))
                grid.addWidget(button, index, column)

        test_row(0, "축 이동",
                    (("Y Forward", "READY 0"), ("Y Backward", "READY 1")))
        for column, (caption, name, offset) in enumerate(
                (("원점 이동", "axis_origin", None), ("CAP UP", "axis_cap_up", 16),
                 ("CAP SIDE", "axis_cap_side", 17)), 1):
            button = self._action_button(caption, name,
                lambda checked=False, offset=offset: self._start_position_action(offset))
            button.setProperty("position_action", True)
            button.setToolTip("현재 좌표계에서 Z=0 후 R=0 이동 · HOME 탐색 없음" if offset is None
                              else "F/W 설정 좌표를 읽어 Z축 절대이동 · 설정/EEPROM 변경 없음")
            grid.addWidget(test_button(button), 1, column)
        test_row(2, "그리퍼",
                    (("바디 ON", "BGRIP 1"), ("바디 OFF", "BGRIP 0"),
                     ("캡 ON", "CGRIP 1"), ("캡 OFF", "CGRIP 0")))
        grid.addWidget(QLabel("속도"), 3, 0)
        self.speed_input = QSpinBox()
        self.speed_input.setObjectName("speed_input")
        self.speed_input.setRange(1, 100)
        self.speed_input.setValue(100)
        self.speed_input.setSuffix(" %")
        self.speed_input.setToolTip("설정할 속도 비율입니다. 적용을 눌러 전송합니다.")
        self.speed_input.setFixedHeight(30)
        self.speed_input.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
        grid.addWidget(self.speed_input, 3, 1)
        grid.addWidget(test_button(self._motion_button("조회", "SPEED")), 3, 2)
        grid.addWidget(test_button(self._action_button("적용", "speed_apply_button", self._send_speed)), 3, 3)
        grid.setRowMinimumHeight(4, 12)
        test_row(5, "유닛 시험", (("UDECAP", "UDECAP"), ("UCAP", "UCAP")))
        grid.addWidget(test_button(self._motion_button("센서 조회", "GSENSOR")), 6, 1)
        self.sensor_status_label = self._compact_label("센서 상태: 확인 안 됨")
        grid.addWidget(self.sensor_status_label, 6, 2, 1, 3)
        grid.addWidget(test_button(self._motion_button("상태 조회", "GSTA")), 7, 1)
        self.grip_status_label = self._compact_label("캡 / 바이알: 확인 안 됨")
        grid.addWidget(self.grip_status_label, 7, 2, 1, 3)
        test_row(8, "자동", (("DECAP", "DECAP"), ("CAP", "CAP"), ("HOME", "HOME"), ("ORG", "ORG")))
        self.auto_condition_label = self._compact_label("자동 운전 조건: 확인 안 됨")
        self.operation_label = self._compact_label("최근 동작: --")
        automatic.addWidget(self.auto_condition_label)
        automatic.addWidget(self.operation_label)
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
        for title, offset in (("Z 캡 위 위치",16),("Z 캡 옆 위치",17),("Z 원위치",18),("R DECAP 설정값",21)):
            self.teach_target_combo.addItem(title, offset)
        self.teach_value_input = QLineEdit()
        self.teach_value_input.setPlaceholderText("pulse")
        teach_row.addWidget(self.teach_target_combo)
        teach_row.addWidget(self.teach_value_input,1)
        teach_row.addWidget(self._action_button("적용", "teach_apply", self._apply_teach))
        layout.addLayout(teach_row)
        save_row = QHBoxLayout()
        for axis in ("Z", "R"):
            button = self._action_button(f"SAVE {axis}", f"teach_save_{axis.lower()}",
                                         lambda checked=False, axis=axis: self._save_captured_axis(axis))
            button.setProperty("save_teaching_axis", axis)
            button.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
            button.setStyleSheet("font-size:12px; padding:3px 6px;")
            save_row.addWidget(button, 1)
        layout.addLayout(save_row)
        self.teach_target_combo.setToolTip("적용은 RAM에 반영합니다. SAVE Z는 Z CAP UP, SAVE R은 R DECAP 캡처값을 저장합니다.")
        rpos_button.setToolTip("현재 위치를 읽습니다. HOME 완료 후 대기 중에는 Z/R 위치를 읽어 저장 후보로 표시합니다.")
        parameter_row = QHBoxLayout()
        self.parameter_combo = QComboBox()
        for offset,(title,unit) in enumerate(PARAMETERS):
            if offset <= 6 or offset in TEACH_OFFSETS:
                continue
            self.parameter_combo.addItem(title, offset)
            if unit:
                self.parameter_combo.setItemData(self.parameter_combo.count() - 1, unit, Qt.ItemDataRole.ToolTipRole)
        self.parameter_value_input = QLineEdit()
        self.parameter_value_input.setPlaceholderText("설정값")
        parameter_row.addWidget(self.parameter_combo, 2)
        parameter_row.addWidget(self.parameter_value_input, 1)
        read_parameter = self._action_button("읽기", "parameter_read", self._read_parameter)
        read_parameter.setToolTip("선택한 모션 설정의 현재 값을 읽습니다.")
        parameter_row.addWidget(read_parameter)
        apply_parameter = self._action_button("적용", "parameter_apply", self._apply_parameter)
        apply_parameter.setToolTip("선택한 모션 설정을 RAM에 반영합니다.")
        parameter_row.addWidget(apply_parameter)
        layout.addLayout(parameter_row)
        self.setting_status_label = self._compact_label("")
        layout.addWidget(self.setting_status_label)
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
        self.longrun_panel = LongrunPanel(log=self._append_log, single_endpoint=self._single_endpoint,
            terminators=lambda: (self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked()),
            initial_ips=MODULE_PRESETS)
        self.motion_tabs.addTab(self.longrun_panel, "Longrun")
        self.motion_tabs.currentChanged.connect(self._on_motion_tab_changed)
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
        stopping = command.strip().upper() == "STOP"
        if stopping:
            for action in self.position_actions.values():
                action.cancel("STOP 요청")
        if stopping and self.motion_tabs.currentWidget() is self.longrun_panel:
            self.longrun_panel.stop_all()
            for session in self.sessions.values():
                if self._session_needs_stop(session):
                    session.request("STOP")
            return True
        action = self.position_actions[self.transport_combo.currentData()]
        if not stopping and action.active:
            self._log_error("위치 이동 절차가 진행 중입니다. 중단하려면 STOP을 누르세요.")
            return False
        if self._active_command_client() is None:
            self._log_error("Cannot send: connect TCP/IP or UART first.")
            return False
        result = self._selected_session().request(command)
        self._refresh_device_controls()
        return result

    @staticmethod
    def _session_needs_stop(session):
        return (session.connected and (session.motion_wait or session.stop_wait or session.uncertain
                or (session.state and (session.state.busy or not session.state.idle))))

    def _on_motion_tab_changed(self, _index):
        if self.motion_tabs.tabText(self.motion_tabs.currentIndex()) != "Test":
            for action in self.position_actions.values():
                action.cancel("탭 변경")
        self._refresh_device_controls()

    def _start_position_action(self, offset):
        action = self.position_actions[self.transport_combo.currentData()]
        result = action.origin() if offset is None else action.cap(offset)
        self._refresh_device_controls()
        return result

    def _send_speed(self):
        self._send_motion_command(f"SPEED {self.speed_input.value()}")

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
        command_input.setToolTip("한 줄에 ASCII 명령 하나를 입력하세요. 최대 253바이트입니다.")
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
        layout.setContentsMargins(16, 10, 16, 10)
        layout.setSpacing(8)

        title_label = QLabel("Vial Decapper")
        title_label.setObjectName("app_title")
        title_label.setFont(QFont("Segoe UI", 22, QFont.Weight.Bold))
        brand_mark = QLabel("◈")
        brand_mark.setObjectName("brand_mark")
        brand_mark.setAlignment(Qt.AlignmentFlag.AlignCenter)
        brand_mark.setFixedSize(36, 36)
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
            address = ipaddress.IPv4Address(host)
            if address.is_unspecified or address.is_multicast:
                raise ValueError
            host = str(address)
        except ValueError:
            self._log_error("올바른 IPv4 주소를 입력하세요.")
            return
        try:
            port = int(self.port_input.text())
            if not 1 <= port <= 65535:
                raise ValueError
        except ValueError:
            self._append_log("ERROR: Port must be between 1 and 65535.")
            return
        if self.longrun_panel.endpoint_in_use((host, port)):
            self._log_error("Longrun에서 연결 중인 IP입니다. 해당 행의 연결을 먼저 해제하세요.")
            return
        self._single_tcp_endpoint = (host, port)
        self.command_client.connect_to_host(host, port)

    def _single_endpoint(self):
        reserved = (self.command_client.is_connected() or
                    self.command_client.socket.state() != QAbstractSocket.SocketState.UnconnectedState)
        return getattr(self, "_single_tcp_endpoint", None) if reserved else None

    def _send_command(self) -> None:
        if self.motion_tabs.currentWidget() is self.longrun_panel:
            self._log_error("Longrun 탭에서는 IP 목록의 시작/STOP 버튼을 사용하세요.")
            return
        self._send_motion_command(self.command_input.text())

    def closeEvent(self, event):
        self.session_timer.stop()
        self.longrun_panel.close_connections()
        self.command_client.disconnect_from_host()
        self.serial_client.close()
        super().closeEvent(event)

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
                current = session.fresh_position(axis)
                if current is None:
                    self._log_error("상대 이동 전에 장비 대기 상태에서 현재 축 위치를 Read로 다시 확인하세요.")
                    return
                target += current
            if not session.target_in_range(axis,target):
                self._log_error("장비 리미트 밖이거나 리미트를 확인하지 못했습니다.")
                return
        self._send_motion_command(f"MOVE {mode} {axis} {value}")

    def _send_rpos_command(self) -> None:
        session = self._selected_session()
        axis = self.move_axis_combo.currentData()
        if session.teaching_read_captures:
            command = "RPOS"
        elif session.extended:
            command = f"GPOS {axis}"
        elif axis == "Z":
            self._log_error("구형 F/W: RPOS는 Z 티칭 저장 후보도 변경합니다.")
            command = "RPOS"
        else:
            self._log_error("R 위치 조회에는 확장 F/W가 필요합니다.")
            return
        target = self.teach_target_combo.currentData()
        text = self.teach_value_input.text()
        if self._send_motion_command(command):
            self._teaching_read = (session.pending, target, text)

    def _finish_teaching_read(self, session, pending, readings):
        if not self._teaching_read or self._teaching_read[0] is not pending or session.pending is pending:
            return
        _, target, text = self._teaching_read
        self._teaching_read = None
        if target != self.teach_target_combo.currentData() or text != self.teach_value_input.text():
            return
        axis = "R" if target == 21 else "Z"
        if readings and axis in readings and (axis == "Z" or session.paired_rpos):
            position = session.fresh_position(axis)
            if position is not None:
                self.teach_value_input.setText(str(position))

    def _active_command_client(self):
        client = self.command_client if self.transport_combo.currentData() == "TCP" else self.serial_client
        return client if client.is_connected() else None

    def _on_transport_changed(self, _index):
        for action in self.position_actions.values():
            action.cancel("전송 경로 변경")
        for session in self.sessions.values():
            session.clear_teaching_capture()
        self._teaching_read = None
        self.current_z_position_value.setText("--")
        self._displayed_speed = None
        self._displayed_parameter = None
        self._refresh_device_controls()
        self._append_log(f"전송 경로: {self.transport_combo.currentText()}")

    def _reset_receive_state(self, source):
        self._responses[source].reset()
        if self.transport_combo.currentData() == source:
            self._teaching_read = None
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
        self.position_actions["TCP"].cancel("재연결")
        peer = self.command_client.socket.peerAddress()
        if not peer.isNull():
            self._single_tcp_endpoint = (peer.toString().removeprefix("::ffff:"), self.command_client.socket.peerPort())
        self.connect_button.setText("Disconnect")
        self._reset_receive_state("TCP")
        self._append_log("[TCP] Connected.")
        self._primed_routes.discard("TCP")
        self.sessions["TCP"].start()
        self._refresh_device_controls()

    def _on_disconnected(self) -> None:
        self.position_actions["TCP"].cancel("연결 끊김")
        self.connect_button.setText("Connect")
        self._reset_receive_state("TCP")
        self._append_log("[TCP] Disconnected.")
        self.sessions["TCP"].stop()
        self._primed_routes.discard("TCP")
        self._refresh_device_controls()

    def _on_uart_connected(self) -> None:
        self.position_actions["UART"].cancel("재연결")
        self.uart_connect_button.setText("UART Disconnect")
        self._reset_receive_state("UART")
        self._append_log("UART connected.")
        self._primed_routes.discard("UART")
        self.sessions["UART"].start()
        self._refresh_device_controls()

    def _on_uart_disconnected(self) -> None:
        self.position_actions["UART"].cancel("연결 끊김")
        self.uart_connect_button.setText("UART Connect")
        self._reset_receive_state("UART")
        self._append_log("UART disconnected.")
        self.sessions["UART"].stop()
        self._primed_routes.discard("UART")
        self._refresh_device_controls()

    def _log_sent(self, payload: bytes, source="TCP") -> None:
        self._append_log(f"[{source}] TX: {self._render_bytes(payload)}")

    def _log_received(self, payload: bytes, source="TCP") -> None:
        session = self.sessions[source]
        received = False
        for line in self._responses[source].feed(payload):
            received = True
            self._append_log(f"[{source}] RX: {line}")
            pending = session.pending
            readings = session.receive(line)
            self._prime_device_parameters(source)
            self.position_actions[source].tick()
            if source == self.transport_combo.currentData():
                self._finish_teaching_read(session, pending, readings)
        if received and source == self.transport_combo.currentData():
            self._refresh_device_controls()

    def _log_error(self, message: str) -> None:
        self._append_log(f"ERROR: {message}")

    @staticmethod
    def _render_bytes(payload: bytes) -> str:
        return payload.decode("ascii", errors="backslashreplace").replace("\r", "\\r").replace("\n", "\\n")

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
