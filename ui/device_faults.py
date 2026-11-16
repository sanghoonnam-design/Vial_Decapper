"""Decapper fault names and operator text for the firmware GERD reply."""
import re


FAULTS = {
    "E2800": ("동작 시간 초과", "제한 시간 안에 축의 정지 또는 목표 위치 도착이 확인되지 않았습니다."),
    "E2810": ("이동 한계 오류", "축 위치가 허용 범위를 벗어났거나 이동 한계 설정이 유효하지 않습니다."),
    "E2820": ("Decapper 상태 오류", "펌웨어가 비정상 동작 상태를 감지해 장비를 정지했습니다."),
    "E2850": ("Z HOME 미감지", "원점 탐색 중 Z HOME 센서가 감지 상태로 바뀌지 않았습니다."),
    "E2851": ("Z HOME 미해제", "원점 센서에서 벗어나는 방향으로 이동했지만 감지가 해제되지 않았습니다."),
    "E2860": ("Y Forward 리미트 미감지", "Y축 전진 끝 위치가 제한 시간 안에 확인되지 않았습니다."),
    "E2861": ("Y Back 리미트 미감지", "Y축 후진 끝 위치가 제한 시간 안에 확인되지 않았습니다."),
    "E2870": ("캡 그리퍼 열림 미확인", "캡을 놓는 단계에서 그리퍼 열림 조건이 충족되지 않았습니다."),
    "E2880": ("바디 그리퍼 파지 미확인", "바이알 몸체를 잡는 단계에서 파지 센서 조건이 충족되지 않았습니다."),
}

CONDITIONS = {
    "Z home sensor not reached during fast search": "Z HOME 고속 탐색",
    "Z home sensor not reached during slow search": "Z HOME 저속 재탐색",
    "Z home sensor did not release": "Z HOME 센서 해제 대기",
    "Y Forward limit not reached": "Y Forward 리미트 감지 대기",
    "Y Back limit not reached": "Y Back 리미트 감지 대기",
    "Cap release not confirmed": "캡 그리퍼 열림 확인",
    "Body hold not confirmed": "바디 그리퍼 파지 확인",
    "Z stop not confirmed after fast search": "고속 탐색 후 Z축 정지 미확인",
    "Z stop not confirmed after backing off": "센서 해제 이동 후 Z축 정지 미확인",
    "Z stop not confirmed after slow search": "저속 재탐색 후 Z축 정지 미확인",
    "R stop not confirmed before zeroing": "원점 지정 전 R축 정지 미확인",
    "Z origin stop or target not confirmed": "원위치 이동 중 Z축 정지 또는 도착 미확인",
    "Z approach stop or target not confirmed": "캡 접근 중 Z축 정지 또는 도착 미확인",
    "Z return stop or target not confirmed": "복귀 중 Z축 정지 또는 도착 미확인",
    "Z stop or target not confirmed": "Z축 정지 또는 목표 도착 미확인",
    "R stop or target not confirmed": "R축 정지 또는 목표 도착 미확인",
    "Z/R move exceeded the time limit": "Z/R 이동 시간 초과",
    "Abnormal Decapper state": "비정상 동작 상태 감지",
}
OPERATIONS = {"READY 0": "Y Forward", "READY 1": "Y Back", "ABNORMAL": "비정상 상태"}
SENSOR_LABELS = {
    "H": "Z HOME", "L": "Z Low", "F": "Y Forward", "B": "Y Back",
    "Open": "열림", "Close": "닫힘",
}
DETAIL_PATTERN = re.compile(
    r"(?:(?P<operation>.+?) step=(?P<step>\d+)(?: completed=(?P<count>\d+))?"
    r"|phase=(?P<phase>\d+)) - (?P<condition>.+)"
)
SENSOR_PATTERN = re.compile(
    r"(?P<condition>.+) \((?P<inputs>(?:[A-Za-z]+=[01] )+)"
    r"expected (?P<expected>[A-Za-z]+=[01]|[01]/[01])\)"
)


def _describe_condition(condition):
    sensors = SENSOR_PATTERN.fullmatch(condition)
    if sensors:
        actual = " ".join(sensors["inputs"].split())
        expected = sensors["expected"]
        if "/" in expected:
            opened, closed = expected.split("/")
            expected = f"Open={opened} Close={closed}"
        condition = CONDITIONS.get(sensors["condition"], sensors["condition"])
        return f"{condition} · 입력 {_sensor_text(actual)} / 기대 {_sensor_text(expected)}"
    limit = re.fullmatch(r"(Z|R) (target|position) (-?\d+) outside \[(-?\d+),(-?\d+)\] pulse", condition)
    if limit:
        axis, kind, value, lower, upper = limit.groups()
        field = "목표" if kind == "target" else "현재 위치"
        return f"{axis}축 {field} {value} pulse · 허용 {lower}~{upper} pulse"
    limit = re.fullmatch(r"(Z|R) limits invalid", condition)
    if limit:
        return f"{limit[1]}축 이동 한계 설정이 유효하지 않습니다."
    limit = re.fullmatch(r"(Z|R) (target|position) outside 32-bit range", condition)
    if limit:
        field = "목표" if limit[2] == "target" else "현재 위치"
        return f"{limit[1]}축 {field}가 32비트 위치 범위를 벗어났습니다."
    return CONDITIONS.get(condition, condition)


def _sensor_text(inputs):
    return ", ".join(f"{SENSOR_LABELS.get(key, key)}={value}"
                     for key, value in (field.split("=") for field in inputs.split()))


def fault_summary(code, detail):
    title, explanation = FAULTS.get(code, ("장비 오류", ""))
    heading = f"{code} · {title}"
    detail = " ".join(detail.split())
    match = DETAIL_PATTERN.fullmatch(detail)
    if match:
        operation = match["operation"]
        if operation and operation.startswith("LR/") and match["count"] is not None:
            where = f"Longrun {operation[3:]} · 완료 {match['count']}회 · 단계 {match['step']}"
        elif operation:
            where = f"{OPERATIONS.get(operation, operation)} · 단계 {match['step']}"
        else:
            where = f"동작 상태 {match['phase']}"
        return f"{heading}\n{where} · {_describe_condition(match['condition'])}"
    lines = [heading, explanation, detail.removeprefix(code + " ")]
    return "\n".join(line for line in lines if line)


def fault_tooltip(summary, detail):
    return f"{summary}\n\nF/W GERD 원문\n{detail}" if detail else summary
