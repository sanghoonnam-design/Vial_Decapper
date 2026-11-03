"""Decapper wire values and read-only operation summaries."""
from dataclasses import dataclass
from datetime import datetime
import math
import re


SENSOR_NAMES = ("CT", "Z High", "Z Low", "Y Forward", "Y Back",
                "Cap Open", "Cap Close", "Body Open", "Body Close")
PHASE_NAMES = ("대기", "정지", "HOME", "DECAP", "CAP", "일시정지", "재개", "ORG",
               "R 상대 이동", "R 절대 이동", "Z 상대 이동", "Z 절대 이동",
               "Y Forward", "Y Back", "유닛 DECAP", "유닛 CAP", "Longrun",
               "Body 그리퍼", "Cap 그리퍼", "이상", "종료")
ERROR_NAMES = {"E2800": "동작 시간 초과", "E2810": "이동 한계 초과",
               "E2820": "Decapper 상태 오류"}
RESULT_NAMES = ("기록 없음", "접수 / 실행", "완료 응답", "중단", "오류")


@dataclass
class DeviceState:
    busy: bool
    homed: bool
    error: bool
    paused: bool = False
    phase: int = 0
    idle: bool = True
    error_code: str = "E0000"
    operation: int = 0
    result: int = 0
    has_operation: bool = False


@dataclass
class LongrunRecord:
    started: datetime
    limit: int
    count: int | None
    elapsed: float
    result: str
    error_code: str


def fields(text):
    return [value.strip() for value in text.rstrip(",").split(",")]


def state_fields(text):
    values = fields(text)
    if len(values) not in {7, 9} or any(values[i] not in {"0", "1"} for i in (0, 1, 2, 3, 5)):
        raise ValueError
    phase = int(values[4])
    if not re.fullmatch(r"E\d{4,}", values[6]) or not 0 <= phase <= 10000:
        raise ValueError
    operation, result = map(int, values[7:]) if len(values) == 9 else (0, 0)
    if not 0 <= operation <= 0xFFFFFFFF or not 0 <= result <= 4:
        raise ValueError
    return DeviceState(*(values[i] == "1" for i in range(4)), phase=phase,
                       idle=values[5] == "1", error_code=values[6],
                       operation=operation, result=result, has_operation=len(values) == 9)


def sensor_fields(text):
    values = fields(text)
    if len(values) != len(SENSOR_NAMES) or any(value not in {"0", "1"} for value in values):
        raise ValueError
    return tuple(map(int, values))


def longrun_fields(text):
    values = fields(text)
    if len(values) != 5 or any(value not in {"0", "1"} for value in values[2:]):
        raise ValueError
    count, limit, phase, paused, busy = map(int, values)
    if not 0 <= count <= 0xFFFFFFFF or not 0 <= limit <= 1000000 or (limit and count > limit):
        raise ValueError
    return count, limit, phase, paused, busy


def parameter_value(text):
    value = float(text)
    if not math.isfinite(value):
        raise ValueError
    return value


def snapshot(name, text):
    if name == "PL":
        result = {}
        for pair in text.split(";"):
            index, value = fields(pair)
            index = int(index)
            if not 1 <= index <= 4096 or index in result:
                raise ValueError
            result[index] = parameter_value(value)
        return result
    items = [entry.split("=", 1) for entry in text.split(";")]
    if any(len(entry) != 2 for entry in items):
        raise ValueError
    result = dict(items)
    if len(result) != len(items):
        raise ValueError
    if name == "SL" and result.keys() == {"STATE", "SENSOR"}:
        return {"STATE": state_fields(result["STATE"]), "SENSOR": sensor_fields(result["SENSOR"])}
    if name == "CD" and result.keys() == {"TARGET_Z", "TARGET_R", "SPEED", "LR"}:
        speed = int(result["SPEED"])
        if not 1 <= speed <= 100:
            raise ValueError
        return {"TARGET_Z": int(result["TARGET_Z"]), "TARGET_R": int(result["TARGET_R"]),
                "SPEED": speed, "LR": longrun_fields(result["LR"])}
    raise ValueError
