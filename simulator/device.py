"""Timed simulation of the commands used by Vial Decapper Software."""
from dataclasses import dataclass
import math
import re
from time import monotonic


PARAMETER_BASE = 35
DEFAULT_PARAMETERS = (
    1.2, 1.2, 0, 0, 30, 30, 4, 160000, 100000, -160000, 160000,
    -100000, 100000, 1, 300000, 20000, 10000, 15000, 0, 300000, 20000, 6400,
)
TEACH_AXES = {16: "Z", 17: "Z", 18: "Z", 21: "R"}
SAVE_OFFSETS = {"Z": 16, "R": 21}
# Representative fault-time inputs and sequence steps from CDecap.
FAULT_CASES = {
    "E2800": ("CAP", 8, "Z/R move exceeded the time limit"),
    "E2810": ("MOVE A Z", 0, "Z position 160001 outside [-160000,160000] pulse"),
    "E2820": ("ABNORMAL", 0, "Abnormal Decapper state"),
    "E2850": ("HOME", 3, "Z home sensor not reached during fast search (H=1 L=1 expected H=0)"),
    "E2851": ("HOME", 6, "Z home sensor did not release (H=0 L=1 expected H=1)"),
    "E2860": ("READY 0", 0, "Y Forward limit not reached (F=1 B=0 expected F=0)"),
    "E2861": ("READY 1", 0, "Y Back limit not reached (F=0 B=1 expected B=0)"),
    "E2870": ("CAP", 9, "Cap release not confirmed (Open=0 Close=0 expected 1/0)"),
    "E2880": ("DECAP", 2, "Body hold not confirmed (Open=1 Close=0 expected 0/0)"),
}
SEQUENCE_STEPS = {
    "E2800": {"CAP": 8, "DECAP": 8, "UCAP": 8, "UDECAP": 8},
    "E2860": {"CAP": 11, "DECAP": 11, "HOME": 13},
    "E2861": {"CAP": 3, "DECAP": 4},
    "E2870": {"CAP": 9, "UCAP": 9},
    "E2880": {"CAP": 2, "DECAP": 2},
}
ALIASES = {
    "VER": "VERSION", "VERS": "VERSION", "ST": "GSTA", "SPD": "SPEED",
    "ERR": "GERR", "ERD": "GERD", "CLEAR": "CLR", "DRT": "CLR", "CAPDECAPLR": "LR",
}
FIXED_MOTIONS = {
    "HOME": (2, "home"), "DECAP": (3, "Decap"), "CAP": (4, "Cap"),
    "ORG": (7, "origin"), "UDECAP": (14, "udecap"), "UCAP": (15, "ucap"),
}
PHASE_LABELS = {
    2: "HOME", 3: "DECAP", 4: "CAP", 7: "ORG", 8: "R 상대 이동", 9: "R 절대 이동",
    10: "Z 상대 이동", 11: "Z 절대 이동", 12: "Y Forward", 13: "Y Back",
    14: "유닛 DECAP", 15: "유닛 CAP", 17: "바디 그리퍼", 18: "캡 그리퍼",
}


def csv(*values):
    return ",".join(str(int(value)) if isinstance(value, bool) else str(value) for value in values)


def parameter_text(value):
    return format(value, ".9g") if isinstance(value, float) else str(value)


@dataclass
class Motion:
    command: str
    phase: int
    deadline: float
    duration: float
    axis: str = ""
    target: int = 0
    paused_at: float | None = None


