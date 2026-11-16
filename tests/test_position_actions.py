import unittest
import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from ui.device_session import DeviceSession
from ui.position_actions import PositionActions


class PositionActionsTests(unittest.TestCase):
    def setUp(self):
        self.now = 0
        self.sent = []
        self.s = DeviceSession(lambda command: self.sent.append(command) or True, lambda: self.now)
        self.s.start()
        for line in ("VERSION 1.0", "GSTA 0,1,1,0,E0000", "SPEED 100",
                     "PCONFIG 1,35", "GSTATE 0,1,0,0,0,1,E0000,7,2"):
            self.s.receive(line)
        for offset, value in ((7, 160000), (8, 100000), (9, -160000), (10, 160000),
                              (11, -100000), (12, 100000), (13, 1)):
            self.s.receive(f"PGET {35+offset},{value}")
        self.sent.clear()
        self.actions = PositionActions(self.s)

    def receive(self, line):
        self.s.receive(line)
        self.actions.tick()

    def test_origin_moves_z_then_r_only_after_confirmed_completion(self):
        self.assertTrue(self.actions.origin())
        self.assertEqual(self.sent, ["MOVE A Z 0"])
        self.receive("MOVE OK")
        self.receive("GSTATE 1,1,0,0,11,0,E0000,8,1")
        self.assertNotIn("MOVE A R 0", self.sent)
        self.receive("GSTATE 0,1,0,0,0,1,E0000,8,2")
        self.assertEqual(self.sent[-1], "MOVE A R 0")
        self.receive("MOVE OK")
        self.receive("GSTATE 0,1,0,0,0,1,E0000,9,2")
        self.assertFalse(self.actions.active)
        self.assertNotIn("HOME", self.sent)
        self.assertNotIn("ORG", self.sent)

    def test_cap_moves_use_fresh_firmware_parameter_not_old_cached_value(self):
        for offset in (16, 17):
            with self.subTest(offset=offset):
                self.setUp()
                self.s.parameters[35+offset] = 999
                self.assertTrue(self.actions.cap(offset))
                self.assertEqual(self.sent, [f"PGET {35+offset}"])
                self.receive(f"PGET {35+offset},12345")
                self.assertEqual(self.sent[-1], "MOVE A Z 12345")

    def test_out_of_range_or_failed_query_cannot_move(self):
        self.actions.cap(16)
        self.receive("PGET 51,200000")
        self.assertFalse(self.actions.active)
        self.assertFalse(any(command.startswith("MOVE") for command in self.sent))
        self.actions.cap(17)
        self.receive("PGET FAIL")
        self.assertFalse(self.actions.active)
        self.assertFalse(any(command.startswith("MOVE") for command in self.sent))

    def test_error_stop_or_timeout_does_not_run_next_axis(self):
        for outcome in ("fault", "stop", "timeout"):
            with self.subTest(outcome=outcome):
                self.setUp()
                self.actions.origin()
                if outcome == "stop":
                    self.actions.cancel("STOP")
                    self.s.request("STOP")
                    self.receive("STOP stop")
                    self.receive("GSTATE 0,1,0,0,0,1,E0000,8,3")
                elif outcome == "fault":
                    self.receive("MOVE OK")
                    self.receive("GSTATE 0,1,1,0,0,1,E2810,8,4")
                else:
                    self.now = 3.1
                    self.s.tick()
                    self.actions.tick()
                self.assertFalse(self.actions.active)
                self.assertNotIn("MOVE A R 0", self.sent)

    def test_no_sequence_without_operation_identity(self):
        self.s.state.has_operation = False
        self.assertFalse(self.actions.origin())
        self.assertEqual(self.sent, [])


class PositionButtonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from PySide6.QtWidgets import QApplication
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        from ui.main_window import MainWindow
        from tests.device_fixture import ready
        self.w = MainWindow()
        self.w.session_timer.stop()
        self.addCleanup(self.w.close)
        self.sent = []
        self.w.command_client.socket.write = lambda data: self.sent.append(bytes(data)) or len(data)
        ready(self.w, extended=True)
        self.sent.clear()

    def test_cap_button_queries_exact_target_then_moves(self):
        from PySide6.QtWidgets import QPushButton
        w = self.w
        w.findChild(QPushButton, "axis_cap_side").click()
        self.assertEqual(self.sent, [b"PGET 52\r\n"])
        w._log_received(b"PGET 51,10000\r\n")
        self.assertEqual(self.sent, [b"PGET 52\r\n"])
        w._log_received(b"PGET 52,15000\r\n")
        self.assertEqual(self.sent[-1], b"MOVE A Z 15000\r\n")
        self.assertFalse(w.findChild(QPushButton, "axis_origin").isEnabled())

    def test_stop_cancels_origin_and_late_reply_never_starts_r(self):
        from PySide6.QtWidgets import QPushButton
        w = self.w
        w.findChild(QPushButton, "axis_origin").click()
        self.assertEqual(self.sent, [b"MOVE A Z 0\r\n"])
        w.findChild(QPushButton, "motion_stop").click()
        w._log_received(b"MOVE OK\r\nSTOP stop\r\nGSTATE 0,1,0,0,0,1,E0000,1,3\r\n")
        self.assertNotIn(b"MOVE A R 0\r\n", self.sent)
        self.assertFalse(w.position_actions["TCP"].active)

    def test_transport_switch_cancels_cap_before_parameter_reply(self):
        from PySide6.QtWidgets import QPushButton
        w = self.w
        w.findChild(QPushButton, "axis_cap_up").click()
        w.transport_combo.setCurrentIndex(w.transport_combo.findData("UART"))
        w._log_received(b"PGET 51,10000\r\n", "TCP")
        self.assertFalse(any(command.startswith(b"MOVE") for command in self.sent))

    def test_longrun_tab_cancels_followup_and_header_stop_reaches_single_device(self):
        from PySide6.QtWidgets import QPushButton
        w = self.w
        w.findChild(QPushButton, "axis_origin").click()
        w.motion_tabs.setCurrentWidget(w.longrun_panel)
        self.assertFalse(w.position_actions["TCP"].active)
        stop = w.findChild(QPushButton, "motion_stop")
        self.assertTrue(stop.isEnabled())
        stop.click()
        self.assertIn(b"STOP\r\n", self.sent)
        w._log_received(b"MOVE OK\r\nSTOP stop\r\nGSTATE 0,1,0,0,0,1,E0000,1,3\r\n")
        self.assertNotIn(b"MOVE A R 0\r\n", self.sent)
