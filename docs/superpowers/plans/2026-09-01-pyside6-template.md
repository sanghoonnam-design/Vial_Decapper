# PySide6 Template Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a runnable PySide6 desktop-application template whose UI code is isolated in `ui/`.

**Architecture:** `main.py` owns application startup and imports `MainWindow` from `ui.main_window`. `MainWindow` owns only the starter window configuration and a central welcome label. Standard-library `unittest` verifies the UI without adding a test dependency.

**Tech Stack:** Python 3, PySide6, unittest

**Spec:** `docs/superpowers/specs/2026-09-01-pyside6-template-design.md`

## Global Constraints

- Keep all GUI classes and future GUI modules beneath `ui/`.
- Do not modify the existing user-owned `test.py`.
- Declare PySide6 in `requirements.txt`.
- Include `.gitignore` for virtual environments, Python cache/build/test artifacts, IDE settings, and Qt temporary files.
- This directory is not yet a Git repository; do not run commit commands until the user initializes Git.

---

## File Structure

- `main.py`: constructs `QApplication`, shows `MainWindow`, and starts Qt's event loop.
- `ui/__init__.py`: makes `ui` importable.
- `ui/main_window.py`: defines the `MainWindow` class.
- `tests/test_main_window.py`: verifies the window's public starter configuration.
- `requirements.txt`: contains the runtime dependency.
- `.gitignore`: excludes generated local files from future Git commits.
- `README.md`: documents setup and launch commands.

### Task 1: Build and test the starter window

**Files:**
- Create: `ui/__init__.py`
- Create: `ui/main_window.py`
- Create: `tests/__init__.py`
- Create: `tests/test_main_window.py`

**Interfaces:**
- Produces: `ui.main_window.MainWindow`, a `PySide6.QtWidgets.QMainWindow` subclass with no constructor parameters.

- [ ] **Step 1: Write the failing UI test**

```python
import os
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication, QLabel
from ui.main_window import MainWindow


class MainWindowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.application = QApplication.instance() or QApplication([])

    def test_window_has_starter_configuration(self) -> None:
        window = MainWindow()

        self.assertEqual(window.windowTitle(), "PySide6 Application")
        self.assertGreaterEqual(window.minimumWidth(), 800)
        self.assertGreaterEqual(window.minimumHeight(), 600)
        self.assertIsInstance(window.centralWidget(), QLabel)
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `python -m unittest tests.test_main_window -v`

Expected: FAIL because `ui.main_window` does not yet exist.

- [ ] **Step 3: Implement the minimal UI package**

Create `ui/__init__.py` as an empty package marker. Create `ui/main_window.py` with:

```python
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QMainWindow


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("PySide6 Application")
        self.setMinimumSize(800, 600)

        welcome_label = QLabel("PySide6 application template")
        welcome_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setCentralWidget(welcome_label)
```

- [ ] **Step 4: Run the UI test to verify it passes**

Run: `python -m unittest tests.test_main_window -v`

Expected: PASS.

### Task 2: Add application startup and project metadata

**Files:**
- Create: `main.py`
- Create: `requirements.txt`
- Create: `.gitignore`
- Create: `README.md`

**Interfaces:**
- Consumes: `ui.main_window.MainWindow` from Task 1.
- Produces: `main.main() -> int`, the script entry point used by `python main.py`.

- [ ] **Step 1: Write the failing startup test**

Create `tests/test_main.py` with:

```python
import os
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import main


class MainTests(unittest.TestCase):
    def test_main_shows_window_and_returns_event_loop_result(self) -> None:
        with patch.object(main.QApplication, "exec", return_value=0) as exec_mock:
            self.assertEqual(main.main(), 0)

        exec_mock.assert_called_once()
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `python -m unittest tests.test_main -v`

Expected: FAIL because `main.py` does not yet exist.

- [ ] **Step 3: Implement startup and metadata**

Create `main.py` with:

```python
import sys

from PySide6.QtWidgets import QApplication

from ui.main_window import MainWindow


def main() -> int:
    application = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return application.exec()


if __name__ == "__main__":
    sys.exit(main())
```

Create `requirements.txt` containing `PySide6`. Create `.gitignore` excluding `.venv/`, `venv/`, `__pycache__/`, `*.py[cod]`, `.pytest_cache/`, `.coverage`, `htmlcov/`, `.idea/`, `.vscode/`, and `*.pro.user`. Create `README.md` that shows the commands `python -m venv .venv`, `.venv\\Scripts\\activate`, `pip install -r requirements.txt`, and `python main.py`.

- [ ] **Step 4: Run all tests to verify they pass**

Run: `python -m unittest discover -s tests -v`

Expected: PASS.

- [ ] **Step 5: Perform a syntax compilation check**

Run: `python -m compileall main.py ui tests`

Expected: all files compile successfully.

- [ ] **Step 6: Review files for Git readiness**

Run: `git status --short`

Expected: Git may report that this is not a repository. Once the user initializes Git, stage only `main.py`, `ui/`, `tests/`, `requirements.txt`, `.gitignore`, `README.md`, and `docs/`; never stage generated virtual-environment or cache files.
