"""Explicit position shortcuts, advanced only by verified device responses."""
from collections import deque
import re


class PositionActions:
    def __init__(self, session, report=lambda message: None):
        self.session, self.report = session, report
        self.phase = None
        self.moves = deque()
        self.label = ""
        self.read_index = None
        self.response_base = 0
        self.expected_operation = None

    @property
    def active(self):
        return self.phase is not None

    def ready(self):
        s = self.session
        return (not self.active and s.can_move and s.extended and s.state.has_operation
                and not s.pending and not s.queue)

    def _begin(self, label):
        if not self.ready():
            self.report("좌표 이동은 확장 F/W의 최신 대기 상태와 완료 확인이 필요합니다.")
            return False
        self.label = label
        self.moves.clear()
        return True

    def origin(self):
        if not self._begin("원점 이동"):
            return False
        if not all(self.session.target_in_range(axis, 0) for axis in ("Z", "R")):
            self.report("Z/R의 0 좌표가 리미트 밖이거나 리미트를 확인하지 못했습니다.")
            return False
        self.moves.extend((("Z", 0), ("R", 0)))
        return self._next_move()

    def cap(self, offset):
        if offset not in {16, 17}:
            return False
        if not self._begin("CAP UP" if offset == 16 else "CAP SIDE"):
            return False
        self.phase = "read"
        self.read_index = self.session.parameter_base + offset
        self.response_base = self.session.response_serial
        self.session.operation_text = f"{self.label} · F/W 좌표 읽는 중"
        if not self.session.request(f"PGET {self.read_index}"):
            self.cancel("좌표 조회를 전송하지 못했습니다.")
            return False
        return True

    def _next_move(self):
        if not self.moves:
            self.phase = None
            self.session.operation_text = f"{self.label} · 이동 완료"
            return True
        s = self.session
        axis, target = self.moves.popleft()
        if not s.can_move or not s.target_in_range(axis, target):
            self.cancel("장비 대기 상태 또는 좌표 리미트를 확인하지 못했습니다.")
            return False
        self.phase = "move"
        self.expected_operation = (s.state.operation + 1) & 0xFFFFFFFF
        if not s.request(f"MOVE A {axis} {target}"):
            self.cancel("이동 명령을 전송하지 못했습니다.")
            return False
        s.operation_text = f"{self.label} · {axis}={target} 이동 확인 중"
        return True

    def cancel(self, reason):
        if self.active:
            self.phase = None
            self.moves.clear()
            message = f"{self.label} · 후속 이동 취소: {reason}"
            self.session.operation_text = message
            self.report(message)

    def tick(self):
        if not self.active:
            return
        s = self.session
        if (not s.connected or not s.fresh or s.uncertain or s.stop_wait
                or s.state.error or s.completion_unknown or s.disconnected_during_motion):
            self.cancel("연결·상태·이동 결과를 확인할 수 없습니다.")
            return
        if self.phase == "read":
            if s.response_serial == self.response_base:
                if not s.pending:
                    self.cancel("좌표 응답을 확인하지 못했습니다.")
                return
            match = re.fullmatch(r"PGET\s+([0-9]+),([+-]?[0-9]+)", s.last_response)
            if not match or int(match[1]) != self.read_index:
                self.cancel("유효한 좌표 응답이 아닙니다.")
                return
            value = int(match[2])
            if not s.target_in_range("Z", value):
                self.cancel("CAP 좌표가 리미트 밖입니다.")
                return
            self.moves.append(("Z", value))
            self._next_move()
        elif not s.pending and not s.queue:
            if s.motion_wait:
                return
            if (not s.state.has_operation or s.state.operation != self.expected_operation
                    or not s.state.idle or s.state.busy or s.state.result != 2):
                self.cancel("동작의 정상 완료를 확인하지 못했습니다.")
                return
            self._next_move()
