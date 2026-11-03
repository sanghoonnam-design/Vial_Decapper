"""Transport-independent device state and command response tracking."""
from collections import deque
from datetime import datetime
import math
import re
import time

from ui.device_telemetry import (DeviceState, LongrunRecord, fields, longrun_fields,
                                 parameter_value, sensor_fields, snapshot, state_fields)

MOTIONS = {"HOME", "ORG", "MOVE", "READY", "DECAP", "CAP", "UDECAP", "UCAP",
           "LR", "CAPDECAPLR", "BGRIP", "CGRIP"}
QUERIES = {"VERSION", "VERS", "VER", "GSTA", "ST", "GSTATE", "GERR", "ERR",
           "GERD", "ERD", "GPOS", "RPOS", "PGET", "PCONFIG", "GSENSOR", "GLR",
           "SL", "CD", "DI", "RTC", "IP", "SYSTEM", "SYS", "PPARAM"}
EXTENDED = {"GPOS", "GSTATE", "GSENSOR", "GLR", "PGET", "TEACH"}
TEACH_OFFSETS = {16, 17, 18, 21}
AUTOMATIC = {"CAP", "DECAP"}
PAUSABLE_PHASES = {3, 4, 16}
LONGRUN_COMMANDS = {"LR", "CAPDECAPLR"}
FAILURES = {"BUSY", "INVALID", "UNSUPPORTED", "RANGE", "FAIL"}
MOTION_ACK = {"HOME": "HOME", "ORG": "ORIGIN", "DECAP": "DECAP", "CAP": "CAP",
              "UDECAP": "UDECAP", "UCAP": "UCAP", "LR": "LR", "CAPDECAPLR": "LR"}
PARAMETERS = (
    ("Z 전류", "A"), ("R 전류", "A"), ("Z 최대 전류 코드", "0/1"), ("R 최대 전류 코드", "0/1"),
    ("Z 정지 전류", "%"), ("R 정지 전류", "%"), ("분해능 코드", "0~8"),
    ("Z 기본 리미트", "pulse"), ("R 기본 리미트", "pulse"),
    ("Z 음의 리미트", "pulse"), ("Z 양의 리미트", "pulse"),
    ("R 음의 리미트", "pulse"), ("R 양의 리미트", "pulse"), ("소프트 리미트 사용", "0/1"),
    ("Z 가속도", ""), ("Z 속도", ""), ("Z 캡 위 위치", "pulse"),
    ("Z 캡 옆 위치", "pulse"), ("Z 원위치", "pulse"), ("R 가속도", ""),
    ("R 속도", ""), ("R 회전량", "pulse"),
)

