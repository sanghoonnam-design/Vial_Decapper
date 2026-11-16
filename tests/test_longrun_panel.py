import os
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QFontMetrics
from PySide6.QtNetwork import QAbstractSocket
from PySide6.QtWidgets import QApplication, QStyle
from ui.longrun_panel import LongrunPanel
from ui.tcp_command_client import TcpCommandClient


class FakeClient(QObject):
    connected = Signal()
    disconnected = Signal()
    received = Signal(bytes)
    sent = Signal(bytes)
    error = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.online = False
        self.commands = []
        self.endpoint = None

    def connect_to_host(self, host, port):
        self.endpoint = (host, port)
        self.online = True
        self.connected.emit()

    def is_connected(self):
        return self.online

    def disconnect_from_host(self):
        self.online = False
        self.disconnected.emit()

    def send_command(self, command, cr, lf):
        self.commands.append(command)
        self.sent.emit(command.encode())
        return command.encode()


class LongrunPanelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        self.logs = []
        self.panel = LongrunPanel(log=self.logs.append, client_factory=FakeClient)
        self.addCleanup(self.panel.close_connections)
        self.panel.set_ip(0, "192.168.0.10")

    def ready(self, module, extended=True):
        for reply in ("VERSION 1.0", "GSTA 0,1,1,0,E0000",
                      "SPEED 100", "PCONFIG 1,35" if extended else "PCONFIG UNSUPPORTED"):
            module.client.received.emit((reply + "\r\n").encode())
        if extended:
            module.client.received.emit(b"GSTATE 0,1,0,0,0,1,E0000,0,0\r\n")
        module.client.commands.clear()

    def test_disconnect_unlocks_pending_connection_and_allows_immediate_retry(self):
        class PendingClient(TcpCommandClient):
            def connect_to_host(self, host, port):
                self.socket.setSocketState(QAbstractSocket.SocketState.ConnectingState)

        panel = LongrunPanel(client_factory=PendingClient, initial_ips=("192.0.2.1",))
        self.addCleanup(panel.close_connections)
        panel.set_checked(0, True)
        panel.connect_selected()
        module = panel.modules[0]
        self.assertIsNotNone(module.connecting_at)
        panel.disconnect_selected()
        self.assertIsNone(module.connecting_at)
        self.assertFalse(panel.rows[0]["ip"].isReadOnly())
        self.assertTrue(panel.port_input.isEnabled())
        panel.connect_selected()
        self.assertIsNotNone(module.connecting_at)

    def test_capacity_and_location_are_shown_in_separate_columns(self):
        table = self.panel.table
        columns = {table.horizontalHeaderItem(i).text(): i for i in range(table.columnCount())}
        self.assertIn("용량", columns)
        self.assertIn("위치", columns)
        for suffix, capacity, location in (
            (170, "25mL", "Reagent Dispenser"), (171, "25mL", "Reagent Dispenser"),
            (172, "25mL", "Workup"), (173, "25mL", "Workup"), (174, "25mL", "Workup"),
            (175, "25mL", "Prep"), (176, "25mL", "Prep"),
            (177, "2mL", "Prep"), (178, "2mL", "Prep"),
            (179, "—", "—"),
        ):
            with self.subTest(suffix=suffix):
                self.panel.set_ip(0, f"192.168.0.{suffix}")
                self.assertEqual(table.item(0, columns["용량"]).text(), capacity)
                self.assertEqual(table.item(0, columns["위치"]).text(), location)

    def test_extra_table_width_expands_progress_after_full_location_name_fits(self):
        panel = self.panel
        panel.set_ip(0, "192.168.0.170")
        panel.resize(650, 400)
        panel.show()
        self.addCleanup(panel.close)
        self.app.processEvents()
        table = panel.table
        item = table.item(0, 3)
        focus_margin = table.style().pixelMetric(QStyle.PixelMetric.PM_FocusFrameHMargin, None, table)
        minimum_width = QFontMetrics(item.font()).horizontalAdvance(item.text()) + 2 * (focus_margin + 1) + 2
        location_width = table.columnWidth(3)
        self.assertGreaterEqual(location_width, minimum_width)
        self.assertLessEqual(location_width, minimum_width + 2)
        original_progress_width = table.columnWidth(5)
        panel.resize(900, 400)
        self.app.processEvents()
        self.assertEqual(table.columnWidth(3), location_width)
        self.assertGreater(table.columnWidth(5), original_progress_width)

    def test_checked_modules_get_independent_connections_and_commands(self):
        p = self.panel
        p.add_module("192.168.0.11")
        p.add_module("192.168.0.12")
        p.set_checked(2, False)
        p.connect_selected()
        self.assertEqual(len(p.modules), 2)
        for module in p.modules:
            self.ready(module)
        self.assertTrue(p.start_selected(3))
        for module in p.modules:
            self.assertEqual(module.client.commands, ["LR 3"])
        self.assertIsNot(p.modules[0].session, p.modules[1].session)

    def test_duplicate_or_single_device_endpoint_is_rejected(self):
        p = self.panel
        p.add_module("192.168.0.10")
        p.connect_selected()
        self.assertEqual(p.modules, [])
        p.set_ip(1, "192.168.0.11")
        p.single_endpoint = lambda: ("192.168.0.10", 8000)
        p.connect_selected()
        self.assertEqual(p.modules, [])

    def test_unready_selected_module_blocks_whole_start(self):
        p = self.panel
        p.add_module("192.168.0.11")
        p.connect_selected()
        self.ready(p.modules[0])
        self.assertFalse(p.start_selected(2))
        self.assertEqual(p.modules[0].client.commands, [])

    def test_all_stop_includes_unchecked_running_module_and_preempts_reply(self):
        p = self.panel
        p.connect_selected()
        module = p.modules[0]
        self.ready(module)
        self.assertTrue(p.start_selected(0))
        p.set_checked(0, False)
        p.stop_all()
        self.assertEqual(module.client.commands, ["LR", "STOP"])

    def test_one_module_disconnect_does_not_stop_the_other(self):
        p = self.panel
        p.add_module("192.168.0.11")
        p.connect_selected()
        for module in p.modules:
            self.ready(module)
        p.start_selected(0)
        p.modules[0].client.disconnect_from_host()
        self.assertTrue(p.modules[0].session.disconnected_during_motion)
        self.assertTrue(p.modules[1].session.track_longrun)
        self.assertNotIn("STOP", p.modules[1].client.commands)

    def test_legacy_unlimited_and_extended_progress(self):
        p = self.panel
        p.connect_selected()
        module = p.modules[0]
        self.ready(module, extended=False)
        self.assertFalse(p.start_selected(3))
        self.assertTrue(p.start_selected(0))
        self.assertEqual(module.client.commands, ["LR"])
        self.assertFalse(p.start_selected(0))

    def test_busy_blocks_start_and_progress_is_per_module(self):
        p = self.panel
        p.connect_selected()
        module = p.modules[0]
        self.ready(module)
        self.assertTrue(p.start_selected(3))
        module.client.received.emit(b"LR LR\r\nGSTATE 1,1,0,0,16,0,E0000,1,1\r\nGLR 2,3,0,0,1\r\n")
        self.assertIn("2/3", module.progress())
        self.assertEqual(p.table.horizontalHeaderItem(5).text(), "진행")
        progress_item = p.table.item(0, 5)
        self.assertEqual(progress_item.text(), "2")
        self.assertEqual(progress_item.toolTip(), module.progress())
        self.assertFalse(p.start_selected(3))
        self.assertEqual(module.client.commands.count("LR 3"), 1)

    def test_failed_write_is_reported_without_resending_other_modules(self):
        p = self.panel
        p.add_module("192.168.0.11")
        p.connect_selected()
        for module in p.modules:
            self.ready(module)
        p.modules[0].client.send_command = lambda *args: b""
        self.assertFalse(p.start_selected(3))
        self.assertTrue(p.modules[0].session.uncertain)
        self.assertEqual(p.modules[1].client.commands, ["LR 3"])

    def test_overflow_and_fragmented_replies_stay_with_their_module(self):
        p = self.panel
        p.add_module("192.168.0.11")
        p.connect_selected()
        a, b = p.modules
        a.received(b"x" * 65537 + b"\nVERSION 1.")
        self.assertEqual(a.session.version, "")
        a.received(b"2\r\n")
        self.assertEqual(a.session.version, "1.2")
        self.assertEqual(b.session.version, "")
