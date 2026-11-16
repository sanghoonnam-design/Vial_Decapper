"""Exercise the simulator and the actual control widgets through loopback TCP."""
import os
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import QElapsedTimer
from PySide6.QtNetwork import QHostAddress
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication, QLineEdit, QMessageBox, QPushButton

from simulator.device import FAULT_CASES, SimulatedDevice
from simulator.window import SimulatorWindow
from ui.device_faults import FAULTS, fault_summary
from ui.main_window import MainWindow


class SimulatorDeviceTests(unittest.TestCase):
    def test_detail_translates_firmware_values_and_preserves_unrecognized_text(self):
        summary = fault_summary("E2850", "HOME step=9 - Z home sensor not reached during slow search "
                                "(H=1 L=1 expected H=0)")
        self.assertIn("HOME · 단계 9 · Z HOME 저속 재탐색", summary)
        self.assertIn("입력 Z HOME=1, Z Low=1 / 기대 Z HOME=0", summary)
        self.assertIn("R축 목표 100001 pulse · 허용 -100000~100000 pulse",
                      fault_summary("E2810", "MOVE A R step=0 - R target 100001 outside [-100000,100000] pulse"))
        self.assertIn("R축 이동 한계 설정이 유효하지 않습니다.",
                      fault_summary("E2810", "MOVE A R step=0 - R limits invalid"))
        self.assertIn("센서가 감지 상태로 바뀌지 않았습니다.",
                      fault_summary("E2850", "Z home sensor not reached."))
        self.assertEqual(fault_summary("E9999", "phase=18 - New fault condition"),
                         "E9999 · 장비 오류\n동작 상태 18 · New fault condition")

    def test_cap_release_fault_in_longrun_reports_completed_cycles(self):
        now = [0.0]
        device = SimulatedDevice(lambda: now[0])
        device.command("LR")
        for tick in (1.0, 2.0, 3.0):
            now[0] = tick
            device.tick()
        device.inject_fault("E2870")
        self.assertEqual(device.command("GERD"),
                         "GERD LR/CAP step=9 completed=1 - Cap release not confirmed "
                         "(Open=0 Close=0 expected 1/0)")
        self.assertEqual(device.command("GLR"), "GLR 1,0,0,0,0")

    def test_fault_uses_firmware_detail_and_keeps_first_cause_until_clear(self):
        device = SimulatedDevice()
        device.inject_fault("E2851")
        detail = "HOME step=6 - Z home sensor did not release (H=0 L=1 expected H=1)"
        self.assertEqual(device.command("GERR"), "GERR E2851")
        self.assertEqual(device.command("GERD"), f"GERD {detail}")
        self.assertEqual(device.command("ERD"), f"ERD {detail}")
        device.inject_fault("E2880")
        self.assertEqual(device.command("GERD"), f"GERD {detail}")
        self.assertEqual(device.command("CLR"), "CLR")
        self.assertEqual(device.command("GERR"), "GERR E0000")
        self.assertEqual(device.command("GERD"), "GERD No Error.")

    def test_pause_freezes_longrun_and_fault_prevents_restart_until_cleared(self):
        now = [0.0]
        device = SimulatedDevice(lambda: now[0])
        self.assertEqual(device.command("LR 2"), "LR LR")
        self.assertEqual(device.command("PAUSE"), "PAUSE pause")
        now[0] = 3.0
        self.assertEqual(device.command("GLR"), "GLR 0,2,0,1,1")
        self.assertEqual(device.command("RESUME"), "RESUME resume")
        now[0] = 4.0
        device.tick()
        now[0] = 5.0
        self.assertEqual(device.command("GLR"), "GLR 1,2,0,0,1")
        device.inject_fault("E2800")
        self.assertEqual(device.command("GLR"), "GLR 1,2,0,0,0")
        self.assertEqual(device.command("LR 2"), "LR E2800")
        self.assertEqual(device.command("CLR"), "CLR")
        self.assertEqual(device.command("LR 2"), "LR LR")

    def test_axis_capture_is_saved_to_the_selected_parameter(self):
        now = [0.0]
        device = SimulatedDevice(lambda: now[0])
        self.assertEqual(device.command("MOVE A R -1200"), "MOVE OK")
        now[0] = 1.0
        self.assertEqual(device.command("RPOS"), "RPOS 0,-1200")
        self.assertEqual(device.command("SAVE R"), "SAVE OK")
        self.assertEqual(device.command("PGET 56"), "PGET 56,-1200")
        self.assertEqual(device.command("PGET 51"), "PGET 51,10000")
        self.assertEqual(device.command("SAVE R"), "SAVE NO_RPOS")
        self.assertEqual(device.command("LOAD"), "LOAD OK HOME_REQUIRED")
        self.assertEqual(device.command("PGET 56"), "PGET 56,-1200")
        self.assertEqual(device.command("PL 42,2000000"), "PL OK")
        self.assertEqual(device.command("PGET 42"), "PGET 42,2000000")


class SimulatorIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        self.sim = SimulatorWindow()
        self.window = MainWindow()
        self.addCleanup(self.sim.close)
        self.addCleanup(self.window.close)
        self.assertTrue(self.sim.server.listen(QHostAddress("127.0.0.1"), 0),
                        self.sim.server.errorString())
        self.port = self.sim.server.serverPort()

    def wait(self, predicate, label, timeout_ms=8000):
        elapsed = QElapsedTimer()
        elapsed.start()
        while not predicate() and elapsed.elapsed() < timeout_ms:
            QTest.qWait(20)
        self.assertTrue(predicate(), label + "\n" + "\n".join(self.window.communication_log.toPlainText().splitlines()[-25:]))

    def button(self, name):
        button = self.window.findChild(QPushButton, name)
        self.assertIsNotNone(button, name)
        self.assertTrue(button.isEnabled(), name)
        return button

    def test_control_app_homes_moves_and_displays_each_injected_fault(self):
        window = self.window
        window.findChild(QLineEdit, "ip_address_input").setText("127.0.0.1")
        window.port_input.setText(str(self.port))
        window.connect_button.click()
        session = window.sessions["TCP"]
        self.wait(lambda: session.can_move and len(session.parameters) == 22 and not session.pending, "handshake")
        self.assertIn("SIM 1.0", window.device_state_label.text())
        self.button("motion_home").click()
        self.wait(lambda: session.can_move and session.state.homed and not session.pending, "HOME")
        self.wait(lambda: self.button_enabled("motion_decap") and not session.pending, "CT ready")
        self.button("motion_decap").click()
        self.wait(lambda: session.can_move and self.sim.device.cap_held and not session.pending, "DECAP")
        self.wait(lambda: self.button_enabled("motion_cap") and not session.pending, "CAP ready")
        self.button("motion_cap").click()
        self.wait(lambda: session.can_move and not self.sim.device.cap_held and not session.pending, "CAP")
        window.motion_tabs.setCurrentIndex(1)
        self.assertTrue(window._send_motion_command("MOVE A R -1200"))
        self.wait(lambda: session.can_move and not session.pending, "R axis move")
        window.rpos_read_button.click()
        self.wait(lambda: session.captured_targets.get("R") == -1200 and not session.pending, "RPOS")
        with patch.object(QMessageBox, "question", return_value=QMessageBox.StandardButton.Yes):
            self.button("teach_save_r").click()
        self.wait(lambda: session.parameters.get(56) == -1200 and not session.pending, "SAVE R and PGET")
        self.assertEqual(self.sim.device.saved[21], -1200)
        window.motion_tabs.setCurrentIndex(0)
        codes = {"E2800", "E2810", "E2820", "E2850", "E2851", "E2860", "E2861", "E2870", "E2880"}
        self.assertEqual(set(FAULTS), codes)
        self.assertEqual(set(FAULT_CASES), codes)
        for code in sorted(codes):
            with self.subTest(code=code):
                self.sim.fault_choice.setCurrentIndex(self.sim.fault_choice.findData(code))
                self.sim.fault_button.click()
                self.wait(lambda: code in window.device_state_label.text() and not session.pending, code)
                self.assertEqual(window.device_state_label.property("tone"), "fault")
                self.assertFalse(self.button_enabled("motion_home"))
                self.wait(lambda: " step=" in session.error_detail and not session.pending, "fault details")
                self.assertIn(code, window.device_detail_label.text())
                self.assertIn("단계", window.device_detail_label.text())
                self.assertIn(session.error_detail, window.device_detail_label.toolTip())
                if code in {"E2850", "E2851", "E2860", "E2861", "E2870", "E2880"}:
                    self.assertIn("기대", window.device_detail_label.text())
                self.button("device_clear").click()
                self.wait(lambda: session.can_move and not session.pending
                          and self.sim.fault_button.isEnabled(), "CLR")
                self.assertEqual(self.sim.device.error, "E0000")
                self.assertTrue(window.device_detail_label.isHidden())
                self.assertTrue(self.button_enabled("motion_home"))

    def button_enabled(self, name):
        return self.window.findChild(QPushButton, name).isEnabled()

    def test_longrun_reaches_target_then_records_an_injected_fault(self):
        panel = self.window.longrun_panel
        self.window.motion_tabs.setCurrentWidget(panel)
        panel.add_module("127.0.0.1")
        panel.port_input.setValue(self.port)
        panel.count_input.setValue(2)
        panel.connect_selected()
        self.assertEqual(len(panel.modules), 1)
        session = panel.modules[0].session
        self.wait(lambda: panel.finite_button.isEnabled(), "Longrun ready")
        panel.finite_button.click()
        self.wait(lambda: len(session.longrun_history) == 1, "Longrun completed")
        self.assertEqual(session.longrun_history[-1].count, 2)
        self.assertEqual(session.longrun_history[-1].result, "목표 달성")
        self.wait(lambda: panel.unlimited_button.isEnabled(), "Longrun ready again")
        panel.unlimited_button.click()
        self.wait(lambda: session.lr_entered and session.longrun and session.longrun[0] > 0, "Longrun running")
        self.sim.fault_choice.setCurrentIndex(self.sim.fault_choice.findData("E2800"))
        self.sim.fault_button.click()
        self.wait(lambda: len(session.longrun_history) == 2, "Longrun fault recorded")
        self.assertEqual(session.longrun_history[-1].result, "오류")
        self.assertEqual(session.longrun_history[-1].error_code, "E2800")
        self.assertGreaterEqual(session.longrun_history[-1].count, 1)
        status_item = panel.table.item(len(panel.rows) - 1, 4)
        self.wait(lambda: "완료 " in status_item.toolTip(), "Longrun fault details")
        self.assertIn("동작 시간 초과", status_item.toolTip())
        self.assertIn("LR/", status_item.toolTip())
        self.assertFalse(panel.unlimited_button.isEnabled())
        self.sim.clear_button.click()
        self.wait(lambda: panel.unlimited_button.isEnabled(), "Longrun fault cleared")
