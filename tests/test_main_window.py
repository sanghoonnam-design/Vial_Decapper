import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import Qt
from PySide6.QtNetwork import QAbstractSocket
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
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

    def test_connect_button_cancels_a_pending_tcp_connection(self) -> None:
        window = MainWindow()
        window.findChild(QLineEdit, "ip_address_input").setText("192.168.0.10")
        window.command_client.socket.state = lambda: QAbstractSocket.SocketState.ConnectingState
        window.command_client.disconnect_from_host = lambda: setattr(window, "_disconnect_called", True)

        window._toggle_connection()

        self.assertTrue(getattr(window, "_disconnect_called", False))

    def test_connection_bar_includes_uart_controls(self) -> None:
        window = MainWindow()

        com_input = window.findChild(QComboBox, "serial_port_combo")
        baud_input = window.findChild(QComboBox, "serial_baud_combo")
        uart_button = window.findChild(QPushButton, "uart_connect_button")

        self.assertIsNotNone(com_input)
        self.assertIsNotNone(baud_input)
        self.assertIn("9600", [baud_input.itemText(i) for i in range(baud_input.count())])
        self.assertEqual(baud_input.currentText(), "115200")
        self.assertIsNotNone(uart_button)
        self.assertEqual(uart_button.text(), "UART Connect")

    def test_send_is_disabled_until_tcp_connection_is_established(self) -> None:
        window = MainWindow()

        self.assertFalse(window.command_send_button.isEnabled())

        window._on_connected()
        self.assertTrue(window.command_send_button.isEnabled())

        window._on_disconnected()
        self.assertFalse(window.command_send_button.isEnabled())

    def test_connection_bar_defaults_to_cr_and_lf_terminators(self) -> None:
        window = MainWindow()

        cr_checkbox = window.findChild(QCheckBox, "cr_checkbox")
        lf_checkbox = window.findChild(QCheckBox, "lf_checkbox")

        self.assertIsNotNone(cr_checkbox)
        self.assertEqual(cr_checkbox.text(), "CR")
        self.assertTrue(cr_checkbox.isChecked())
        self.assertIsNotNone(lf_checkbox)
        self.assertEqual(lf_checkbox.text(), "LF")
        self.assertTrue(lf_checkbox.isChecked())

    def test_connection_bar_defaults_to_firmware_port(self) -> None:
        window = MainWindow()

        port_input = window.findChild(QLineEdit, "port_input")

        self.assertEqual(port_input.text(), "8000")

    def test_command_area_includes_save_button(self) -> None:
        window = MainWindow()

        save_button = window.findChild(QPushButton, "log_save_button")

        self.assertIsNotNone(save_button)
        self.assertEqual(save_button.text(), "Save")

    def test_command_area_includes_clear_button_after_save(self) -> None:
        window = MainWindow()

        save_button = window.findChild(QPushButton, "log_save_button")
        clear_button = window.findChild(QPushButton, "log_clear_button")

        self.assertIsNotNone(save_button)
        self.assertIsNotNone(clear_button)
        self.assertEqual(clear_button.text(), "Clear")

    def test_clear_button_removes_all_communication_log_text(self) -> None:
        window = MainWindow()
        window.communication_log.setPlainText("TX: MOVE R Z 100\\r\\n\nRX: OK")

        window._clear_communication_log()

        self.assertEqual(window.communication_log.toPlainText(), "")

    @patch("ui.main_window.QFileDialog.getSaveFileName")
    def test_save_writes_complete_communication_log(self, dialog_mock) -> None:
        window = MainWindow()
        log = window.findChild(QPlainTextEdit, "communication_log")
        log.setPlainText("TX: GSTA\\r\\n\nRX: GSTA,0")
        with tempfile.TemporaryDirectory() as temp_directory:
            output_path = Path(temp_directory) / "communication.txt"
            dialog_mock.return_value = (str(output_path), "Text files (*.txt)")

            window._save_communication_log()

            self.assertEqual(output_path.read_text(encoding="utf-8"), log.toPlainText())

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

    def test_teaching_point_controls_have_move_mode_axis_position_and_send(self) -> None:
        window = MainWindow()

        move_label = window.findChild(QLabel, "move_command_label")
        mode_combo = window.findChild(QComboBox, "move_mode_combo")
        axis_combo = window.findChild(QComboBox, "move_axis_combo")
        position_input = window.findChild(QLineEdit, "move_position_input")
        send_button = window.findChild(QPushButton, "move_send_button")

        self.assertIsNotNone(move_label)
        self.assertEqual(move_label.text(), "MOVE")
        self.assertIsNotNone(mode_combo)
        self.assertEqual([mode_combo.itemText(i) for i in range(mode_combo.count())], ["REL", "VEL"])
        self.assertIsNotNone(axis_combo)
        self.assertEqual([axis_combo.itemText(i) for i in range(axis_combo.count())], ["Z", "R"])
        self.assertIsNotNone(position_input)
        self.assertIsNotNone(send_button)
        self.assertEqual(send_button.text(), "Send")
        self.assertFalse(send_button.isEnabled())

    def test_teaching_point_send_builds_move_command(self) -> None:
        window = MainWindow()
        window._on_connected()
        window.findChild(QComboBox, "move_mode_combo").setCurrentText("VEL")
        window.findChild(QComboBox, "move_axis_combo").setCurrentText("R")
        window.findChild(QLineEdit, "move_position_input").setText("250")

        sent = []
        window.command_client.is_connected = lambda: True
        window.command_client.send_command = lambda command, append_cr, append_lf: sent.append(
            (command, append_cr, append_lf)
        )
        window._send_teaching_point()

        self.assertEqual(sent, [("MOVE A R 250", True, True)])

    def test_teaching_point_rejects_non_integer_position(self) -> None:
        window = MainWindow()
        window._on_connected()
        window.findChild(QLineEdit, "move_position_input").setText("12.5")

        sent = []
        window.command_client.send_command = lambda *args: sent.append(args)
        window._send_teaching_point()

        self.assertEqual(sent, [])
        self.assertIn("Position must be an integer.", window.communication_log.toPlainText())

    def test_teaching_point_includes_rpos_readback_controls(self) -> None:
        window = MainWindow()

        command_label = window.findChild(QLabel, "rpos_command_label")
        position_label = window.findChild(QLabel, "current_z_position_label")
        read_button = window.findChild(QPushButton, "rpos_read_button")

        self.assertIsNotNone(command_label)
        self.assertEqual(command_label.text(), "RPOS")
        self.assertIsNotNone(position_label)
        self.assertEqual(position_label.text(), "Current Z Position")
        self.assertIsNotNone(read_button)
        self.assertEqual(read_button.text(), "Read")
        self.assertFalse(read_button.isEnabled())

    def test_rpos_read_sends_command_and_updates_position_from_rx(self) -> None:
        window = MainWindow()
        window.command_client.is_connected = lambda: True
        sent = []
        window.command_client.send_command = lambda *args: sent.append(args)

        window._on_connected()
        window._send_rpos_command()
        window._log_received(b"RPOS 12345\r\n")

        self.assertEqual(sent, [("RPOS", True, True)])
        self.assertEqual(window.current_z_position_label.text(), "Current Z Position")
        self.assertEqual(window.current_z_position_value.text(), "12345")

    def test_received_log_removes_terminal_echo_and_ansi_sequences(self) -> None:
        window = MainWindow()

        payload = b"\x1b[2K\r     g\r\nRND> g\x1b[2K\r\nRND> [2K\r\nGSTA 0,0,0,0,E0000,\r\n"
        window._log_received(payload)

        log_text = window.communication_log.toPlainText()
        self.assertIn("RX: GSTA 0,0,0,0,E0000,", log_text)
        self.assertNotIn("RND>", log_text)
        self.assertNotIn("[2K", log_text)

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
