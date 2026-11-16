import sys

from PySide6.QtWidgets import QApplication

from .window import SimulatorWindow


def main():
    app = QApplication(sys.argv)
    window = SimulatorWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
