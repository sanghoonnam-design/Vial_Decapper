import unittest

from ui.device_session import DeviceSession


class OperationDashboardTests(unittest.TestCase):
    def setUp(self):
        self.now = 0.0
        self.sent = []
        self.s = DeviceSession(lambda cmd: self.sent.append(cmd) or True, lambda: self.now)
        self.s.start()
        for reply in ("VERSION 1.0", "GSTA 0,1,1,0,E0000,1,1",
                      "SPEED 100", "PCONFIG 1,35", "GSTATE 0,1,0,0,0,1,E0000,5,2"):
            self.s.receive(reply)
        self.sent.clear()

    def test_grip_and_y_are_preserved_separately_from_motion_state(self):
        s = self.s
        s.receive("GSENSOR 1,1,0,0,1,0,0,1,0")
        s.receive("GSTA 0,1,1,0,E0000,1,1")
        self.assertEqual(s.fresh_grip, (True, True))
        self.assertEqual(s.y_position, "Forward")
        self.assertEqual(s.state.operation, 5)
        self.now = 4.1
        self.assertIsNone(s.fresh_grip)
        self.assertIsNone(s.y_position)

    def test_automatic_command_rejects_known_missing_ct(self):
        self.s.receive("GSENSOR 0,1,0,0,1,1,0,1,0")
        self.assertFalse(self.s.request("DECAP"))
        self.assertEqual(self.sent, [])

    def test_lost_speed_does_not_prevent_capability_discovery(self):
        s = self.s
        s.start()
        s.receive("VERSION 1.0")
        s.receive("GSTA 0,1,1,0,E0000")
        self.assertEqual(s.pending[0], "SPEED")
        self.now = 3.1
        s.tick()
        s.tick()
        self.assertEqual(s.pending[0], "PCONFIG")
        s.receive("PCONFIG 1,35")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,5,2")
        self.assertTrue(s.can_move)

    def test_stop_is_pending_until_matching_idle_state(self):
        s = self.s
        self.assertTrue(s.request("STOP"))
        s.receive("STOP stop")
        s.receive("GSTATE 0,1,0,0,0,0,E0000,5,3")
        self.assertTrue(s.stop_wait)
        s.receive("GSTATE 0,1,0,0,0,1,E0000,5,3")
        self.assertFalse(s.stop_wait)
        self.assertIn("정지 확인", s.operation_text)

    def test_late_idle_state_cannot_confirm_unacknowledged_stop(self):
        s = self.s
        s.request("STOP")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,5,2")
        self.assertTrue(s.stop_wait)
        self.assertFalse(s.can_move)

    def test_older_operation_cannot_confirm_stop_after_ack(self):
        s = self.s
        s.receive("GSTATE 1,1,0,0,16,0,E0000,6,1")
        s.request("STOP")
        s.receive("STOP stop")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,5,2")
        self.assertTrue(s.stop_wait)
        self.assertTrue(s.state.busy)
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,3")
        self.assertFalse(s.stop_wait)

    def test_new_idle_operation_invalidates_prior_ct_preflight(self):
        s = self.s
        s.receive("GSENSOR 1,1,0,0,1,1,0,1,0")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,2")
        self.assertFalse(s.request("CAP"))
        self.assertNotIn("CAP", self.sent)

    def test_pause_needs_supported_operation_and_firmware_state_change(self):
        s = self.s
        s.receive("GSTATE 1,1,0,0,11,0,E0000,6,1")
        self.assertFalse(s.request("PAUSE"))
        s.receive("GSTATE 1,1,0,0,4,0,E0000,7,1")
        self.assertTrue(s.request("PAUSE"))
        s.receive("PAUSE pause")
        s.receive("GSTATE 1,1,0,0,4,0,E0000,7,1")
        self.assertNotEqual(s.operation_text, "일시정지 확인")
        s.receive("GSTATE 1,1,0,1,4,0,E0000,7,1")
        self.assertEqual(s.operation_text, "일시정지 확인")
        self.assertTrue(s.can_pause(resume=True))

    def test_disconnect_during_longrun_cannot_be_logged_as_completion(self):
        s = self.s
        s.request("LR")
        s.receive("LR LR")
        s.receive("GSTATE 1,1,0,0,16,0,E0000,6,1")
        s.receive("GLR 7,0,0,0,1")
        self.now = 8
        s.stop()
        self.assertTrue(s.disconnected_during_motion)
        record = s.longrun_history[-1]
        self.assertIn("결과 미확인", record.result)
        self.assertIsNone(record.count)
        self.assertEqual(record.elapsed, 8)
        s.start()
        self.assertTrue(s.disconnected_during_motion)

    def test_longrun_records_final_count_after_idle(self):
        s = self.s
        s.request("LR 2")
        s.receive("LR LR")
        s.receive("GSTATE 1,1,0,0,16,0,E0000,6,1")
        s.receive("GLR 1,2,1,0,1")
        self.now = 12
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,2")
        s.tick()
        s.receive("GLR 2,2,1,0,0")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,2")
        record = s.longrun_history[-1]
        self.assertEqual((record.count, record.elapsed, record.result), (2, 12, "목표 달성"))

    def test_stop_before_longrun_enters_fsm_cannot_reuse_previous_count(self):
        s = self.s
        s.request("LR 2")
        s.receive("LR LR")
        s.request("STOP")
        s.receive("STOP stop")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,3")
        s.receive("GLR 2,2,1,0,0")
        record = s.longrun_history[-1]
        self.assertIsNone(record.count)
        self.assertIn("STOP", record.result)

    def test_operation_id_without_command_ack_is_not_confirmation(self):
        s = self.s
        s.request("HOME")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,2")
        self.assertTrue(s.motion_wait)
        self.assertNotIn("완료", s.operation_text)

    def test_status_targets_never_replace_measured_positions(self):
        s = self.s
        s.request("CD")
        s.receive("CD TARGET_Z=100;TARGET_R=200;SPEED=50;LR=1,10,0,0,1")
        self.assertEqual(s.diagnostics["CD"]["TARGET_Z"], 100)
        self.assertEqual(s.positions, {})
        self.assertIsNone(s.longrun)

    def test_settings_require_readback_and_save_ok(self):
        s = self.s
        s.receive("PGET 51,100")
        s.request("TEACH 51,200")
        s.receive("TEACH OK")
        self.assertNotIn(51, s.verified_settings)
        s.receive("PGET 51,200")
        self.assertEqual(s.verified_settings[51], (100, 200))
        s.request("SAVE 1")
        s.receive("SAVE FAIL")
        self.assertIn(51, s.verified_settings)
        s.request("SAVE 1")
        s.receive("SAVE OK")
        self.assertNotIn(51, s.verified_settings)
        self.assertIn("저장 확인", s.setting_text)

    def test_readback_mismatch_blocks_eeprom_save(self):
        s = self.s
        s.request("PL 51,200")
        s.receive("PL OK")
        s.receive("PGET 51,199")
        self.assertIn("불일치", s.setting_text)
        self.assertFalse(s.request("SAVE 1"))
        self.assertNotIn("SAVE 1", self.sent)

    def test_repeated_mismatch_stays_blocked_until_requested_value_is_confirmed(self):
        s = self.s
        s.request("PL 51,200")
        s.receive("PL OK")
        s.receive("PGET 51,199")
        self.assertTrue(s.request("PGET 51"))
        s.receive("PGET 51,199")
        self.assertFalse(s.request("SAVE 1"))
        self.assertTrue(s.request("PGET 51"))
        s.receive("PGET 51,200")
        self.assertTrue(s.request("SAVE 1"))

    def test_non_ascii_longrun_count_does_not_change_motion_state(self):
        for command in ("LR ²", "LR １２", "CAPDECAPLR ²"):
            with self.subTest(command=command):
                self.assertFalse(self.s.request(command))
                self.assertTrue(self.s.can_move)
                self.assertEqual(self.sent, [])

    def test_confirming_another_setting_does_not_clear_unresolved_write(self):
        s = self.s
        s.request("PL 51,200")
        s.receive("PL OK")
        s.receive("PGET 51,199")
        s.request("PL 52,300")
        s.receive("PL OK")
        s.receive("PGET 52,300")
        self.assertFalse(s.request("SAVE 1"))
        self.assertTrue(s.request("PL 51,199"))
        s.receive("PL OK")
        s.receive("PGET 51,199")
        self.assertTrue(s.request("SAVE 1"))

    def test_maintenance_and_diagnostics_do_not_prove_motion_completion(self):
        s = self.s
        s.request("HOME")
        s.receive("HOME home")
        s.receive("SL STATE=0,1,0,0,0,1,E0000,6,2;SENSOR=1,1,0,0,1,0,0,1,0")
        self.assertFalse(s.can_move)
        s.receive("GSTATE 0,1,0,0,0,1,E0000,6,2")
        self.assertTrue(s.can_move)
