import os, unittest
from unittest.mock import patch
os.environ.setdefault("QT_QPA_PLATFORM","offscreen")
from PySide6.QtWidgets import QApplication, QPushButton, QMessageBox, QScrollArea
from ui.main_window import MainWindow
from tests.device_fixture import ready

class DeviceControlsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=QApplication.instance() or QApplication([])
    def setUp(self):
        self.w=MainWindow();self.addCleanup(self.w.close)
        self.sent=[]
        self.w.command_client.is_connected=lambda:True
        self.w.command_client.socket.write=lambda payload:self.sent.append(bytes(payload)) or len(payload)

    def ready(self):
        s = ready(self.w, extended=True)
        self.sent.clear()
        return s

    def test_connect_waits_for_device_status_before_motion(self):
        w=self.w;w._on_connected()
        self.assertEqual(self.sent,[b"VERSION\r\n"])
        self.assertFalse(w.findChild(QPushButton,"motion_decap").isEnabled())
        self.assertTrue(w.findChild(QPushButton,"motion_stop").isEnabled())

    def test_error_status_updates_display_and_leaves_clear_stop_available(self):
        self.ready();w=self.w
        w._log_received(b"GSTATE 0,1,1,0,0,1,E2810,1,4\r\n")
        self.assertIn("E2810",w.device_detail_label.text())
        self.assertFalse(w.findChild(QPushButton,"motion_decap").isEnabled())
        self.assertTrue(w.findChild(QPushButton,"device_clear").isEnabled())

    def test_position_read_before_home_queries_selected_axis_without_capturing(self):
        self.ready();w=self.w;w.move_axis_combo.setCurrentIndex(1)
        w._log_received(b"GSTATE 0,0,0,0,0,1,E0000,0,0\r\n")
        w.rpos_read_button.click()
        self.assertEqual(self.sent,[b"GPOS R\r\n"])
        w._log_received(b"GPOS R,4321\r\n")
        self.assertEqual(w.current_z_position_value.text(),"4321")

    def test_selected_teaching_target_is_applied_to_ram(self):
        session=self.ready();w=self.w
        w.teach_target_combo.setCurrentIndex(1);w.teach_value_input.setText("1234")
        w.findChild(QPushButton,"teach_apply").click()
        self.assertEqual(self.sent,[b"TEACH 52,1234\r\n"])
        w._log_received(b"TEACH OK\r\n");w._log_received(b"PGET 52,1234\r\n")
        self.assertEqual(self.sent, [b"TEACH 52,1234\r\n", b"PGET 52\r\n"])
        self.assertEqual(session.verified_settings[52], (15000, 1234))

    def test_axis_teaching_buttons_capture_and_save_the_selected_axis(self):
        session = self.ready()
        window = self.w
        save_z = window.findChild(QPushButton, "teach_save_z")
        save_r = window.findChild(QPushButton, "teach_save_r")
        for removed in ("teach_rpos", "teach_capture", "motion_save"):
            self.assertIsNone(window.findChild(QPushButton, removed))
        self.assertEqual(save_z.text(), "SAVE Z")
        self.assertEqual(save_r.text(), "SAVE R")
        self.assertFalse(save_r.isEnabled())
        window.teach_target_combo.setCurrentIndex(window.teach_target_combo.findData(21))
        window.rpos_read_button.click()
        self.assertEqual(self.sent, [b"RPOS\r\n"])
        window._log_received(b"RPOS 12000,-3200\r\n")
        self.assertTrue(save_r.isEnabled())
        self.assertEqual(save_z.text(), "SAVE Z · 12000")
        self.assertEqual(save_r.text(), "SAVE R · -3200")
        self.assertIn("-3200", save_r.toolTip())
        self.assertEqual(window.teach_value_input.text(), "-3200")
        with patch("ui.device_controls.QMessageBox.question", return_value=QMessageBox.StandardButton.Yes):
            save_r.click()
        self.assertEqual(self.sent[-1], b"SAVE R\r\n")
        window._log_received(b"SAVE OK\r\nPGET 56,-3200\r\n")
        self.assertEqual(session.parameters[56], -3200)
        self.assertFalse(save_r.isEnabled())
        self.assertTrue(save_z.isEnabled())
        with patch("ui.device_controls.QMessageBox.question", return_value=QMessageBox.StandardButton.Yes):
            save_z.click()
        self.assertEqual(self.sent[-1], b"SAVE Z\r\n")
        window._log_received(b"SAVE OK\r\nPGET 51,12000\r\n")
        self.assertEqual(session.parameters[51], 12000)
        self.assertFalse(save_z.isEnabled())

    def test_transport_change_invalidates_captures_and_in_flight_capture(self):
        session = self.ready()
        window = self.w
        window.rpos_read_button.click()
        window._log_received(b"RPOS 12000,-3200\r\n")
        self.assertEqual(session.captured_targets["R"], -3200)
        window.rpos_read_button.click()
        window.transport_combo.setCurrentIndex(window.transport_combo.findData("UART"))
        window.transport_combo.setCurrentIndex(window.transport_combo.findData("TCP"))
        window._log_received(b"RPOS 13000,-3400\r\n")
        self.assertFalse(session.captured_targets)
        self.assertEqual(window.teach_value_input.text(), "12000")
        self.assertFalse(window.findChild(QPushButton, "teach_save_r").isEnabled())

    def test_move_outside_device_limit_is_blocked_before_write(self):
        s=self.ready();w=self.w
        s.positions["Z"]=150000;s.position_at["Z"]=s.clock()
        w.move_position_input.setText("20000")
        w.move_send_button.click()
        self.assertEqual(self.sent,[])
        self.assertIn("리미트",w.communication_log.toPlainText())

    def test_parameter_apply_sends_real_pl_and_queries_readback(self):
        self.ready();w=self.w
        w.parameter_combo.setCurrentIndex(w.parameter_combo.findData(14));w.parameter_value_input.setText("300000")
        w.findChild(QPushButton,"parameter_apply").click()
        self.assertEqual(self.sent,[b"PL 49,300000\r\n"])
        w._log_received(b"PL OK\r\n")
        self.assertEqual(self.sent[-1],b"PGET 49\r\n")

    def test_longrun_controls_are_in_their_own_tab(self):
        w = self.w
        self.assertEqual(w.motion_tabs.tabText(3), "Longrun")
        self.assertEqual(w.motion_tabs.widget(0).findChildren(QPushButton, "motion_lr"), [])
        self.assertIsNotNone(w.longrun_panel.findChild(QPushButton, "motion_lr"))

    def test_main_connecting_endpoint_is_reserved_against_longrun(self):
        from PySide6.QtNetwork import QAbstractSocket
        from tests.test_longrun_panel import FakeClient
        w = self.w
        w._single_tcp_endpoint = ("192.168.0.10", 8000)
        w.command_client.socket.state = lambda: QAbstractSocket.SocketState.ConnectingState
        w.longrun_panel.client_factory = FakeClient
        w.longrun_panel.set_ip(0, "192.168.0.10")
        w.longrun_panel.set_checked(0, True)
        w.longrun_panel.connect_selected()
        self.assertEqual(w.longrun_panel.modules, [])

    def test_longrun_header_stop_targets_all_batch_modules(self):
        from tests.test_longrun_panel import FakeClient
        w = self.w
        p = w.longrun_panel
        p.client_factory = FakeClient
        p.set_ip(0, "192.168.0.10")
        p.set_checked(0, True)
        p.add_module("192.168.0.11")
        p.connect_selected()
        p.set_checked(0, False)
        w.motion_tabs.setCurrentWidget(p)
        w.findChild(QPushButton, "motion_stop").click()
        self.assertTrue(all(m.client.commands[-1] == "STOP" for m in p.modules))
        self.assertFalse(w.command_send_button.isEnabled())

    def test_other_transport_cannot_replace_selected_device_state(self):
        self.ready();w=self.w;before=w.device_state_label.text()
        w._log_received(b"GSTATE 1,0,1,0,5,0,E2800,2,4\r\n","UART")
        self.assertEqual(w.device_state_label.text(),before)

    def test_motion_pages_have_no_scroll_and_command_area_is_smaller(self):
        w=self.w;w.resize(1100,760);w.show();self.app.processEvents()
        self.assertEqual(w.motion_tabs.findChildren(QScrollArea),[])
        self.assertNotIn("\n",w.device_state_label.text())
        self.assertLessEqual(w.device_state_label.font().pointSizeF(),11)
        top=w.findChild(type(w.centralWidget()),"input_area")
        bottom=w.findChild(type(w.centralWidget()),"command_area")
        self.assertGreater(top.height(),bottom.height())
        self.assertGreaterEqual(bottom.height(),w.findChild(type(w.centralWidget()),"right_splitter").height()*0.35)

    def test_teaching_parameter_editor_omits_hardware_and_duplicate_positions(self):
        w = self.w
        for offset in (*range(7), 16, 17, 18, 21):
            self.assertEqual(w.parameter_combo.findData(offset), -1)
        self.assertEqual([w.teach_target_combo.itemData(i) for i in range(w.teach_target_combo.count())],
                         [16, 17, 18, 21])

    def test_invalid_soft_limit_selection_is_blocked_before_transmission(self):
        self.ready()
        w = self.w
        w.parameter_combo.setCurrentIndex(w.parameter_combo.findData(13))
        w.parameter_value_input.setText("2")
        w.findChild(QPushButton, "parameter_apply").click()
        self.assertEqual(self.sent, [])

    def test_legacy_fast_motion_displays_explicit_stop_recovery(self):
        from tests.device_fixture import ready
        ready(self.w);w=self.w
        w.findChild(QPushButton,"motion_bgrip_1").click()
        w._log_received(b"BGRIP OK\r\nGSTA 0,1,1,0,E0000\r\n")
        self.assertIn("STOP",w.device_state_label.text())
        self.assertFalse(w.findChild(QPushButton,"motion_decap").isEnabled())
        w.findChild(QPushButton,"motion_stop").click()
        w._log_received(b"STOP stop\r\nGSTA 0,1,1,0,E0000\r\n")
        self.assertTrue(w.findChild(QPushButton,"motion_bgrip_1").isEnabled())

    def test_live_device_preview_uses_only_sensor_endpoints(self):
        self.ready()
        w = self.w
        w.live_model_check.setChecked(True)
        w._log_received(b"GSTA 0,1,1,0,E0000,1,1\r\nGSENSOR 1,1,0,0,1,0,0,1,0\r\n")
        self.assertIn("Forward", w.live_model_label.text())
        self.assertIn("보유", w.live_model_label.text())
        self.assertFalse(w.carriage_slider.isEnabled())
        w._log_received(b"GSENSOR 0,1,0,1,1,0,0,1,0\r\n")
        self.assertIn("양쪽 미감지", w.live_model_label.text())
        self.assertIn("Forward 마지막 감지", w.grip_status_label.text())
        self.assertEqual(self.sent, [])

    def test_status_cd_renders_request_values_and_recent_measured_positions(self):
        self.ready()
        w = self.w
        w._log_received(b"GPOS Z,42\r\n")
        w.findChild(QPushButton, "motion_cd").click()
        w._log_received(b"CD TARGET_Z=100;TARGET_R=200;SPEED=50;LR=1,10,0,0,1\r\n")
        table = w.diagnostic_table
        rows = {table.item(i, 0).text(): table.item(i, 1).text() for i in range(table.rowCount())}
        self.assertEqual(rows["Z 이동 요청값 (pulse)"], "100")
        self.assertEqual(rows["Z 최근 읽기 (pulse)"], "42")

    def test_teaching_and_relative_move_require_new_position_after_motion(self):
        s = self.ready()
        w = self.w
        w._log_received(b"GPOS Z,1000\r\n")
        self.assertTrue(s.request("MOVE A Z 159000"))
        w._log_received(b"MOVE OK\r\nGSTATE 1,1,0,0,11,0,E0000,1,1\r\n")
        w.rpos_read_button.click()
        w._log_received(b"GPOS Z,80000\r\n")  # A position read while moving.
        w._log_received(b"GSTATE 0,1,0,0,0,1,E0000,1,2\r\n")
        self.assertEqual(w.teach_value_input.text(), "")
        w.move_position_input.setText("2000")
        self.sent.clear()
        w._send_teaching_point()
        self.assertEqual(self.sent, [])
        w.rpos_read_button.click()
        self.assertEqual(self.sent, [b"RPOS\r\n"])
        w._log_received(b"RPOS 159000,0\r\n")
        self.assertEqual(w.teach_value_input.text(), "159000")
        self.sent.clear()
        w._send_teaching_point()
        self.assertEqual(self.sent, [])  # +2000 would now cross the limit.
        w.move_position_input.setText("500")
        w._send_teaching_point()
        self.assertEqual(self.sent, [b"MOVE R Z 500\r\n"])
