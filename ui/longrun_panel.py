"""Independent TCP sessions for checked modules in the Longrun workspace."""
import csv
import ipaddress
from pathlib import Path

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFontMetrics
from PySide6.QtWidgets import (QAbstractItemView, QCheckBox, QFileDialog, QHBoxLayout,
    QHeaderView, QLabel, QLineEdit, QPushButton, QSpinBox, QStyle, QTableWidget,
    QTableWidgetItem, QVBoxLayout, QWidget, QSizePolicy)

from ui.command_wire import ResponseStream
from ui.device_faults import fault_summary, fault_tooltip
from ui.device_session import DeviceSession
from ui.tcp_command_client import TcpCommandClient
from ui.theme import LONGRUN_STYLE

MODULE_PRESETS = {
    "192.168.0.170": (25, "Reagent Dispenser"),
    "192.168.0.171": (25, "Reagent Dispenser"),
    "192.168.0.172": (25, "Workup"),
    "192.168.0.173": (25, "Workup"),
    "192.168.0.174": (25, "Workup"),
    "192.168.0.175": (25, "Prep"),
    "192.168.0.176": (25, "Prep"),
    "192.168.0.177": (2, "Prep"),
    "192.168.0.178": (2, "Prep"),
}


def elapsed(seconds):
    minutes, seconds = divmod(int(seconds), 60)
    hours, minutes = divmod(minutes, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


class LongrunModule:
    def __init__(self, endpoint, panel):
        self.endpoint, self.panel = endpoint, panel
        self.client = panel.client_factory(panel)
        self.connecting_at = None
        self.error = ""
        self.responses = ResponseStream(lambda: self.report("RX가 64 KiB를 초과해 줄 끝까지 버립니다."))
        self.session = DeviceSession(
            lambda command: bool(self.client.send_command(command, *panel.terminators())),
            report=self.report)
        self.client.connected.connect(self.connected)
        self.client.disconnected.connect(self.disconnected)
        self.client.received.connect(self.received)
        self.client.sent.connect(lambda data: self.log("TX: " +
            data.decode("ascii", errors="backslashreplace").replace("\r", "\\r").replace("\n", "\\n")))
        self.client.error.connect(self.report)

    def log(self, text):
        host, port = self.endpoint
        self.panel.log(f"[LR {host}:{port}] {text}")

    def report(self, message):
        self.error = message
        self.log("ERROR: " + message)
        self.panel.refresh()

    def connect(self):
        if self.client.is_connected() or self.connecting_at is not None:
            return
        self.error = ""
        self.connecting_at = self.session.clock()
        self.client.connect_to_host(*self.endpoint)

    def connected(self):
        self.connecting_at = None
        self.responses.reset()
        self.log("Connected.")
        self.session.start()
        self.panel.refresh()

    def disconnect(self):
        self.client.disconnect_from_host()
        # Qt can abort a pending connection without emitting disconnected.
        if self.connecting_at is not None:
            self.disconnected()

    def disconnected(self):
        self.connecting_at = None
        self.responses.reset()
        self.session.stop()
        self.log("Disconnected.")
        self.panel.refresh()

    def received(self, data):
        for line in self.responses.feed(data):
            self.log("RX: " + line)
            self.session.receive(line)
        self.panel.refresh()

    def tick(self):
        if self.connecting_at is not None and self.session.clock() - self.connecting_at > 5:
            self.disconnect()
            self.error = "연결 시간 초과"
        self.session.tick()

    def status(self):
        s = self.session
        if self.connecting_at is not None:
            return "연결 중", "#d6b465"
        if not s.connected:
            return ("끊김 · 정지 미확인", "#e27b7b") if s.disconnected_during_motion else ("미연결", "#91a5af")
        if not s.version:
            return "F/W 응답 대기", "#d6b465"
        if s.stop_wait:
            return "STOP 확인 중", "#d6b465"
        if s.disconnected_during_motion:
            return "이전 정지 미확인", "#e27b7b"
        if not s.fresh or s.uncertain or s.completion_unknown:
            return "상태 확인 필요", "#d6b465"
        if s.state.error:
            return f"오류 {s.state.error_code}", "#e27b7b"
        if s.state.paused:
            return "일시정지", "#d6b465"
        if s.state.busy or not s.state.idle or s.motion_wait:
            return "동작 중", "#74cbbd"
        if s.schema is None:
            return "API 확인 중", "#d6b465"
        return "연결 · 대기", "#74cbbd"

    def progress(self):
        s = self.session
        if not s.track_longrun and s.longrun_history:
            last = s.longrun_history[-1]
            count = last.count if last.count is not None else "?"
            return f"{count}/{last.limit or '∞'} · {elapsed(last.elapsed)} · {last.result}"
        if s.track_longrun:
            if s.longrun:
                count, limit, phase, paused, busy = s.longrun
                stage = "일시정지" if paused else "DECAP" if phase == 0 else "CAP"
                if not s._fresh_at(s.longrun_at):
                    stage = "이전 조회값"
                return f"{count}/{limit or '∞'} · {elapsed(s.longrun_elapsed)} · {stage}"
            return f"--/{s.lr_limit or '∞'} · {elapsed(s.longrun_elapsed)} · 접수/실행 확인 중"
        return "--"


class LongrunPanel(QWidget):
    def __init__(self, parent=None, log=lambda message: None,
                 client_factory=TcpCommandClient, single_endpoint=lambda: None,
                 terminators=lambda: (True, True), initial_ips=None):
        super().__init__(parent)
        self.setStyleSheet(LONGRUN_STYLE)
        self.log, self.client_factory = log, client_factory
        self.single_endpoint, self.terminators = single_endpoint, terminators
        self.rows = []
        self.archived = []
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 6, 8, 6)
        layout.setSpacing(4)
        actions = QHBoxLayout()
        actions.setSpacing(3)
        self.select_all = QCheckBox("전체 선택")
        self.select_all.setTristate(True)
        self.select_all.setChecked(True)
        self.select_all.clicked.connect(self.check_all)
        actions.addWidget(self.select_all)
        for text, callback in (("+ IP", lambda: self.add_module()),
                               ("삭제", self.remove_selected), ("선택 연결", self.connect_selected),
                               ("선택 해제", self.disconnect_selected)):
            actions.addWidget(self.button(text, callback))
        layout.addLayout(actions)
        self.table = QTableWidget(0, 6)
        self.table.setObjectName("longrun_module_table")
        self.table.setHorizontalHeaderLabels(("선택", "IP", "용량", "위치", "상태", "진행"))
        self.table.horizontalHeaderItem(5).setToolTip("완료 횟수 · 마우스를 올리면 목표, 경과 시간, 결과를 표시합니다.")
        self.table.verticalHeader().hide()
        self.table.verticalHeader().setDefaultSectionSize(27)
        self.table.setMinimumHeight(94)
        self.table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.table.setWordWrap(True)
        self.table.setAlternatingRowColors(True)
        self._location_font = self.table.font()
        self._location_font.setPixelSize(11)
        metrics = QFontMetrics(self._location_font)
        focus_margin = self.table.style().pixelMetric(QStyle.PixelMetric.PM_FocusFrameHMargin, None, self.table)
        # Account for the item delegate's text inset, grid line and pixel rounding.
        self._location_padding = 2 * (focus_margin + 1) + int(self.table.showGrid()) + 1
        locations = {location for _, location in MODULE_PRESETS.values()}
        self._full_location_width = max(metrics.horizontalAdvance(name) for name in locations) + self._location_padding
        self._wrapped_location_width = max(metrics.horizontalAdvance(word)
                                           for name in locations for word in name.split()) + self._location_padding
        header = self.table.horizontalHeader()
        for column, width in ((0, 28), (1, 112), (2, 52), (3, self._wrapped_location_width), (4, 70)):
            header.setSectionResizeMode(column, QHeaderView.ResizeMode.Fixed)
            self.table.setColumnWidth(column, width)
        header.setSectionResizeMode(5, QHeaderView.ResizeMode.Stretch)
        header.sectionResized.connect(self.refresh, Qt.ConnectionType.QueuedConnection)
        layout.addWidget(self.table, 1)
        controls = QHBoxLayout()
        controls.setSpacing(3)
        controls.addWidget(QLabel("Port"))
        self.port_input = QSpinBox()
        self.port_input.setRange(1, 65535)
        self.port_input.setValue(8000)
        self.port_input.setFixedWidth(92)
        self.port_input.setMinimumHeight(30)
        self.port_input.setButtonSymbols(QSpinBox.ButtonSymbols.NoButtons)
        controls.addWidget(self.port_input)
        controls.addSpacing(6)
        controls.addWidget(QLabel("횟수"))
        self.count_input = QSpinBox()
        self.count_input.setRange(1, 1000000)
        self.count_input.setValue(100)
        self.count_input.setMaximumWidth(86)
        controls.addWidget(self.count_input)
        self.finite_button = self.button("횟수 시작", lambda: self.start_selected(self.count_input.value()))
        self.finite_button.setObjectName("motion_lr")
        controls.addWidget(self.finite_button)
        self.unlimited_button = self.button("무한 시작", lambda: self.start_selected(0))
        self.unlimited_button.setObjectName("motion_lr_unlimited")
        controls.addWidget(self.unlimited_button)
        layout.addLayout(controls)
        stop_row = QHBoxLayout()
        self.stop_button = self.button("선택 STOP", self.stop_selected)
        self.all_stop_button = self.button("전체 STOP", self.stop_all)
        self.all_stop_button.setProperty("danger", True)
        self.export_button = self.button("기록 저장", self.export_records)
        for button in (self.stop_button, self.all_stop_button, self.export_button):
            stop_row.addWidget(button)
        layout.addLayout(stop_row)
        self.note = QLabel("IP를 입력하고 체크한 모듈을 연결하세요. 준비 완료 후 시작합니다.")
        self.note.setStyleSheet("font-size:11px;")
        self.note.setMinimumHeight(16)
        self.note.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
        layout.addWidget(self.note)
        for ip in initial_ips if initial_ips is not None else ("192.168.0.",):
            row = self.add_module(ip)
            if initial_ips is not None:
                row["check"].setChecked(False)

    def button(self, title, callback):
        button = QPushButton(title)
        button.setMinimumHeight(26)
        button.clicked.connect(lambda checked=False: callback())
        return button

    @property
    def modules(self):
        return [row["module"] for row in self.rows if row["module"] is not None]

    def add_module(self, ip="192.168.0."):
        index = len(self.rows)
        self.table.insertRow(index)
        check = QCheckBox()
        check.setChecked(True)
        address = QLineEdit(ip)
        address.setPlaceholderText("192.168.0.x")
        row = {"check": check, "ip": address, "module": None}
        self.rows.append(row)
        self.table.setCellWidget(index, 0, check)
        self.table.setCellWidget(index, 1, address)
        for column in (2, 3, 4, 5):
            item = QTableWidgetItem("--")
            if column in (2, 3, 5):
                item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            if column == 3:
                item.setFont(self._location_font)
            self.table.setItem(index, column, item)
        check.toggled.connect(self.refresh)
        address.textChanged.connect(self.refresh)
        self.refresh()
        return row

    def set_ip(self, index, address):
        self.rows[index]["ip"].setText(address)

    def set_checked(self, index, checked):
        self.rows[index]["check"].setChecked(checked)

    def check_all(self, checked):
        for row in self.rows:
            row["check"].setChecked(checked)

    def selected(self):
        return [row for row in self.rows if row["check"].isChecked()]

    def message(self, text):
        self.note.setText(text)
        self.note.setToolTip(text)
        self.log("[Longrun] " + text)

    def endpoint_in_use(self, endpoint):
        return any(module.endpoint == endpoint and
                   (module.client.is_connected() or module.connecting_at is not None) for module in self.modules)

    def validate_selection(self):
        rows = self.selected()
        if not rows:
            self.message("모듈을 하나 이상 체크하세요.")
            return None
        endpoints = []
        for row in rows:
            try:
                address = ipaddress.IPv4Address(row["ip"].text().strip())
                if address.is_unspecified or address.is_multicast:
                    raise ValueError
            except ValueError:
                self.message(f"올바른 IP를 입력하세요: {row['ip'].text()}")
                return None
            endpoints.append((str(address), self.port_input.value()))
        if len(endpoints) != len(set(endpoints)):
            self.message("중복된 IP가 있습니다.")
            return None
        for row, endpoint in zip(rows, endpoints):
            if self.single_endpoint() == endpoint:
                self.message(f"{endpoint[0]}: 개별 제어 연결을 먼저 해제하세요.")
                return None
            if any(other is not row and other["module"] and other["module"].endpoint == endpoint and
                   (other["module"].client.is_connected() or other["module"].connecting_at is not None)
                   for other in self.rows):
                self.message(f"{endpoint[0]}: 다른 행에서 이미 연결 중입니다.")
                return None
        return list(zip(rows, endpoints))

    def connect_selected(self):
        selection = self.validate_selection()
        if selection is None:
            return
        for row, endpoint in selection:
            module = row["module"]
            if module and module.endpoint != endpoint:
                self.archived.extend((module.endpoint, record) for record in module.session.longrun_history)
                module.client.deleteLater()
                module = None
            if module is None:
                module = row["module"] = LongrunModule(endpoint, self)
            module.connect()
        self.message("연결 요청 완료 · F/W 응답을 확인합니다.")
        self.refresh()

    def disconnect_selected(self):
        for row in self.selected():
            if row["module"]:
                row["module"].disconnect()
        self.message("선택 연결 해제 · 운전 중이었다면 장비는 계속 동작할 수 있습니다.")
        self.refresh()

    def remove_selected(self):
        for index in range(len(self.rows)-1, -1, -1):
            row = self.rows[index]
            module = row["module"]
            if not row["check"].isChecked():
                continue
            if module and (module.client.is_connected() or module.connecting_at is not None):
                continue
            if module:
                self.archived.extend((module.endpoint, record) for record in module.session.longrun_history)
                module.client.deleteLater()
            self.table.removeRow(index)
            self.rows.pop(index)
        self.refresh()

    def start_selected(self, count):
        rows = self.selected()
        if not rows:
            self.message("시작할 모듈을 체크하세요.")
            return False
        for row in rows:
            module = row["module"]
            if not module or not module.session.can_move or module.session.pending or module.session.queue:
                self.message(f"{row['ip'].text()}: 연결·대기 상태와 응답을 먼저 확인하세요.")
                return False
            if count and not module.session.extended:
                self.message(f"{row['ip'].text()}: 횟수 시작은 확장 F/W가 필요합니다.")
                return False
        command = f"LR {count}" if count else "LR"
        successes = 0
        for row in rows:
            successes += bool(row["module"].session.request(command))
        self.message(f"{len(rows)}대 중 {successes}대에 {command} 전송 · 각 모듈에서 접수/실행을 확인합니다.")
        self.refresh()
        return successes == len(rows)

    def stop_selected(self):
        self._stop([row["module"] for row in self.selected() if row["module"]])

    def stop_all(self):
        self._stop(self.modules)

    def _stop(self, modules):
        sent = sum(bool(module.session.request("STOP")) for module in modules if module.client.is_connected())
        self.message(f"{sent}대에 STOP 전송 · 정지 확인 중")
        self.refresh()

    def tick(self):
        for module in self.modules:
            module.tick()
        self.refresh()

    def refresh(self, *_):
        if not hasattr(self, "note"):
            return
        # Leave room for the progress count, even when the location needs two lines.
        fixed_width = sum(self.table.columnWidth(column) for column in (0, 1, 2, 4))
        available = self.table.viewport().width() - fixed_width - 52
        location_width = min(self._full_location_width, max(self._wrapped_location_width, available))
        self.table.setColumnWidth(3, location_width)
        metrics = QFontMetrics(self._location_font)
        checked = [row["check"].isChecked() for row in self.rows]
        self.select_all.blockSignals(True)
        self.select_all.setCheckState(Qt.CheckState.Checked if checked and all(checked) else
                                     Qt.CheckState.PartiallyChecked if any(checked) else Qt.CheckState.Unchecked)
        self.select_all.blockSignals(False)
        for index, row in enumerate(self.rows):
            module = row["module"]
            active = bool(module and (module.client.is_connected() or module.connecting_at is not None))
            row["ip"].setReadOnly(active)
            status, color = module.status() if module else ("미연결", "#91a5af")
            host = module.endpoint[0] if active else row["ip"].text().strip()
            capacity, location = MODULE_PRESETS.get(host, (None, ""))
            self.table.item(index, 2).setText(f"{capacity}mL" if capacity is not None else "—")
            location_item = self.table.item(index, 3)
            location_item.setText(location or "—")
            location_item.setToolTip(location or "설치 위치 미확인")
            wraps = metrics.horizontalAdvance(location_item.text()) + self._location_padding > location_width
            row_height = metrics.height() + metrics.lineSpacing() + self._location_padding if wraps else 27
            self.table.setRowHeight(index, max(27, row_height))
            item = self.table.item(index, 4)
            item.setText(status)
            item.setForeground(QColor(color))
            tooltip = [status]
            if module:
                session = module.session
                if session.fresh and session.state.error:
                    summary = fault_summary(session.state.error_code, session.error_detail)
                    tooltip.append(fault_tooltip(summary, session.error_detail))
                tooltip.extend((module.error, session.operation_text, "F/W " + session.version))
            item.setToolTip("\n".join(filter(None, tooltip)))
            progress = module.progress() if module else "--"
            self.table.item(index, 5).setText(progress.partition("/")[0])
            self.table.item(index, 5).setToolTip(progress)
        self.port_input.setEnabled(not any(m.client.is_connected() or m.connecting_at is not None for m in self.modules))
        modules = [row["module"] for row in self.selected()]
        ready = bool(modules) and all(m is not None and m.session.can_move and
                                      not m.session.pending and not m.session.queue for m in modules)
        self.unlimited_button.setEnabled(ready)
        self.finite_button.setEnabled(ready and all(m.session.extended for m in modules))
        self.stop_button.setEnabled(any(m and m.client.is_connected() for m in modules))
        self.all_stop_button.setEnabled(any(m.client.is_connected() for m in self.modules))
        self.export_button.setEnabled(bool(self.archived or any(m.session.longrun_history for m in self.modules)))

    def export_records(self):
        path, _ = QFileDialog.getSaveFileName(self, "Longrun 기록 저장", "longrun.csv", "CSV (*.csv)")
        if not path:
            return
        records = list(self.archived)
        records.extend((m.endpoint, record) for m in self.modules for record in m.session.longrun_history)
        try:
            with Path(path).open("w", encoding="utf-8-sig", newline="") as output:
                writer = csv.writer(output)
                writer.writerow(("시작 시각", "IP", "모듈 구분", "Port", "목표", "완료 횟수", "경과", "결과", "오류"))
                for (host, port), r in records:
                    capacity, location = MODULE_PRESETS.get(host, (None, ""))
                    detail = f"{capacity} ml Vial" if capacity is not None else "사용자 지정"
                    if location:
                        detail = f"{location} · {detail}"
                    writer.writerow((r.started.isoformat(timespec="seconds"), host, detail, port, r.limit or "무제한",
                                     r.count if r.count is not None else "미확인", elapsed(r.elapsed), r.result, r.error_code))
            self.message(f"기록 저장: {path}")
        except OSError as error:
            self.message(f"기록 저장 실패: {error}")

    def close_connections(self):
        for module in self.modules:
            module.disconnect()
