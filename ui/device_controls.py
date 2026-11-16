"""Device controls shared by the main window's transport and teaching pages."""
import re

from PySide6.QtWidgets import QMessageBox, QTableWidgetItem
from ui.device_faults import fault_summary, fault_tooltip
from ui.device_session import AUTOMATIC, MOTIONS, EXTENDED, PARAMETERS
from ui.device_telemetry import PHASE_NAMES, RESULT_NAMES, SENSOR_NAMES


def _phase_name(phase):
    return PHASE_NAMES[phase] if 0 <= phase < len(PHASE_NAMES) else str(phase)


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
        monitor = self.motion_tabs.tabText(self.motion_tabs.currentIndex()) == "Test" or self.live_model_check.isChecked()
        for source,session in self.sessions.items():
            session.tick(poll=source == selected or session.motion_wait or session.stop_wait or session.track_longrun,
                         sensors=source == selected and monitor)
            self.position_actions[source].tick()
        self.longrun_panel.tick()
        self._refresh_device_controls()

    def _refresh_device_controls(self, *_):
        # Construction connects combo signals before all pages exist.
        if not hasattr(self,"transport_combo") or not hasattr(self,"command_send_button"):
            return
        source = self.transport_combo.currentData()
        session = self.sessions[source]
        connected = session.connected
        batch = self.motion_tabs.currentWidget() is self.longrun_panel
        state = session.state
        action = self.position_actions[source]
        idle = (session.fresh and state.idle and not state.busy and not session.motion_wait
                and not session.uncertain and not session.stop_wait and not session.disconnected_during_motion)
        auto_block = session.automatic_block()
        self.command_send_button.setEnabled(connected and not batch)
        for button in self.motion_buttons:
            command = button.property("device_command") or ""
            name = command.split()[0] if command else ""
            enabled = connected
            if name in MOTIONS or button is self.move_send_button:
                enabled = session.can_move
                if name in AUTOMATIC:
                    enabled = enabled and not auto_block
                    button.setToolTip(auto_block or command)
            elif button.property("position_action"):
                enabled = action.ready()
            elif name in {"PAUSE","RESUME"}:
                enabled = session.can_pause(resume=name == "RESUME")
            elif button.objectName() in {"speed_apply_button","device_clear"}:
                enabled = idle
            elif button.property("save_teaching_axis"):
                axis = button.property("save_teaching_axis")
                target = "Z CAP UP" if axis == "Z" else "R DECAP"
                value = session.captured_targets.get(axis)
                enabled = session.can_capture_teaching and value is not None
                caption = f"SAVE {axis}" if value is None else f"SAVE {axis} · {value}"
                if button.text() != caption:
                    button.setText(caption)
                button.setToolTip(f"{target} 저장 · HOME 완료 후 Read를 누르세요." if value is None
                                  else f"{target}에 캡처값 {value} pulse를 저장합니다.")
            elif button.objectName() in {"teach_apply","parameter_apply"}:
                enabled = idle and session.extended and not state.error
            elif button.objectName() in {"parameter_read","motion_lr"} or name in EXTENDED:
                enabled = connected and session.extended
                if button.objectName()=="motion_lr": enabled = enabled and session.can_move
            if action.active and name != "STOP":
                enabled = False
            if batch:
                enabled = name == "STOP" and (any(m.client.is_connected() for m in self.longrun_panel.modules)
                    or any(self._session_needs_stop(s) for s in self.sessions.values()))
            button.setEnabled(enabled)
        tone = "waiting"
        if not connected:
            status, tone = (("연결 끊김 · 정지 여부 미확인", "fault") if session.disconnected_during_motion
                            else ("연결 안 됨", "offline"))
        elif session.stop_wait:
            status = session.operation_text
        elif session.disconnected_during_motion:
            status, tone = "재연결 · 이전 정지 여부 미확인", "fault"
        elif not session.version:
            status = "포트 연결 · F/W 응답 대기"
        elif not session.fresh or session.uncertain:
            status = "상태 확인 필요 · 모션 대기"
        elif session.completion_unknown:
            status = "완료 확인 필요 · STOP으로 대기 복구"
        else:
            if state.error:
                status, tone = f"오류 {state.error_code}", "fault"
            elif state.paused:
                status = "일시정지"
            elif state.busy or not state.idle or session.motion_wait:
                status, tone = "동작 중", "active"
            else:
                status, tone = "대기", "ready" if state.homed and session.can_move else "waiting"
            status += f" · HOME {'완료' if state.homed else '필요'}"
            if state.phase:
                status += f" · {_phase_name(state.phase)}"
        capability = "확장 API" if session.extended else "기존 API" if session.schema==0 else "API 확인 중"
        status_line = f"{source} · {status} · F/W {session.version or '--'} · {capability}"
        if batch:
            online = sum(m.client.is_connected() for m in self.longrun_panel.modules)
            status_line = f"Longrun · 선택 {len(self.longrun_panel.selected())}대 · TCP 연결 {online}대 · 상단 STOP = 전체"
            tone = "active" if online else "offline"
        badge = self.device_state_label
        self._show_text(badge, status_line)
        if badge.property("tone") != tone:
            badge.setProperty("tone", tone)
            badge.style().unpolish(badge)
            badge.style().polish(badge)
            badge.update()
        error = ""
        if session.fresh and state.error:
            error = fault_summary(state.error_code, session.error_detail)
        elif session.error_detail:
            error = session.error_detail
        self._show_text(self.device_detail_label, error)
        tooltip = fault_tooltip(error, session.error_detail) if session.fresh and state.error else error
        if self.device_detail_label.toolTip() != tooltip:
            self.device_detail_label.setToolTip(tooltip)
        self.device_detail_label.setVisible(bool(error))
        conditions = auto_block or ("HOME 완료 · CT 감지" if session.can_move else "장비 대기 필요")
        self._show_text(self.auto_condition_label, f"자동 운전 조건: {conditions}")
        operation = session.operation_text or "--"
        if action.active and not operation.startswith(action.label):
            operation = f"{action.label} · {operation}"
        self._show_text(self.operation_label, f"최근 동작: {operation}")
        setting = session.setting_text or "Read 후 저장할 축을 선택하세요."
        if session.verified_settings:
            setting += f" · 미저장 {len(session.verified_settings)}개"
        if session.setting_unverified:
            setting += f" · 검증 미완료 {len(session.setting_unverified)}개"
        self._show_text(self.setting_status_label, f"설정: {setting}")
        r_index = self.teach_target_combo.findData(21)
        r_title = "R DECAP 목표 위치" if session.paired_rpos else "R DECAP 설정값"
        if self.teach_target_combo.itemText(r_index) != r_title:
            self.teach_target_combo.setItemText(r_index, r_title)
        axis = self.move_axis_combo.currentData()
        self.current_z_position_label.setText(f"Current {axis} Position")
        position_command = "RPOS" if session.teaching_read_captures or not session.extended else "GPOS"
        self.findChild(type(self.current_z_position_label), "rpos_command_label").setText(position_command)
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
        suffix = "" if session.fresh_position(axis) is not None else " (이전)"
        return f"{session.positions[axis]}{suffix}"

    def _refresh_debug(self, session):
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
                ("Z 이동 요청값 (pulse)", str(data["TARGET_Z"])),
                ("R 이동 요청값 (pulse)", str(data["TARGET_R"])),
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
        if name == "CD" and data:
            self.diagnostic_label.setToolTip(caption + "\n이동 요청값: ABS는 절대 좌표, REL은 이동량입니다.")

    def _apply_teach(self):
        session = self._selected_session()
        text = self.teach_value_input.text().strip()
        try:
            if not re.fullmatch(r"[+-]?[0-9]+", text):
                raise ValueError
            value = int(text)
        except ValueError:
            self._log_error("티칭 값은 32비트 정수 범위의 pulse로 입력하세요.")
            return
        offset = self.teach_target_combo.currentData()
        axis = "R" if offset==21 else "Z"
        if not session.target_in_range(axis,value):
            self._log_error("장비 리미트 밖이거나 리미트를 확인하지 못했습니다.")
            return
        self._send_motion_command(f"TEACH {session.parameter_base+offset},{value}")

    def _save_captured_axis(self, axis):
        session = self._selected_session()
        value = session.captured_targets.get(axis)
        if value is None or not session.can_capture_teaching:
            return
        target = "Z CAP UP" if axis == "Z" else "R DECAP"
        answer = QMessageBox.question(self, "EEPROM 축별 저장", f"{target}에 {value} pulse를 저장할까요?",
                                      QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                                      QMessageBox.StandardButton.No)
        if answer != QMessageBox.StandardButton.Yes:
            return
        if (self._selected_session() is not session or not session.can_capture_teaching
                or session.captured_targets.get(axis) != value):
            self._log_error("장비 상태 또는 캡처값이 바뀌었습니다. 다시 캡처하세요.")
            return
        self._send_motion_command(f"SAVE {axis}")

    def _read_parameter(self):
        session = self._selected_session()
        if session.extended:
            self._send_motion_command(f"PGET {session.parameter_base+self.parameter_combo.currentData()}")

    def _apply_parameter(self):
        session = self._selected_session()
        offset = self.parameter_combo.currentData()
        text = self.parameter_value_input.text().strip()
        try:
            if not re.fullmatch(r"[+-]?[0-9]+", text):
                raise ValueError
            value = int(text)
            if not -2147483648 <= value <= 2147483647:
                raise ValueError
        except ValueError:
            self._log_error("32비트 정수 범위의 설정값을 입력하세요.")
            return
        reason = session.parameter_issue(offset, value)
        if reason:
            self._log_error(reason)
            self._show_text(self.setting_status_label, reason)
            return
        self._send_motion_command(f"PL {session.parameter_base+offset},{value}")