class DeviceSession:
    def __init__(self, send, clock=time.monotonic, report=lambda message: None):
        self.send, self.clock, self.report = send, clock, report
        self.longrun_history = deque(maxlen=100)
        self.lr_started = None
        self.disconnected_during_motion = False
        self.stop()

    def stop(self):
        if getattr(self, "connected", False):
            self.disconnected_during_motion |= bool(
                self.stop_wait or self.motion_wait or self.track_longrun
                or (self.state and (self.state.busy or not self.state.idle)))
            if self.lr_started is not None:
                self._finish_longrun("연결 끊김 · 결과 미확인")
        self.connected = False
        self.version = ""
        self.state = None
        self.state_at = None
        self.schema = None
        self.retry_schema_at = None
        self.parameter_base = None
        self.parameters = {}
        self.positions = {}
        self.position_at = {}
        self.speed = None
        self.sensors = None
        self.sensors_at = None
        self.sensor_idle = False
        self.grip = None
        self.grip_at = None
        self.longrun = None
        self.longrun_at = None
        self.lr_started = None
        self.lr_started_wall = None
        self.lr_limit = 0
        self.lr_terminal = None
        self.lr_operation = None
        self.lr_entered = False
        self.lr_final_count = None
        self.lr_deadline = None
        self.error_detail = ""
        self.error_queried = None
        self.diagnostics = {}
        self.diagnostic_name = None
        self.diagnostic_at = {}
        self.verified_settings = {}
        self.setting_verify = None
        self.setting_text = ""
        self.setting_unverified = set()
        self.operation_text = ""
        self.active_motion = None
        self.motion_id = None
        self.motion_correlate = False
        self.stop_wait = False
        self.stop_acknowledged = False
        self.pause_target = None
        self.pause_acknowledged = False
        self.pause_deadline = None
        self.motion_started = None
        self.last_response = ""
        self.pending = None
        self.queue = deque()
        self.uncertain = False
        self.motion_wait = False
        self.observed_motion = False
        self.motion_base = 0
        self.motion_acknowledged = False
        self.completion_unknown = False
        self.next_poll = 0
        self.next_discovery = 0
        self.next_sensor_poll = 0
        self.next_grip_poll = 0
        self.track_longrun = False
        self.next_longrun_poll = 0

    @property
    def extended(self):
        return self.schema == 1 and self.parameter_base is not None

    @property
    def fresh(self):
        return self.connected and self.state_at is not None and self.clock()-self.state_at <= 3

    @property
    def can_move(self):
        return (self.responding and self.schema is not None and not self.uncertain and not self.motion_wait and
                not self.stop_wait and not self.disconnected_during_motion and
                not self.track_longrun and self.state.idle and not self.state.busy and not self.state.error)

    @property
    def responding(self):
        return bool(self.version and self.fresh)

    def _fresh_at(self, timestamp, max_age=3):
        return self.connected and timestamp is not None and self.clock() - timestamp <= max_age

    @property
    def fresh_sensors(self):
        return self.sensors if self._fresh_at(self.sensors_at) else None

    @property
    def fresh_grip(self):
        return self.grip if self._fresh_at(self.grip_at, 4) else None

    @property
    def y_position(self):
        sensors = self.fresh_sensors
        if sensors is None:
            return None
        return {(0, 1): "Forward", (1, 0): "Back",
                (1, 1): "양쪽 미감지", (0, 0): "센서 조합 확인"}[sensors[3:5]]

    def automatic_block(self):
        if not self.fresh:
            return "장비 상태 확인 필요"
        if not self.state.homed:
            return "HOME 필요"
        if not self.extended:
            return "CT 확인 가능한 F/W 필요"
        sensors = self.fresh_sensors
        if sensors is None or not self.sensor_idle:
            return "CT 센서 확인 중"
        if not sensors[0]:
            return "CT 바이알 미감지"
        return ""

    def can_pause(self, resume=False):
        return (self.extended and self.fresh and self.state.busy and not self.state.error
                and not self.stop_wait and not self.uncertain and self.pause_target is None
                and (self.state.paused if resume else
                     not self.state.paused and self.state.phase in PAUSABLE_PHASES))

    @property
    def longrun_elapsed(self):
        if self.lr_started is not None:
            return max(0, (self.lr_terminal[2] if self.lr_terminal else self.clock()) - self.lr_started)
        return self.longrun_history[-1].elapsed if self.longrun_history else 0

    def start(self):
        self.stop()
        self.connected = True
        self.queue.extend(("VERSION", "GSTA", "SPEED", "PCONFIG"))
        self._pump()

    def _pump(self):
        if self.connected and self.pending is None and self.queue:
            command = self.queue.popleft()
            self.pending = (command, self.clock()+3)
            if not self.send(command):
                self.pending = None
                self.queue.clear()
                self.uncertain = True
                self._request_unknown(command)

    def request(self, command):
        command = command.strip().upper()
        if not command or "\r" in command or "\n" in command or not self.connected:
            return False
        name = command.split()[0]
        if name == "STOP":
            if self.pending:
                interrupted = self.pending[0]
                if interrupted.startswith(("SAVE", "TEACH ", "PL ", "PGET ")):
                    self._request_unknown(interrupted)
            self.queue.clear()
            self.pending = None
            self.completion_unknown = False
            self.stop_wait = True
            self.stop_acknowledged = False
            self.pause_target = None
            self.operation_text = "STOP · 요청 전송 / 정지 확인 대기"
            self.queue.append(command)
            self._pump()
            return self.pending is not None
        if self.pending or self.queue:
            self.report("이전 명령의 응답을 기다리는 중입니다.")
            return False
        if (name in EXTENDED or command == "SAVE 1" or (name=="LR" and command!="LR")) and not self.extended:
            self.report("현재 F/W에서 확인되지 않은 기능입니다.")
            return False
        if name in MOTIONS and not self.can_move:
            self.report("장비 상태 확인·대기·오류 해제가 필요합니다.")
            return False
        if name in AUTOMATIC:
            reason = self.automatic_block()
            if reason:
                self.operation_text = f"{name} · 실행 안 함: {reason}"
                self.report(self.operation_text)
                return False
        if name in {"PAUSE", "RESUME"}:
            if not self.can_pause(resume=name == "RESUME"):
                return False
            self.pause_target = name == "PAUSE"
            self.pause_acknowledged = False
            self.pause_deadline = self.clock() + 3
            self.operation_text = f"{name} · 적용 확인 대기"
        query = name in QUERIES or command in {"SPEED","SPD","PL"}
        if not query and name not in MOTIONS and name not in {"PAUSE","RESUME"}:
            if (not self.fresh or not self.state.idle or self.state.busy or self.motion_wait
                    or self.stop_wait or self.uncertain or self.disconnected_during_motion):
                self.report("설정 변경은 장비 대기 상태에서만 가능합니다.")
                return False
        if name in MOTIONS:
            self.sensor_idle = False
            self.motion_wait = True
            self.motion_acknowledged = False
            self.completion_unknown = False
            self.observed_motion = False
            self.motion_base = self.state.operation
            self.motion_correlate = self.state.has_operation
            self.motion_id = None
            self.motion_started = self.clock()
            self.active_motion = name
            self.operation_text = f"{name} · 접수 확인 대기"
            if name in LONGRUN_COMMANDS:
                args = command.split()
                if len(args) > 2 or (len(args) == 2 and
                        (not args[1].isdigit() or len(args[1]) > 7 or not 1 <= int(args[1]) <= 1000000)):
                    self.motion_wait = False
                    self.active_motion = None
                    self.operation_text = "Longrun · 반복 횟수는 1~1000000"
                    self.report(self.operation_text)
                    return False
                self.track_longrun = True
                self.longrun = None
                self.longrun_at = None
                self.lr_started = self.clock()
                self.lr_started_wall = datetime.now().astimezone()
                self.lr_limit = int(args[1]) if len(args) == 2 else 0
                self.lr_operation = None
                self.lr_entered = False
                self.lr_final_count = None
                self.lr_terminal = None
                self.lr_deadline = None
                self.next_longrun_poll = 0
        elif name in {"PL", "TEACH"} and "," in command:
            try:
                index, value = command.split(maxsplit=1)[1].split(",")
                index = int(index)
                value = parameter_value(value)
            except (ValueError, IndexError):
                self.report("파라미터 인덱스와 값이 올바르지 않습니다.")
                return False
            if self.extended and 0 <= index - self.parameter_base < len(PARAMETERS):
                self.setting_verify = (index, self.parameters.get(index), value)
            else:
                self.setting_verify = None
            self.setting_text = "설정 응답 대기"
        elif name == "SAVE":
            if self.setting_unverified:
                self.report("반영을 확인하지 못한 설정이 있습니다. 장비 값을 먼저 읽으세요.")
                return False
            self.setting_text = "EEPROM 저장 확인 대기"
        elif command in {"SL", "CD", "PL"}:
            self.diagnostic_name = command
        self.queue.append(command)
        self._pump()
        return self.pending is not None

    def enqueue_queries(self, commands):
        """Queue read-only synchronization; never queue motion retries."""
        for command in commands:
            if command.split()[0] not in QUERIES and command not in {"SPEED","PL"}:
                raise ValueError("Only queries may be queued")
        self.queue.extend(commands)
        self._pump()

    def receive(self, line):
        match = re.match(r"^([A-Za-z?]+)(?:\s+(.*))?$", line.strip())
        if not match:
            return
        name, body = match.group(1).upper(), (match.group(2) or "").strip()
        expected = self.pending[0] if self.pending and name == self.pending[0].split()[0] else None
        failed = body in FAILURES or re.fullmatch(r"E\d+", body)
        if failed and name!="PCONFIG":
            if expected:
                self.pending=None
                self.queue.clear()
                self._request_rejected(expected, body)
                self.report(line)
            return
        values = fields(body)
        try:
            if name in {"VERSION","VERS","VER"}:
                if not body or re.fullmatch(r"E\d+", body): return
                self.version = body
            elif name in {"GSTA","ST"}:
                if len(values) not in {5,7} or any(v not in {"0","1"} for v in values[:4]+values[5:]):
                    return
                if not re.fullmatch(r"E\d{4,}",values[4]):return
                self.grip = tuple(v == "1" for v in values[5:]) if len(values) == 7 else None
                self.grip_at = self.clock()
                self.next_grip_poll = self.clock() + 2
                if not self.extended:
                    self._update_state(DeviceState(values[0]=="1",values[2]=="1",values[3]=="1",
                                                   idle=values[0]=="0",error_code=values[4]), expected)
            elif name=="GSTATE":
                state = state_fields(body)
                if self.state and self.state.has_operation and not state.has_operation:
                    return
                if (self.state and self.state.has_operation and state.has_operation
                        and ((state.operation - self.state.operation) & 0xFFFFFFFF) >= 0x80000000):
                    return
                self._update_state(state, expected)
            elif name in {"SPEED","SPD"}:
                speed=int(body.split()[-1])
                if not 1<=speed<=100: return
                self.speed=speed
            elif name=="PCONFIG":
                if not expected:
                    return
                self.retry_schema_at = None
                if len(values)==2 and values[0]=="1" and 1<=int(values[1])<=1000:
                    self.schema=1;self.parameter_base=int(values[1])
                else:
                    self.schema=0;self.parameter_base=None
            elif name=="GPOS":
                if len(values)!=2 or values[0] not in {"Z","R"}: return
                self.positions[values[0]]=int(values[1])
                self.position_at[values[0]]=self.clock()
            elif name=="RPOS":
                self.positions["Z"]=int(body.rstrip(","))
                self.position_at["Z"]=self.clock()
            elif name=="PGET":
                if len(values)!=2: return
                index, number = int(values[0]), parameter_value(values[1])
                if expected and int(expected.split()[1]) != index:
                    return
                self.parameters[index] = number
                if expected:
                    self._settings_readback(index, number)
            elif name=="GSENSOR":
                self.sensors = sensor_fields(body)
                self.sensors_at = self.clock()
                self.sensor_idle = bool(self.fresh and self.state.idle and not self.state.busy
                                        and not self.motion_wait and not self.stop_wait)
                self.next_sensor_poll = self.clock() + 1
            elif name=="GLR":
                progress = longrun_fields(body)
                if not self.track_longrun or (self.lr_operation is not None and progress[1] == self.lr_limit):
                    self.longrun = progress
                    self.longrun_at = self.clock()
                if expected and self.lr_terminal and not progress[4] and progress[1] == self.lr_limit:
                    self.lr_final_count = progress[0]
            elif name in {"GERD","ERD"}:
                if expected:
                    self.error_detail=body
            elif expected in {"SL", "CD", "PL"}:
                data = snapshot(name, body)
                if name == "PL" and self.extended:
                    if data.keys() != set(range(self.parameter_base, self.parameter_base + len(PARAMETERS))):
                        return
                    self.parameters.update(data)
                self.diagnostics[name] = data
                self.diagnostic_at[name] = self.clock()
            elif name in MOTIONS and body.upper() != MOTION_ACK.get(name, "OK"):
                return
            elif expected and name == "STOP" and body.upper() != "STOP":
                return
            elif expected and name in {"SAVE", "PL", "TEACH"} and body != "OK":
                return
        except (ValueError, IndexError, OverflowError):
            return
        if expected:
            command=expected
            if name=="GPOS" and command.split()[1]!=values[0]:return
            self.last_response=line
            self.pending=None
            if name=="PCONFIG" and self.extended:
                self.queue.appendleft("GSTATE")
            elif name in MOTIONS or name in {"STOP","CLR","CLEAR","DRT","PAUSE","RESUME"}:
                if name in MOTIONS:
                    self.motion_acknowledged = True
                    self.operation_text = f"{name} · 접수됨 / 실행 확인 대기"
                elif name == "STOP":
                    self.stop_acknowledged = True
                elif name in {"PAUSE", "RESUME"}:
                    self.pause_acknowledged = body.upper() == name
                    if not self.pause_acknowledged:
                        self.pause_target = None
                        self.operation_text = f"{name} · 요청 거절"
                self.queue.appendleft("GSTATE" if self.extended else "GSTA")
            elif name in {"TEACH","PL"} and "," in command:
                if self.setting_verify:
                    self.setting_unverified.add(self.setting_verify[0])
                    self.setting_text = "RAM 값 확인 중"
                    self.queue.appendleft(f"PGET {self.setting_verify[0]}")
                else:
                    self.setting_text = "RAM 변경 응답 · 이 항목의 읽기 검증 미지원"
            elif name == "SAVE":
                self.verified_settings.clear()
                self.setting_text = "EEPROM 저장 확인"
            elif name in {"LOAD", "FACT", "FACTORY"}:
                self.parameters.clear()
                self.positions.clear()
                self.position_at.clear()
                self.verified_settings.clear()
                self.setting_unverified.clear()
                self.setting_text = "장비 설정 재조회 필요"
                self.state_at = None
                self.queue.appendleft("GSTATE" if self.extended else "GSTA")
                if self.extended:
                    self.queue.append("PL")
            elif name == "GLR" and self.lr_final_count is not None:
                self.queue.appendleft("GSTATE")
            self._pump()

    def _settings_readback(self, index, number):
        self.setting_unverified.discard(index)
        if self.setting_verify is None or index != self.setting_verify[0]:
            return
        _, before, requested = self.setting_verify
        self.setting_verify = None
        if not math.isclose(number, requested, rel_tol=2e-7 if index in
                            {self.parameter_base, (self.parameter_base or 0)+1} else 0):
            self.setting_unverified.add(index)
            self.setting_text = f"RAM 검증 불일치: 요청 {requested:g} / 읽음 {number:g}"
            self.report(self.setting_text)
            return
        original = self.verified_settings.get(index, (before, number))[0]
        self.verified_settings[index] = (original, number)
        self.setting_text = f"RAM 확인: {format(before, 'g') if before is not None else '미조회'} → {number:g}"

    def _request_rejected(self, command, reason):
        name = command.split()[0]
        if name in MOTIONS:
            self.motion_wait = False
            self.active_motion = None
            self.operation_text = f"{name} · 접수 거절: {reason}"
            if name in LONGRUN_COMMANDS:
                self._finish_longrun("접수 거절", error=reason)
        elif name == "STOP":
            self.operation_text = f"STOP · 정지 여부 미확인: {reason}"
        elif name in {"PAUSE", "RESUME"}:
            self.pause_target = None
            self.operation_text = f"{name} · 요청 거절: {reason}"
        elif name == "SAVE" or command.startswith(("PL ", "TEACH ", "PGET ")):
            self.setting_verify = None
            self.setting_text = f"{name} · 확인 실패: {reason}"

    def _request_unknown(self, command):
        name = command.split()[0]
        if name in MOTIONS or name == "STOP":
            self.operation_text = f"{name} · 결과 미확인"
        elif name in {"PAUSE", "RESUME"}:
            self.pause_target = None
            self.operation_text = f"{name} · 적용 미확인"
        elif name == "SAVE" or (self.setting_verify and name in {"PL", "TEACH", "PGET"}):
            if self.setting_verify:
                self.setting_unverified.add(self.setting_verify[0])
            self.setting_text = f"{name} · 결과 미확인, 장비 값 재조회 필요"

    def _finish_longrun(self, result, count=None, error=None):
        if self.lr_started is not None:
            self.longrun_history.append(LongrunRecord(
                self.lr_started_wall, self.lr_limit, count, self.longrun_elapsed, result,
                error or (self.state.error_code if self.state else "")))
        self.lr_started = None
        self.lr_terminal = None
        self.lr_deadline = None
        self.lr_final_count = None
        self.track_longrun = False

    def _terminal_longrun(self, result):
        if self.lr_started is None or self.lr_terminal:
            return
        if self.extended and self.lr_operation is not None and self.lr_entered:
            now = self.clock()
            self.lr_terminal = (result, self.state.error_code, now)
            self.lr_deadline = now + 5
            self.next_longrun_poll = now
        else:
            self._finish_longrun(result + " · 최종 횟수 미확인")

    def _update_state(self, state, expected):
        previous_code = self.state.error_code if self.state and self.state.error else None
        new_operation = bool(state.has_operation and self.state and self.state.has_operation
                             and state.operation != self.state.operation)
        self.state = state
        if not state.error or state.error_code != previous_code:
            self.error_detail = ""
            self.error_queried = None
        self.state_at=self.clock()
        self.uncertain=False
        self.next_poll = self.clock() + 0.5
        idle = state.idle and not state.busy
        if not idle or new_operation:
            self.sensor_idle = False
        if self.lr_terminal and self.lr_final_count is not None and expected == "GSTATE":
            result, error, _ = self.lr_terminal
            if not idle or state.operation != self.lr_operation or not state.has_operation:
                self._finish_longrun(result + " · 최종 횟수 미확인", error=error)
            else:
                if result == "목표 달성" and self.lr_final_count != self.lr_limit:
                    result = "종료 · 목표 미달"
                self._finish_longrun(result, self.lr_final_count, error)
        if (self.motion_wait and self.motion_acknowledged and state.has_operation
                and state.operation == ((self.motion_base + 1) & 0xFFFFFFFF)):
            self.motion_id = state.operation
            if self.active_motion in LONGRUN_COMMANDS:
                self.lr_operation = state.operation
                self.lr_entered |= (state.result == 2 or
                                    (state.result == 1 and state.phase == 16 and state.busy))
        if self.stop_wait:
            if self.stop_acknowledged and idle:
                self.stop_wait = False
                self.motion_wait = False
                self.active_motion = None
                self.disconnected_during_motion = False
                self.operation_text = "STOP · 정지 확인" if self.extended else "STOP · Busy 해제 (기존 F/W)"
                if self.lr_operation is not None and self.lr_operation == state.operation:
                    outcome = "오류" if state.result == 4 else "목표 달성" if state.result == 2 and self.lr_limit else "STOP"
                else:
                    outcome = "STOP · 실행 여부 미확인"
                self._terminal_longrun(outcome)
            return
        if self.pause_target is not None:
            if self.pause_acknowledged and state.paused == self.pause_target and state.busy:
                self.operation_text = "일시정지 확인" if self.pause_target else "재개 확인"
                self.pause_target = None
            elif idle or self.clock() >= self.pause_deadline:
                self.operation_text = "일시정지/재개 적용 미확인"
                self.pause_target = None
        if not self.motion_wait:
            return
        if self.motion_correlate:
            running = state.has_operation and self.motion_id == state.operation and state.result == 1
            finished = state.has_operation and self.motion_id == state.operation and state.result in {2, 3, 4}
        else:
            running = state.busy or not state.idle
            finished = self.motion_acknowledged and (self.observed_motion or state.error)
        if running and not idle:
            self.observed_motion = True
            self.completion_unknown = False
            if self.pause_target is None and self.operation_text not in {"일시정지 확인", "재개 확인"}:
                self.operation_text = f"{self.active_motion} · {'일시정지' if state.paused else '동작 중'}"
        if finished and idle:
            result = "오류" if state.error or state.result == 4 else "STOP" if state.result == 3 else "완료"
            display = "F/W 완료 응답" if result == "완료" and self.active_motion in AUTOMATIC else result
            self.operation_text = f"{self.active_motion} · {display}"
            self.motion_wait = False
            self.completion_unknown = False
            self._terminal_longrun("목표 달성" if result == "완료" and self.lr_limit else result)
            self.active_motion = None
        elif idle and ((self.motion_acknowledged and not self.motion_correlate) or
                       (self.motion_started is not None and self.clock() - self.motion_started >= 3)):
            if not self.completion_unknown:
                self.report("완료를 확인하지 못했습니다. STOP 후 상태를 다시 확인하세요.")
            self.completion_unknown = True

    def tick(self, poll=True, sensors=False):
        if self.pending and self.clock()>=self.pending[1]:
            command=self.pending[0]
            if command=="PCONFIG":
                self.schema=0;self.parameter_base=None
                self.retry_schema_at = self.clock() + 10
            self.pending=None;self.queue.clear();self.uncertain=True
            self._request_unknown(command)
            if command == "GLR" and self.lr_terminal:
                self._finish_longrun(self.lr_terminal[0] + " · 최종 횟수 미확인")
            self.report(f"응답 시간 초과: {command}. 동작 결과를 확인하세요.")
            self.next_poll=self.clock()+0.5
            return
        if not poll or not self.connected or self.pending or self.queue:
            return
        now = self.clock()
        if self.lr_terminal and now >= self.lr_deadline:
            self._finish_longrun(self.lr_terminal[0] + " · 최종 횟수 미확인")
        if self.lr_terminal and now >= self.next_longrun_poll:
            command = "GLR"
            self.next_longrun_poll = now + 0.5
        elif now >= self.next_poll:
            command = "GSTATE" if self.extended else "GSTA"
            self.next_poll = now + 0.5
        elif now >= self.next_discovery and (not self.version or self.schema is None or
                                             (self.retry_schema_at is not None and now >= self.retry_schema_at)):
            command = "VERSION" if not self.version else "PCONFIG"
            self.next_discovery = now + 2
        elif self.state and self.state.error and self.error_queried != self.state.error_code:
            command = "GERD"
            self.error_queried = self.state.error_code
        elif self.extended and self.track_longrun and self.lr_operation is not None and now >= self.next_longrun_poll:
            command = "GLR"
            self.next_longrun_poll = now + 1
        elif self.extended and sensors and now >= self.next_sensor_poll:
            command = "GSENSOR"
            self.next_sensor_poll = now + 1
        elif self.extended and sensors and now >= self.next_grip_poll:
            command = "GSTA"
            self.next_grip_poll = now + 2
        else:
            return
        self.queue.append(command)
        self._pump()

    def target_in_range(self, axis, target):
        if not self.extended:return False
        b=self.parameter_base
        offsets=(7,9,10) if axis=="Z" else (8,11,12)
        if any(b+i not in self.parameters for i in (*offsets,13)):return False
        limit=self.parameters[b+offsets[0]]
        low,high=-limit,limit
        if self.parameters[b+13]:
            low=max(low,self.parameters[b+offsets[1]])
            high=min(high,self.parameters[b+offsets[2]])
        return 0<limit<=8388607 and low<high and low<=target<=high

    def parameter_issue(self, offset, value):
        if offset in {2, 3, 13} and value not in {0, 1}:
            return "전류 선택과 소프트 리미트 사용은 0 또는 1입니다."
        if offset == 6 and not 0 <= value <= 8:
            return "분해능 코드는 0~8입니다."
        if offset in {4, 5} and not 0 <= value <= 100:
            return "정지 전류 비율은 0~100%입니다."
        if offset in {7, 8} and not 1 <= value <= 8388607:
            return "기본 리미트는 1~8388607 pulse입니다."
        if offset in {14, 15, 19, 20} and value <= 0:
            return "속도와 가속도는 0보다 커야 합니다."
        if offset in {16, 17, 18, 21} and not self.target_in_range("R" if offset == 21 else "Z", value):
            return "장비 리미트 밖이거나 리미트를 확인하지 못했습니다."
        if offset in {0, 1, 2, 3}:
            current = self.parameters.get(self.parameter_base + offset % 2)
            code = self.parameters.get(self.parameter_base + offset % 2 + 2)
            if offset in {0, 1}:
                current = value
            else:
                code = value
            max_current = 1.7 if code == 1 else 3.1
            if current is not None and not 0 < current <= max_current:
                return f"실행 전류는 선택한 범위에서 0 초과 {max_current:g} A 이하여야 합니다."
        if offset in {7, 8, 9, 10, 11, 12, 13}:
            values = {i: self.parameters.get(self.parameter_base + i) for i in range(7, 22)}
            values[offset] = value
            for basic, neg, pos, targets in ((7, 9, 10, (16, 17, 18)), (8, 11, 12, (21,))):
                if offset not in {basic, neg, pos, 13}:
                    continue
                limit, low, high = (values[i] for i in (basic, neg, pos))
                if None in (limit, low, high):
                    return "먼저 현재 장비 리미트를 읽어주세요."
                if low >= high:
                    return "음의 리미트가 양의 리미트보다 작아야 합니다."
                lower, upper = (-limit, limit)
                if values[13]:
                    lower, upper = max(lower, low), min(upper, high)
                if lower >= upper:
                    return "기본 리미트와 소프트 리미트의 공통 이동 범위가 없습니다."
                if any(values[i] is not None and not lower <= values[i] <= upper for i in targets):
                    return "새 리미트에 이미 읽은 티칭 값이 포함되지 않습니다."
        return ""
