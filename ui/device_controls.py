"""Device controls shared by the main window's transport and teaching pages."""
import math
import re
import csv
from pathlib import Path

from PySide6.QtWidgets import QFileDialog, QMessageBox, QTableWidgetItem
from ui.device_session import AUTOMATIC, MOTIONS, EXTENDED, PARAMETERS
from ui.device_telemetry import ERROR_NAMES, PHASE_NAMES, RESULT_NAMES, SENSOR_NAMES


def _phase_name(phase):
    return PHASE_NAMES[phase] if 0 <= phase < len(PHASE_NAMES) else str(phase)


def _elapsed(seconds):
    minutes, seconds = divmod(int(seconds), 60)
    hours, minutes = divmod(minutes, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


class DeviceControls:
    def _selected_session(self):
        return self.sessions[self.transport_combo.currentData()]

    def _send_to_route(self, source, command):
        client = self.command_client if source == "TCP" else self.serial_client
        return bool(client.send_command(command, self.cr_checkbox.isChecked(), self.lf_checkbox.isChecked()))

    def _prime_device_parameters(self, source):
        session = self.sessions[source]
        if session.extended and source not in self._primed_routes:
            self._primed_routes.add(source)
            session.enqueue_queries(["PL"])

    def _tick_sessions(self):
        selected = self.transport_combo.currentData()
        monitor = self.motion_tabs.tabText(self.motion_tabs.currentIndex()) in {"Test", "Debug"} or self.live_model_check.isChecked()
        for source,session in self.sessions.items():
            session.tick(poll=source == selected or session.motion_wait or session.stop_wait or session.track_longrun,
                         sensors=source == selected and monitor)
        self._refresh_device_controls()

    def _refresh_device_controls(self, *_):
        # Construction connects combo signals before all pages exist.
        if not hasattr(self,"transport_combo") or not hasattr(self,"command_send_button"):
            return
        source = self.transport_combo.currentData()
        session = self.sessions[source]
        connected = session.connected
        state = session.state
        idle = (session.fresh and state.idle and not state.busy and not session.motion_wait
                and not session.uncertain and not session.stop_wait and not session.disconnected_during_motion)
        auto_block = session.automatic_block()
        self.command_send_button.setEnabled(connected)
        for button in self.motion_buttons:
            command = button.property("device_command") or ""
            name = command.split()[0] if command else ""
            enabled = connected
            if name in MOTIONS or button is self.move_send_button:
                enabled = session.can_move
                if name in AUTOMATIC:
                    enabled = enabled and not auto_block
                    button.setToolTip(auto_block or command)
            elif name in {"PAUSE","RESUME"}:
                enabled = session.can_pause(resume=name == "RESUME")
            elif button.objectName() in {"speed_apply_button","device_clear"}:
                enabled = idle
            elif button.objectName() in {"teach_apply","parameter_apply","motion_save","teach_capture"}:
                enabled = idle and session.extended and not state.error
            elif button.objectName() in {"parameter_read","motion_lr"} or name in EXTENDED:
                enabled = connected and session.extended
                if button.objectName()=="motion_lr": enabled = enabled and session.can_move
            button.setEnabled(enabled)
        if not connected:
            status = "연결 끊김 · 정지 여부 미확인" if session.disconnected_during_motion else "연결 안 됨"
        elif session.stop_wait:
            status = session.operation_text
        elif session.disconnected_during_motion:
            status = "재연결 · 이전 정지 여부 미확인"
        elif not session.version:
            status = "포트 연결 · F/W 응답 대기"
        elif not session.fresh or session.uncertain:
            status = "상태 확인 필요 · 모션 대기"
        elif session.completion_unknown:
            status = "완료 확인 필요 · STOP으로 대기 복구"
        else:
            status = (f"오류 {state.error_code}" if state.error else "일시정지" if state.paused else
                      "동작 중" if state.busy or not state.idle or session.motion_wait else "대기")
            status += f" · HOME {'완료' if state.homed else '필요'} · {_phase_name(state.phase)}"
        capability = "확장 API" if session.extended else "기존 API" if session.schema==0 else "API 확인 중"
        self.device_state_label.setText(f"{source} · {status} · F/W {session.version or '--'} · {capability}")
        self.device_state_label.setToolTip(self.device_state_label.text())
        error = ""
        if session.fresh and state.error:
            error = f"{state.error_code} · {ERROR_NAMES.get(state.error_code, '장비 오류')}"
            if session.error_detail:
                error += " · " + " ".join(session.error_detail.splitlines())
        elif session.error_detail:
            error = " ".join(session.error_detail.splitlines())
        self._show_text(self.device_detail_label, error)
        self.device_detail_label.setVisible(bool(error))
        conditions = auto_block or ("HOME 완료 · CT 감지" if session.can_move else "장비 대기 필요")
        self._show_text(self.auto_condition_label, f"자동 운전 조건: {conditions}")
        self._show_text(self.operation_label, f"최근 동작: {session.operation_text or '--'}")
        setting = session.setting_text or "RAM 확인 후 EEPROM에 저장"
        if session.verified_settings:
            setting += f" · 미저장 {len(session.verified_settings)}개"
        self._show_text(self.setting_status_label, f"설정: {setting}")
        axis = self.move_axis_combo.currentData()
        self.current_z_position_label.setText(f"Current {axis} Position")
        self.findChild(type(self.current_z_position_label),"rpos_command_label").setText("GPOS" if session.extended else "RPOS")
        self.current_z_position_value.setText(str(session.positions[axis]) if axis in session.positions else "--")
        self.current_z_position_value.setToolTip(f"최근 읽기: {self._position_display(session, axis)} pulse")
        speed_key = (source,session.speed)
        if session.speed is not None and speed_key != self._displayed_speed:
            self.speed_input.setValue(session.speed)
            self._displayed_speed = speed_key
        if session.extended:
            index = session.parameter_base+self.parameter_combo.currentData()
            parameter_key = (source,index,session.parameters.get(index))
            if parameter_key != self._displayed_parameter:
                value = session.parameters.get(index)
                self.parameter_value_input.setText(format(value,"g") if value is not None else "")
                self._displayed_parameter = parameter_key
        self._refresh_debug(session)
        self._refresh_diagnostic_table(session, source)

    @staticmethod
    def _show_text(label, text):
        if label.text() != text:
            label.setText(text)
            label.setToolTip(text)

    @staticmethod
    def _position_display(session, axis):
        if axis not in session.positions:
            return "--"
        suffix = "" if session._fresh_at(session.position_at.get(axis)) else " (이전)"
        return f"{session.positions[axis]}{suffix}"

    def _refresh_debug(self, session):
        elapsed = _elapsed(session.longrun_elapsed)
        if not session.track_longrun and session.longrun_history:
            record = session.longrun_history[-1]
            count = record.count if record.count is not None else "미확인"
            text = f"완료 횟수: {count} / {record.limit or '무제한'} · {elapsed} · {record.result}"
        elif session.longrun:
            count,limit,phase,paused,busy = session.longrun
            stage = "일시정지" if paused else ("DECAP" if phase == 0 else "CAP")
            if not session._fresh_at(session.longrun_at):
                stage = "이전 조회값"
            text = f"완료 횟수: {count} / {limit or '무제한'} · {elapsed} · {stage}"
        elif session.track_longrun:
            mode = f"목표 {session.lr_limit}회" if session.lr_limit else "무한 반복"
            text = f"{mode} · {elapsed} · {session.operation_text}"
        else:
            text = "완료 횟수: --"
        self._show_text(self.longrun_status_label, text)
        self.longrun_export_button.setEnabled(bool(session.longrun_history))
        sensors = session.fresh_sensors
        position = session.y_position
        if sensors is not None:
            self.sensor_status_label.setText(f"Y: {position} · CT: {'감지' if sensors[0] else '미감지'}")
            self.sensor_status_label.setToolTip("원시 값: " + " · ".join(
                f"{name}={bit}" for name, bit in zip(SENSOR_NAMES, sensors)))
        else:
            self._show_text(self.sensor_status_label, "Y / CT: 확인 안 됨")
        grip = session.fresh_grip
        cap, vial = grip if grip is not None else (None, None)
        cap_text = "보유" if cap else "미보유" if cap is not None else "확인 안 됨"
        vial_text = "감지" if vial else "미감지" if vial is not None else "확인 안 됨"
        self._show_text(self.grip_status_label, f"캡: {cap_text} · 바이알(Forward 마지막 감지): {vial_text}")
        live = self.live_model_check.isChecked()
        self.carriage_slider.setEnabled(not live)
        self.live_model_label.setVisible(live)
        self._show_text(self.live_model_label, f"Y: {position or '확인 안 됨'} · 캡: {cap_text}")
        self.device_model_view.set_live_state(live, position, cap)

    def _refresh_diagnostic_table(self, session, source):
        name = session.diagnostic_name
        data = session.diagnostics.get(name)
        rows = []
        if data and name == "SL":
            state = data["STATE"]
            rows = [
                ("상태", "일시정지" if state.paused else "대기" if state.idle else "동작 중"),
                ("HOME", "완료" if state.homed else "필요"),
                ("오류", state.error_code if state.error else "없음"),
                ("단계", _phase_name(state.phase)),
                ("동작 번호 / 결과", f"{state.operation} / {RESULT_NAMES[state.result]}" if state.has_operation else "--"),
                *[(f"{sensor} (원시)", str(bit)) for sensor, bit in zip(SENSOR_NAMES, data["SENSOR"])],
            ]
        elif data and name == "CD":
            count, limit, phase, paused, busy = data["LR"]
            rows = [
                ("Z 목표 (pulse)", str(data["TARGET_Z"])),
                ("R 목표 (pulse)", str(data["TARGET_R"])),
                ("Z 최근 읽기 (pulse)", self._position_display(session, "Z")),
                ("R 최근 읽기 (pulse)", self._position_display(session, "R")),
                ("속도", f"{data['SPEED']} %"),
                ("Longrun 완료", f"{count} / {limit or '무제한'}"),
                ("Longrun 구간", ("DECAP" if phase == 0 else "CAP") + (" · 일시정지" if paused else "")),
            ]
        elif data and name == "PL":
            for index, value in sorted(data.items()):
                offset = index - session.parameter_base if session.parameter_base is not None else -1
                title, unit = PARAMETERS[offset] if 0 <= offset < len(PARAMETERS) else ("", "")
                rows.append((f"{index} {title}", f"{value:g} {unit}".strip()))
        key = (source, name, rows)
        if key != getattr(self, "_diagnostic_display", None):
            self._diagnostic_display = key
            self.diagnostic_table.setRowCount(len(rows))
            for i, row in enumerate(rows):
                for j, value in enumerate(row):
                    self.diagnostic_table.setItem(i, j, QTableWidgetItem(value))
        if name and data:
            age = int(session.clock() - session.diagnostic_at[name])
            caption = f"{name} · 조회 당시 값 · {age}초 전"
        elif name:
            caption = f"{name} · 확인된 데이터 없음"
        else:
            caption = "SL 상태 · CD 제어값 · PL 설정"
        self._show_text(self.diagnostic_label, caption)

    def _export_longrun(self):
        session = self._selected_session()
        path, _ = QFileDialog.getSaveFileName(self, "Longrun 기록 저장", "longrun.csv", "CSV (*.csv)")
        if not path:
            return
        try:
            with Path(path).open("w", encoding="utf-8-sig", newline="") as output:
                writer = csv.writer(output)
                writer.writerow(("시작 시각", "전송 경로", "목표", "완료 횟수", "경과", "결과", "오류 코드"))
                source = self.transport_combo.currentData()
                for record in session.longrun_history:
                    writer.writerow((record.started.isoformat(timespec="seconds"), source,
                                     record.limit or "무제한", record.count if record.count is not None else "미확인",
                                     _elapsed(record.elapsed), record.result, record.error_code))
            self._append_log(f"Longrun 기록 저장: {path}")
        except OSError as error:
            self._log_error(f"Longrun 기록 저장 실패: {error}")

    def _apply_teach(self):
        session = self._selected_session()
        text = self.teach_value_input.text().strip()
        if not re.fullmatch(r"[+-]?[0-9]+",text):
            self._log_error("티칭 값은 정수 pulse로 입력하세요.")
            return
        value = int(text)
        offset = self.teach_target_combo.currentData()
        axis = "R" if offset==21 else "Z"
        if not session.target_in_range(axis,value):
            self._log_error("장비 리미트 밖이거나 리미트를 확인하지 못했습니다.")
            return
        self._send_motion_command(f"TEACH {session.parameter_base+offset},{value}")

    def _capture_teach(self):
        session = self._selected_session()
        if self.teach_target_combo.currentData()==21:
            self._log_error("R 회전량은 절대 위치와 다르므로 직접 입력하세요.")
        elif "Z" in session.positions and session.clock()-session.position_at["Z"]<=3:
            self.teach_value_input.setText(str(session.positions["Z"]))
        else:
            self._log_error("Z 축 위치를 Read로 먼저 확인하세요.")

    def _save_device_parameters(self):
        answer = QMessageBox.question(self,"EEPROM 저장","장비 파라미터 전체를 EEPROM에 저장할까요?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,QMessageBox.StandardButton.No)
        if answer==QMessageBox.StandardButton.Yes:
            self._send_motion_command("SAVE 1")

    def _read_parameter(self):
        session = self._selected_session()
        if session.extended:
            self._send_motion_command(f"PGET {session.parameter_base+self.parameter_combo.currentData()}")

    def _apply_parameter(self):
        session = self._selected_session()
        offset = self.parameter_combo.currentData()
        text = self.parameter_value_input.text().strip()
        try:
            value=float(text)
            if not math.isfinite(value):raise ValueError
            if offset>1 and not re.fullmatch(r"[+-]?[0-9]+",text):raise ValueError
            if not -2147483648<=value<=2147483647:raise ValueError
        except ValueError:
            self._log_error("유효한 파라미터 값을 입력하세요. 전류 이외 값은 정수입니다.")
            return
        reason = session.parameter_issue(offset, value)
        if reason:
            self._log_error(reason)
            self._show_text(self.setting_status_label, reason)
            return
        number=format(value,"g") if offset<=1 else str(int(value))
        self._send_motion_command(f"PL {session.parameter_base+offset},{number}")

    def _start_longrun(self):
        self._send_motion_command(f"LR {self.longrun_count_input.value()}")
