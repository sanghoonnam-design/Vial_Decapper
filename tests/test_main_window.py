import os
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication,
    QFrame,
    QGroupBox,
    QLabel,
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
        command_input = window.findChild(QLineEdit, "command_input")
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

    def test_command_area_has_compact_input_and_read_only_communication_log(self) -> None:
        window = MainWindow()
        window.resize(1000, 700)
        window.show()
        self.application.processEvents()

        right_splitter = window.findChild(QSplitter, "right_splitter")
        input_area = window.findChild(QGroupBox, "input_area")
        command_area = window.findChild(QGroupBox, "command_area")
        command_input = window.findChild(QLineEdit, "command_input")
        communication_log = window.findChild(QPlainTextEdit, "communication_log")

        self.assertIsNotNone(command_input)
        self.assertLessEqual(command_input.height(), 40)
        self.assertIsNotNone(communication_log)
        self.assertTrue(communication_log.isReadOnly())
        self.assertLessEqual(abs(input_area.height() - command_area.height()), 30)
        self.assertEqual(right_splitter.orientation(), Qt.Orientation.Vertical)

    def test_workspace_uses_equal_columns_with_labeled_boundaries(self) -> None:
        window = MainWindow()
        window.resize(1000, 700)
        window.show()
        self.application.processEvents()

        visual_area = window.findChild(QGroupBox, "visual_area")
        input_area = window.findChild(QGroupBox, "input_area")
        command_area = window.findChild(QGroupBox, "command_area")
        right_splitter = window.findChild(QSplitter, "right_splitter")

        self.assertIsNotNone(visual_area)
        self.assertIsNotNone(input_area)
        self.assertIsNotNone(command_area)
        self.assertEqual(visual_area.title(), "Vial Decapper Image")
        self.assertEqual(input_area.title(), "Teaching Point")
        self.assertEqual(command_area.title(), "Command")
        self.assertLessEqual(abs(visual_area.width() - right_splitter.width()), 30)

    def test_connection_bar_has_prominent_product_title(self) -> None:
        window = MainWindow()

        title_label = window.findChild(QLabel, "app_title")
        brand_mark = window.findChild(QLabel, "brand_mark")
        visual_pattern = window.findChild(QLabel, "visual_pattern")

        self.assertIsNotNone(title_label)
        self.assertEqual(title_label.text(), "Vial Decapper")
        self.assertGreaterEqual(title_label.font().pointSize(), 20)
        self.assertIsNotNone(brand_mark)
        self.assertEqual(brand_mark.text(), "◈")
        self.assertIsNotNone(visual_pattern)
        self.assertEqual(visual_pattern.text(), "◈")
