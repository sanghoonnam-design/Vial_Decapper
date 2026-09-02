import os
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication,
    QFrame,
    QLineEdit,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QWidget,
)
from ui.main_window import MainWindow


class MainWindowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.application = QApplication.instance() or QApplication([])

    def test_window_has_starter_configuration(self) -> None:
        window = MainWindow()

        self.assertEqual(window.windowTitle(), "Vial Decapper")
        self.assertGreaterEqual(window.minimumWidth(), 800)
        self.assertGreaterEqual(window.minimumHeight(), 600)
        self.assertIsInstance(window.centralWidget(), QWidget)

    def test_connection_controls_accept_device_address(self) -> None:
        window = MainWindow()

        ip_address_input = window.findChild(QLineEdit, "ip_address_input")
        port_input = window.findChild(QLineEdit, "port_input")
        connect_button = window.findChild(QPushButton, "connect_button")

        self.assertIsNotNone(ip_address_input)
        self.assertIsNotNone(port_input)
        self.assertIsNotNone(connect_button)
        self.assertEqual(connect_button.text(), "Connect")

    def test_workspace_separates_visual_input_and_command_areas(self) -> None:
        window = MainWindow()

        separator = window.findChild(QFrame, "connection_separator")
        content_splitter = window.findChild(QSplitter, "content_splitter")
        visual_area = window.findChild(QWidget, "visual_area")
        right_splitter = window.findChild(QSplitter, "right_splitter")
        input_area = window.findChild(QWidget, "input_area")
        command_input = window.findChild(QPlainTextEdit, "command_input")
        command_send_button = window.findChild(QPushButton, "command_send_button")

        self.assertIsNotNone(separator)
        self.assertEqual(separator.frameShape(), QFrame.Shape.HLine)
        self.assertIsNotNone(content_splitter)
        self.assertEqual(content_splitter.orientation(), Qt.Orientation.Horizontal)
        self.assertIsNotNone(visual_area)
        self.assertIsNotNone(right_splitter)
        self.assertEqual(right_splitter.orientation(), Qt.Orientation.Vertical)
        self.assertIsNotNone(input_area)
        self.assertIsNotNone(command_input)
        self.assertIsNotNone(command_send_button)
        self.assertEqual(command_send_button.text(), "Send")
