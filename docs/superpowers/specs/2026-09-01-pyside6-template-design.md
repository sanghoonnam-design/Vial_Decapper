# PySide6 기본 프로젝트 템플릿 설계

## 목적

PySide6 기반의 데스크톱 GUI 프로그램을 시작할 수 있는 최소 실행 골격을 제공한다. UI 코드는 `ui/` 패키지 안에 두고, 프로그램 실행 책임은 `main.py`에 한정한다.

## 프로젝트 구조

```text
main.py
requirements.txt
README.md
.gitignore
ui/
    __init__.py
    main_window.py
```

기존 `test.py`는 사용자 파일이므로 수정하지 않는다.

## 구성 요소와 흐름

`main.py`는 `QApplication`을 만들고 `ui.main_window.MainWindow`를 생성하여 표시한 뒤 Qt 이벤트 루프를 실행한다. `MainWindow`는 창 제목, 최소 크기, 중앙 안내 레이블을 제공하는 기본 `QMainWindow`다. 이후 화면·위젯 코드는 `ui/` 아래의 별도 모듈로 추가한다.

## 의존성과 실행

`requirements.txt`에는 PySide6 의존성을 선언한다. 사용자는 가상환경을 만든 뒤 `pip install -r requirements.txt`를 실행하고 `python main.py`로 프로그램을 시작한다. 이 절차는 `README.md`에 기록한다.

## Git 준비

프로젝트는 아직 Git 저장소가 아니므로 초기화나 원격 저장소 연결은 사용자가 원할 때 수행한다. `.gitignore`에는 가상환경, Python 바이트코드·캐시, 테스트·커버리지 산출물, IDE 설정 및 Qt가 생성하는 임시 파일을 등록해 소스와 문서만 추적되도록 한다.

## 오류 처리와 검증

시작 중 발생한 예외는 숨기지 않고 콘솔에 표시한다. 구현 후에는 Python 문법 컴파일 검사와 Qt 플랫폼을 오프스크린으로 설정한 비대화형 시작 검사를 수행해, 창 생성과 이벤트 루프 진입 전 과정을 확인한다.