class SimulatedDevice:
    def __init__(self, clock=monotonic):
        self.clock = clock
        self.parameters = list(DEFAULT_PARAMETERS)
        self.saved = self.parameters.copy()
        self.position = {"Z": 0, "R": 0}
        self.targets = self.position.copy()
        self.capture = {}
        self.homed = False
        self.vial_present = True
        self.y_forward = True
        self.body_vial_detected = True
        self.body_closed = False
        self.cap_closed = False
        self.cap_held = False
        self.speed = 100
        self.error = "E0000"
        self.fault_detail = ""
        self.operation = 0
        self.result = 0
        self.motion = None
        self.lr_count = 0
        self.lr_limit = 0
        self.lr_phase = 0

    @property
    def busy(self):
        return self.motion is not None

    @property
    def paused(self):
        return self.busy and self.motion.paused_at is not None

    def inject_fault(self, code):
        if code not in FAULT_CASES:
            raise ValueError(code)
        if self.error != "E0000":
            return
        operation, step, condition = FAULT_CASES[code]
        steps = SEQUENCE_STEPS.get(code, {})
        motion = self.motion
        if motion and motion.command == "LR":
            half = "CAP" if self.lr_phase else "DECAP"
            unit_operation = f"U{half}"
            if unit_operation in steps:
                operation, step = f"LR/{half}", steps[unit_operation]
        elif motion:
            if motion.command in steps:
                operation, step = motion.command, steps[motion.command]
            elif code == "E2800" and motion.command == "MOVE":
                mode = "R" if motion.phase in {8, 10} else "A"
                operation, step = f"MOVE {mode} {motion.axis}", 0
                condition = f"{motion.axis} stop or target not confirmed"
        count = f" completed={self.lr_count}" if operation.startswith("LR/") else ""
        self.fault_detail = f"{operation} step={step}{count} - {condition}"
        self.error = code
        self._finish(4)
        self.lr_phase = 0
        self.capture.clear()

    def clear_fault(self):
        self.error = "E0000"
        self.fault_detail = ""

    def set_vial(self, present):
        self.vial_present = present
        if self.y_forward:
            self.body_vial_detected = present

    def _finish(self, result):
        self.motion = None
        self.result = result

    def _start(self, command, phase, duration, axis="", target=0):
        self.operation = (self.operation + 1) & 0xFFFFFFFF
        self.result = 1
        self.motion = Motion(command, phase, self.clock() + duration, duration, axis, target)

    def _cap_action(self, decap):
        self.position.update(Z=self.parameters[18], R=self.parameters[21] if decap else 0)
        self.cap_held = decap
        self.cap_closed = decap

    def tick(self):
        motion = self.motion
        if not motion or self.paused or self.clock() < motion.deadline:
            return
        command = motion.command
        if command == "LR":
            self._cap_action(self.lr_phase == 0)
            if self.lr_phase == 1:
                self.lr_count += 1
                if self.lr_limit and self.lr_count >= self.lr_limit:
                    self._finish(2)
                    return
            self.lr_phase = 1 - self.lr_phase
            motion.deadline = self.clock() + motion.duration
            return
        if command == "HOME":
            self.homed = True
            self.position.update(Z=0, R=0)
            self.y_forward = True
            self.body_vial_detected = self.vial_present
        elif command == "ORG":
            self.position.update(Z=self.parameters[18], R=0)
        elif command == "MOVE":
            self.position[motion.axis] = motion.target
        elif command == "READY":
            self.y_forward = motion.target == 0
            if self.y_forward:
                self.body_vial_detected = self.vial_present
        elif command == "BGRIP":
            self.body_closed = bool(motion.target)
        elif command == "CGRIP":
            self.cap_closed = bool(motion.target)
            if not self.cap_closed:
                self.cap_held = False
        elif command in {"CAP", "DECAP", "UCAP", "UDECAP"}:
            if command.startswith("U") or (self.homed and self.vial_present):
                self._cap_action(command.endswith("DECAP"))
        self._finish(2)

    def _state(self):
        phase = self.motion.phase if self.busy else 0
        return csv(self.busy, self.homed, self.error != "E0000", self.paused, phase,
                   not self.busy, self.error, self.operation, self.result)

    def _sensors(self):
        cap_switches = (0, 0) if self.cap_held else (int(not self.cap_closed), int(self.cap_closed))
        return csv(self.vial_present and self.y_forward, self.position["Z"] == 0, False,
                   not self.y_forward, self.y_forward, *cap_switches,
                   not self.body_closed, self.body_closed)

    def _progress(self):
        return csv(self.lr_count, self.lr_limit, self.lr_phase, self.paused, self.busy)

    def _in_range(self, axis, target):
        limit, lower, upper = (7, 9, 10) if axis == "Z" else (8, 11, 12)
        high = self.parameters[limit]
        low = -high
        if self.parameters[13]:
            low, high = max(low, self.parameters[lower]), min(high, self.parameters[upper])
        return low <= target <= high

    def _set_parameter(self, name, args):
        if len(args) != 2:
            return "INVALID"
        offset = int(args[0]) - PARAMETER_BASE
        if not 0 <= offset < len(self.parameters) or (name == "TEACH" and offset not in TEACH_AXES):
            return "INVALID"
        value = float(args[1]) if offset < 2 else int(args[1])
        if not math.isfinite(value):
            return "INVALID"
        axis = TEACH_AXES.get(offset)
        if axis and not self._in_range(axis, value):
            return "RANGE"
        self.parameters[offset] = value
        self.capture.clear()
        return "OK"

    def _query(self, name, args):
        if name == "GPOS":
            return csv(args[0], self.position[args[0]]) if len(args) == 1 and args[0] in self.position else "INVALID"
        if name == "RPOS":
            axes = args or ["Z", "R"]
            if args and (len(args) != 1 or args[0] not in self.position):
                return "INVALID"
            self.capture.update((axis, self.position[axis]) for axis in axes)
            return csv(*(self.position[axis] for axis in axes))
        if name == "PGET":
            if len(args) != 1:
                return "INVALID"
            index = int(args[0])
            offset = index - PARAMETER_BASE
            return csv(index, parameter_text(self.parameters[offset])) if 0 <= offset < len(self.parameters) else "INVALID"
        if args:
            return "INVALID"
        if name == "VERSION":
            return "SIM 1.0"
        if name == "PCONFIG":
            return f"1,{PARAMETER_BASE}"
        if name == "GSTATE":
            return self._state()
        if name == "GSTA":
            return csv(self.busy, 1, self.homed, self.error != "E0000", self.error,
                       self.cap_held, self.body_vial_detected)
        if name == "GSENSOR":
            return self._sensors()
        if name == "GLR":
            return self._progress()
        if name == "GERR":
            return self.error
        if name == "GERD":
            return self.fault_detail or "No Error."
        if name == "SL":
            return f"STATE={self._state()};SENSOR={self._sensors()}"
        if name == "CD":
            return f"TARGET_Z={self.targets['Z']};TARGET_R={self.targets['R']};SPEED={self.speed};LR={self._progress()}"
        if name == "PL":
            return ";".join(csv(index, parameter_text(value))
                            for index, value in enumerate(self.parameters, PARAMETER_BASE))
        return "UNSUPPORTED"

    def _save(self, args):
        if args == ["1"]:
            self.saved = self.parameters.copy()
            self.capture.clear()
            return "OK"
        if len(args) != 1 or args[0] not in SAVE_OFFSETS:
            return "INVALID"
        axis = args[0]
        if axis not in self.capture:
            return "NO_RPOS"
        target = self.capture[axis]
        if not self._in_range(axis, target):
            return "RANGE"
        offset = SAVE_OFFSETS[axis]
        self.parameters[offset] = self.saved[offset] = target
        del self.capture[axis]
        return "OK"

    def _move(self, args):
        if len(args) != 3 or args[0] not in {"A", "R"} or args[1] not in self.position:
            return "INVALID"
        mode, axis, pulse = args
        pulse = int(pulse)
        target = pulse + self.position[axis] if mode == "R" else pulse
        if not self._in_range(axis, target):
            return "RANGE"
        self.targets[axis] = pulse
        phase = (8 if axis == "R" else 10) + (mode == "A")
        self._start("MOVE", phase, 0.6, axis, target)
        return "OK"

    def _execute(self, name, args):
        if name == "STOP":
            if args:
                return "INVALID"
            if self.busy:
                self._finish(3)
            return "stop"
        if name in {"PAUSE", "RESUME"}:
            if args or not self.busy or self.motion.phase not in {3, 4, 16}:
                return "INVALID"
            if name == "PAUSE" and not self.paused:
                self.motion.paused_at = self.clock()
                return "pause"
            if name == "RESUME" and self.paused:
                self.motion.deadline += self.clock() - self.motion.paused_at
                self.motion.paused_at = None
                return "resume"
            return "INVALID"
        if name == "SPEED":
            if args:
                if len(args) != 1 or not 1 <= int(args[0]) <= 100:
                    return "INVALID"
                self.speed = int(args[0])
            return f"SPEED {self.speed}"
        if name in FIXED_MOTIONS or name in {"MOVE", "READY", "BGRIP", "CGRIP", "LR"}:
            if self.error != "E0000":
                return self.error
            if self.busy:
                return "BUSY"
            return self._motion_command(name, args)
        if name in {"CLR", "LOAD", "SAVE", "TEACH"} or (name == "PL" and args):
            if self.busy:
                return "BUSY"
            if name == "CLR" and not args:
                self.clear_fault()
                return ""
            if name == "LOAD" and not args:
                self.parameters = self.saved.copy()
                self.homed = False
                self.position.update(Z=0, R=0)
                self.capture.clear()
                return "OK HOME_REQUIRED"
            if name == "SAVE":
                return self._save(args)
            if name in {"TEACH", "PL"}:
                return self._set_parameter(name, args)
            return "INVALID"
        return self._query(name, args)

    def _motion_command(self, name, args):
        if name in FIXED_MOTIONS:
            if args:
                return "INVALID"
            phase, reply = FIXED_MOTIONS[name]
            if name == "HOME":
                self.capture.clear()
            self._start(name, phase, 1.3 if name in {"CAP", "DECAP"} else 0.7)
            return reply
        if name == "MOVE":
            return self._move(args)
        if name == "LR":
            limit = int(args[0]) if len(args) == 1 else 0
            if args and (len(args) != 1 or not 1 <= limit <= 1000000):
                return "INVALID"
            self.lr_count, self.lr_limit, self.lr_phase = 0, limit, 0
            self._start(name, 16, 0.65)
            return "LR"
        if len(args) != 1 or args[0] not in {"0", "1"}:
            return "INVALID"
        target = int(args[0])
        phase = {"READY": 12 + target, "BGRIP": 17, "CGRIP": 18}[name]
        self._start(name, phase, 0.5, target=target)
        return "OK"

    def command(self, line):
        self.tick()
        words = re.split(r"[ ,]+", line.strip().upper())
        name, *args = words
        if not re.fullmatch(r"[A-Z]+", name):
            return None
        try:
            reply = self._execute(ALIASES.get(name, name), args)
        except (ValueError, OverflowError):
            reply = "INVALID"
        return f"{name} {reply}".rstrip()
