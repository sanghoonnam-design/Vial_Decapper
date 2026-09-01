import os
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication, QLabel, QLineEdit, QPushButton
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
        self.assertIsInstance(window.centralWidget(), QLabel)

    def test_connection_controls_accept_device_address(self) -> None:
        window = MainWindow()

        ip_address_input = window.findChild(QLineEdit, "ip_address_input")
        port_input = window.findChild(QLineEdit, "port_input")
        connect_button = window.findChild(QPushButton, "connect_button")

        self.assertIsNotNone(ip_address_input)
        self.assertIsNotNone(port_input)
        self.assertIsNotNone(connect_button)
        self.assertEqual(connect_button.text(), "Connect")
