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

    def test_position_read_is_pure_gpos_and_uses_selected_axis(self):
        self.ready();w=self.w;w.move_axis_combo.setCurrentIndex(1)
        w.rpos_read_button.click()
        self.assertEqual(self.sent,[b"GPOS R\r\n"])
        w._log_received(b"GPOS R,4321\r\n")
        self.assertEqual(w.current_z_position_value.text(),"4321")

    def test_selected_teaching_target_is_explicit_and_save_is_confirmed(self):
        self.ready();w=self.w
        w.teach_target_combo.setCurrentIndex(1);w.teach_value_input.setText("1234")
        w.findChild(QPushButton,"teach_apply").click()
        self.assertEqual(self.sent,[b"TEACH 52,1234\r\n"])
        w._log_received(b"TEACH OK\r\n");w._log_received(b"PGET 52,1234\r\n")
        with patch("ui.main_window.QMessageBox.question",return_value=QMessageBox.StandardButton.Yes):
            w.findChild(QPushButton,"motion_save").click()
        self.assertEqual(self.sent[-1],b"SAVE 1\r\n")

    def test_move_outside_device_limit_is_blocked_before_write(self):
        s=self.ready();w=self.w
        s.positions["Z"]=150000;s.position_at["Z"]=s.clock()
        w.move_position_input.setText("20000")
        w.move_send_button.click()
        self.assertEqual(self.sent,[])
        self.assertIn("리미트",w.communication_log.toPlainText())

    def test_parameter_apply_sends_real_pl_and_queries_readback(self):
        self.ready();w=self.w
        w.parameter_combo.setCurrentIndex(14);w.parameter_value_input.setText("300000")
        w.findChild(QPushButton,"parameter_apply").click()
        self.assertEqual(self.sent,[b"PL 49,300000\r\n"])
        w._log_received(b"PL OK\r\n")
        self.assertEqual(self.sent[-1],b"PGET 49\r\n")

    def test_finite_lr_start_and_progress_match_firmware_count(self):
        self.ready();w=self.w;w.longrun_count_input.setValue(3)
        w.findChild(QPushButton,"motion_lr").click()
        self.assertEqual(self.sent,[b"LR 3\r\n"])
        w._log_received(b"LR LR\r\nGSTATE 1,1,0,0,16,0,E0000,1,1\r\nGLR 2,3,0,0,1\r\n")
        self.assertIn("2 / 3",w.longrun_status_label.text())

    def test_debug_unlimited_button_sends_plain_lr_to_legacy_firmware(self):
        from tests.device_fixture import ready
        ready(self.w);w=self.w;self.sent.clear()
        button=w.findChild(QPushButton,"motion_lr_unlimited")
        self.assertIsNotNone(button)
        self.assertTrue(button.isEnabled())
        self.assertFalse(w.findChild(QPushButton,"motion_lr").isEnabled())
        w.longrun_count_input.setValue(123)
        button.click()
        self.assertEqual(self.sent,[b"LR\r\n"])
        button.click()
        self.assertEqual(self.sent,[b"LR\r\n"])
        self.assertIn("무한",w.longrun_status_label.text())

    def test_debug_unlimited_button_remains_blocked_while_busy(self):
        self.ready();w=self.w
        w._log_received(b"GSTATE 1,1,0,0,5,0,E0000,1,1\r\n")
        button=w.findChild(QPushButton,"motion_lr_unlimited")
        self.assertIsNotNone(button)
        self.assertFalse(button.isEnabled())
        button.click()
        self.assertEqual(self.sent,[])

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

    def test_invalid_hardware_enum_is_blocked_before_transmission(self):
        self.ready();w=self.w
        for offset,value in ((6,"3200"),(2,"2"),(3,"-1")):
            with self.subTest(offset=offset):
                w.parameter_combo.setCurrentIndex(offset)
                w.parameter_value_input.setText(value)
                w.findChild(QPushButton,"parameter_apply").click()
                self.assertEqual(self.sent,[])

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

    def test_status_cd_renders_targets_and_recent_measured_positions(self):
        self.ready()
        w = self.w
        w._log_received(b"GPOS Z,42\r\n")
        w.findChild(QPushButton, "motion_cd").click()
        w._log_received(b"CD TARGET_Z=100;TARGET_R=200;SPEED=50;LR=1,10,0,0,1\r\n")
        table = w.diagnostic_table
        rows = {table.item(i, 0).text(): table.item(i, 1).text() for i in range(table.rowCount())}
        self.assertEqual(rows["Z 목표 (pulse)"], "100")
        self.assertEqual(rows["Z 최근 읽기 (pulse)"], "42")
