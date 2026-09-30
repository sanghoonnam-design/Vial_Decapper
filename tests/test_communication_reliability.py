import os
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication, QComboBox, QPushButton
from PySide6.QtSerialPort import QSerialPort
from PySide6.QtCore import QElapsedTimer
from PySide6.QtNetwork import QHostAddress, QTcpServer
from PySide6.QtTest import QTest
from ui.main_window import MainWindow


class CommunicationReliabilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        self.window = MainWindow()
        self.addCleanup(self.window.close)

    def test_split_position_is_not_published_until_complete(self):
        w = self.window
        w.command_client.received.emit(b"RPO")
        w.command_client.received.emit(b"S 12")
        self.assertEqual(w.current_z_position_value.text(), "--")
        w.command_client.received.emit(b"345\r")
        self.assertEqual(w.current_z_position_value.text(), "--")
        w.command_client.received.emit(b"\n")
        self.assertEqual(w.current_z_position_value.text(), "12345")

    def test_all_coalesced_responses_are_processed_in_order(self):
        w = self.window
        w.command_client.received.emit(b"RPOS 100\r\nRPOS -200\r\n")
        self.assertEqual(w.current_z_position_value.text(), "-200")

    def test_split_utf8_and_ansi_are_cleaned_after_line_assembly(self):
        w = self.window
        data = "\x1b[2K\r장비 응답\r\n".encode("utf-8")
        for byte in data:
            w.command_client.received.emit(bytes([byte]))
        self.assertIn("[TCP] RX: 장비 응답", w.communication_log.toPlainText())
        self.assertNotIn("\ufffd", w.communication_log.toPlainText())

    def test_uart_buttons_use_selected_terminators_at_write_boundary(self):
        w = self.window
        written = []
        w.serial_client.is_connected = lambda: True
        w.serial_client.serial.write = lambda data: written.append(bytes(data)) or len(data)
        w.transport_combo.setCurrentIndex(w.transport_combo.findData("UART"))
        for cr, lf, expected in (
            (False, False, b"STOP"), (True, False, b"STOP\r"),
            (False, True, b"STOP\n"), (True, True, b"STOP\r\n"),
        ):
            w.cr_checkbox.setChecked(cr)
            w.lf_checkbox.setChecked(lf)
            w.findChild(QPushButton, "motion_stop").click()
            self.assertEqual(written[-1], expected)
        self.assertIn("[UART] TX:", w.communication_log.toPlainText())

    def test_tcp_loopback_button_send_and_split_reply(self):
        # Local test peer only: never connect to a physical device.
        server = QTcpServer()
        self.addCleanup(server.close)
        self.assertTrue(server.listen(QHostAddress.SpecialAddress.LocalHost, 0))
        w = self.window
        self.addCleanup(w.command_client.disconnect_from_host)

        def wait_until(predicate):
            timer = QElapsedTimer()
            timer.start()
            while not predicate() and timer.elapsed() < 2000:
                self.app.processEvents()
                QTest.qWait(5)
            self.assertTrue(predicate(), "Loopback operation timed out")

        w.command_client.connect_to_host("127.0.0.1", server.serverPort())
        wait_until(lambda: server.hasPendingConnections() and w.command_client.is_connected())
        peer = server.nextPendingConnection()
        self.addCleanup(peer.abort)
        w.cr_checkbox.setChecked(True)
        w.lf_checkbox.setChecked(True)
        w.rpos_read_button.click()
        wait_until(lambda: peer.bytesAvailable() >= 6)
        self.assertEqual(bytes(peer.readAll()), b"RPOS\r\n")
        peer.write(b"RPOS 12")
        peer.flush()
        wait_until(lambda: bool(w._receive_buffers["TCP"]))
        self.assertEqual(w.current_z_position_value.text(), "--")
        peer.write(b"345\r\nRPOS -67\r\n")
        peer.flush()
        wait_until(lambda: w.current_z_position_value.text() == "-67")
        self.assertIn("[TCP] RX: RPOS 12345", w.communication_log.toPlainText())

    def test_streams_do_not_mix_or_overwrite_selected_position(self):
        w = self.window
        w.command_client.received.emit(b"RPOS 12")
        w.serial_client.received.emit(b"RPOS 900\r\n")
        self.assertEqual(w.current_z_position_value.text(), "--")
        w.command_client.received.emit(b"345\r\n")
        self.assertEqual(w.current_z_position_value.text(), "12345")
        log = w.communication_log.toPlainText()
        self.assertIn("[UART] RX: RPOS 900", log)
        self.assertIn("[TCP] RX: RPOS 12345", log)

    def test_disconnect_discards_incomplete_response_and_stale_position(self):
        w = self.window
        w.command_client.received.emit(b"RPOS 10\r\nRPOS 12")
        w.command_client.disconnected.emit()
        self.assertEqual(w.current_z_position_value.text(), "--")
        w.command_client.received.emit(b"345\r\n")
        self.assertEqual(w.current_z_position_value.text(), "--")

    def test_oversized_line_is_discarded_then_next_response_recovers(self):
        w = self.window
        w.command_client.received.emit(b"x" * 70000)
        w.command_client.received.emit(b"RPOS 42\r\nRPOS 99\r\n")
        self.assertEqual(w.current_z_position_value.text(), "99")
        self.assertIn("ERROR:", w.communication_log.toPlainText())

    def test_failed_or_partial_write_is_not_reported_as_sent(self):
        w = self.window
        for client, device in ((w.command_client, w.command_client.socket),
                               (w.serial_client, w.serial_client.serial)):
            client.is_connected = lambda: True
            for count in (-1, 0, 2):
                with self.subTest(client=type(client).__name__, count=count):
                    device.write = lambda payload, count=count: count
                    w.communication_log.clear()
                    self.assertEqual(client.send_command("HOME", False, False), b"")
                    log = w.communication_log.toPlainText()
                    self.assertNotIn("TX:", log)
                    self.assertIn("ERROR:", log)

    def test_uart_no_error_is_ignored(self):
        self.window.serial_client._on_error(QSerialPort.SerialPortError.NoError)
        self.assertEqual(self.window.communication_log.toPlainText(), "")

    def test_uart_resource_error_closes_port_and_notifies_ui(self):
        client = self.window.serial_client
        events = []
        opened = [True]
        client.serial.isOpen = lambda: opened[0]
        client.serial.close = lambda: opened.__setitem__(0, False)
        client.disconnected.connect(lambda: events.append("disconnected"))
        client._on_error(QSerialPort.SerialPortError.ResourceError)
        self.assertFalse(client.is_connected())
        self.assertEqual(events, ["disconnected"])

    def test_selected_transport_never_silently_falls_back(self):
        w = self.window
        selector = w.findChild(QComboBox, "transport_combo")
        self.assertIsNotNone(selector)
        tcp, uart = [], []
        w.command_client.is_connected = lambda: True
        w.serial_client.is_connected = lambda: True
        w.command_client.socket.write = lambda data: tcp.append(bytes(data)) or len(data)
        w.serial_client.serial.write = lambda data: uart.append(bytes(data)) or len(data)
        w._on_connected()
        w._on_uart_connected()
        selector.setCurrentIndex(selector.findData("UART"))
        w.findChild(QPushButton, "motion_stop").click()
        self.assertEqual(uart, [b"STOP\r\n"])
        self.assertEqual(tcp, [])
        w.serial_client.is_connected = lambda: False
        w.serial_client.disconnected.emit()
        self.assertFalse(w.command_send_button.isEnabled())
        w._send_motion_command("HOME")
        self.assertEqual(tcp, [])
        selector.setCurrentIndex(selector.findData("TCP"))
        w.findChild(QPushButton, "motion_stop").click()
        self.assertEqual(tcp, [b"STOP\r\n"])

    def test_log_save_error_is_reported_without_losing_log(self):
        w = self.window
        w._append_log("keep this log")
        with patch("ui.main_window.QFileDialog.getSaveFileName", return_value=("blocked.txt", "")), \
             patch("ui.main_window.Path.write_text", side_effect=PermissionError("denied")):
            w._save_communication_log()
        log = w.communication_log.toPlainText()
        self.assertIn("keep this log", log)
        self.assertIn("ERROR:", log)

    def test_log_keeps_only_recent_entries(self):
        w = self.window
        for index in range(10010):
            w._append_log(f"entry {index}")
        self.assertLessEqual(w.communication_log.document().blockCount(), 10000)
        self.assertTrue(w.communication_log.toPlainText().endswith("entry 10009"))
