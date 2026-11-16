import unittest
from tests.device_fixture import parameter_snapshot
try:
    from ui.device_session import DeviceSession
except ImportError:
    DeviceSession = None

class DeviceSessionTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(DeviceSession, "Device session implementation is missing")
        self.clock=0.0
        self.sent=[];self.errors=[]
        self.session=DeviceSession(lambda text:self.sent.append(text) or True,
                                   lambda:self.clock, self.errors.append)

    def ready(self, extended=True):
        s=self.session;s.start()
        s.receive("VERSION 1.0")
        s.receive("GSTA 0,1,1,0,E0000,")
        s.receive("SPEED SPEED 35")
        s.receive("PCONFIG 1,35" if extended else "PCONFIG E1000")
        if extended:
            s.receive("GSTATE 0,1,0,0,0,1,E0000")
            s.receive("GSENSOR 1,1,0,0,1,1,0,1,0")
        self.sent.clear()

    def test_handshake_is_serialized_and_controls_wait_for_state(self):
        s=self.session;s.start()
        self.assertEqual(self.sent,["VERSION"]);self.assertFalse(s.can_move)
        s.receive("VERSION 1.0")
        self.assertEqual(self.sent,["VERSION","GSTA"])
        s.receive("GSTA 0,1,1,0,E0000,")
        self.assertEqual(s.version,"1.0");self.assertEqual(s.state.homed,True)

    def test_rpos_pair_and_axis_reply_keep_signed_axes_distinct(self):
        self.ready()
        session = self.session
        self.assertTrue(session.request("RPOS"))
        session.receive("RPOS 12000,-3200")
        self.assertEqual(session.positions, {"Z": 12000, "R": -3200})
        self.assertIsNone(session.pending)
        self.assertTrue(session.request("RPOS R"))
        session.receive("RPOS -3400")
        self.assertEqual(session.positions, {"Z": 12000, "R": -3400})

    def test_legacy_z_only_rpos_preserves_reading_and_disables_axis_save(self):
        self.ready()
        session = self.session
        session.request("RPOS")
        session.receive("RPOS 12000")
        self.assertEqual(session.positions, {"Z": 12000})
        self.assertFalse(session.captured_targets)
        self.assertFalse(session.paired_rpos)
        self.assertIn("GPOS", self.errors[-1])
        self.assertFalse(session.request("SAVE Z"))

    def test_axis_save_reads_back_only_that_target_and_keeps_other_edits(self):
        self.ready()
        session = self.session
        session.verified_settings.update({51: (10000, 11000), 56: (0, 2000)})
        session.request("RPOS")
        session.receive("RPOS 12000,-3200")
        self.assertTrue(session.request("SAVE R"))
        session.receive("SAVE OK")
        self.assertEqual(self.sent[-1], "PGET 56")
        session.receive("PGET 56,-3200")
        self.assertEqual(session.parameters[56], -3200)
        self.assertEqual(session.verified_settings, {51: (10000, 11000)})
        self.assertIn("저장 확인", session.setting_text)
        self.assertTrue(session.request("SAVE Z"))
        session.receive("SAVE OK")
        self.assertEqual(self.sent[-1], "PGET 51")
        session.receive("PGET 51,12000")
        self.assertFalse(session.verified_settings)

    def test_axis_save_missing_capture_is_rejected_without_timeout(self):
        self.ready()
        session = self.session
        self.assertFalse(session.request("SAVE R"))
        session.request("RPOS")
        session.receive("RPOS 12000,-3200")
        self.assertTrue(session.request("SAVE R"))
        session.receive("SAVE NO_RPOS")
        self.assertIsNone(session.pending)
        self.assertIn("NO_RPOS", session.setting_text)
        self.assertFalse(session.request("SAVE R"))

    def test_axis_save_needs_a_new_capture_after_motion_or_reconnect(self):
        self.ready()
        session = self.session
        session.request("RPOS")
        session.receive("RPOS 12000,-3200")
        self.assertTrue(session.request("MOVE A Z 0"))
        session.receive("MOVE OK")
        session.receive("GSTATE 1,1,0,0,1,0,E0000")
        session.receive("GSTATE 0,1,0,0,0,1,E0000")
        self.assertNotIn("R", session.captured_targets)
        self.assertFalse(session.request("SAVE R"))
        self.ready()
        self.assertFalse(session.request("SAVE R"))
        self.assertFalse(session.request("SAVE 82"))

    def test_axis_save_readback_mismatch_does_not_confirm_the_capture(self):
        self.ready()
        session = self.session
        session.request("RPOS")
        session.receive("RPOS 12000,-3200")
        session.request("SAVE R")
        session.receive("SAVE OK")
        session.receive("PGET 56,100")
        self.assertIn("불일치", session.setting_text)
        self.assertFalse(session.request("SAVE 1"))

    def test_gsta_accepts_grip_suffix_and_preserves_legacy_handshake(self):
        s=self.session;s.start();s.receive("VERSION 1.0")
        s.receive("GSTA 0,1,1,0,E0000,1,2")
        self.assertIsNone(s.state)
        s.receive("GSTA 0,1,1,0,E0000,1,0")
        self.assertTrue(s.state.homed)
        self.assertEqual(self.sent[-1],"SPEED")
        s.receive("ST 1,1,1,0,E0000,")
        self.assertTrue(s.state.busy)

    def test_motion_is_never_retried_after_timeout(self):
        self.ready();s=self.session
        self.assertTrue(s.request("DECAP"));self.clock+=3.1;s.tick()
        self.assertEqual(self.sent.count("DECAP"),1)
        self.assertFalse(s.can_move);self.assertIn("DECAP",self.errors[-1])

    def test_busy_and_error_block_motion_but_stop_preempts(self):
        self.ready();s=self.session
        s.receive("GSTATE 1,1,0,0,5,0,E0000")
        self.assertFalse(s.request("MOVE R Z 100"))
        self.assertTrue(s.request("STOP"));self.assertEqual(self.sent[-1],"STOP")
        s.receive("STOP stop");s.receive("GSTATE 0,1,1,0,0,1,E2810")
        self.assertFalse(s.request("DECAP"))
        self.assertTrue(s.request("CLR"))

    def test_legacy_schema_rejection_preserves_basic_controls(self):
        self.ready(False)
        self.assertFalse(self.session.extended)
        self.assertTrue(self.session.can_move)
        self.assertFalse(self.session.request("GPOS Z"))
        self.assertNotIn("GPOS Z",self.sent)

    def test_malformed_state_cannot_unlock_controls(self):
        self.session.start()
        self.session.receive("GSTATE 0,1,0")
        self.assertFalse(self.session.can_move)

    def test_response_updates_speed_parameter_and_position(self):
        self.ready();s=self.session
        s.receive("SPEED SPEED 20");s.receive("GPOS R,-50")
        s.receive("PGET 51,1234")
        self.assertEqual(s.speed,20);self.assertEqual(s.positions["R"],-50)
        self.assertEqual(s.parameters[51],1234)

    def test_clear_request_refreshes_state_instead_of_starting_motion(self):
        self.ready();s=self.session;s.receive("GSTATE 0,1,1,0,0,1,E2810")
        self.assertTrue(s.request("CLR"));s.receive("CLR")
        self.assertEqual(self.sent,["CLR","GSTATE"])

    def test_stop_bypasses_waiting_response(self):
        self.ready();s=self.session;s.request("DECAP")
        self.assertTrue(s.request("STOP"))
        self.assertEqual(self.sent,["DECAP","STOP"])

    def test_disconnect_and_stale_state_disable_motion(self):
        self.ready();self.clock+=4
        self.assertFalse(self.session.can_move)
        self.session.stop()
        self.assertFalse(self.session.request("HOME"))

    def test_reconnect_does_not_replay_unanswered_motion(self):
        self.ready();s=self.session;s.request("DECAP")
        s.stop();s.start()
        self.assertEqual(self.sent.count("DECAP"),1)
        self.assertEqual(self.sent[-1],"VERSION")

    def test_fast_completion_is_correlated_to_new_operation(self):
        self.ready();s=self.session
        s.receive("GSTATE 0,1,0,0,0,1,E0000,7,2")
        s.request("HOME");s.receive("HOME home")
        s.receive("GSTATE 0,1,0,0,0,1,E0000,7,2")
        self.assertFalse(s.can_move)
        s.receive("GSTATE 0,1,0,0,0,1,E0000,8,2")
        self.assertTrue(s.can_move)

    def test_error_reply_is_reported_without_waiting_for_timeout(self):
        self.ready();s=self.session;s.request("GPOS Z")
        s.receive("GPOS INVALID")
        self.assertIsNone(s.pending)
        self.assertIn("INVALID",self.errors[-1])

    def test_wrong_parameter_reply_cannot_complete_current_request(self):
        self.ready();s=self.session;s.request("PGET 51")
        s.receive("PGET 52,500")
        self.assertIsNotNone(s.pending)
        s.receive("PGET 51,400")
        self.assertIsNone(s.pending)

    def test_capability_timeout_falls_back_without_replaying_motion(self):
        s=self.session;s.start()
        for line in ("VERSION 1.0","GSTA 0,1,1,0,E0000","SPEED SPEED 35"):
            s.receive(line)
        self.clock=3.1;s.tick()
        self.assertEqual(s.schema,0)
        self.clock=3.7;s.tick();s.receive("GSTA 0,1,1,0,E0000")
        self.assertTrue(s.can_move)
        self.assertFalse(s.request("TEACH 51,100"))

    def test_invalid_state_numbers_and_error_code_do_not_unlock_motion(self):
        for line in ("GSTATE 0,1,0,0,-1,1,E0000,1,2",
                     "GSTATE 0,1,0,0,0,1,E0000,-1,2",
                     "GSTATE 0,1,0,0,0,1,E0000,1,9",
                     "GSTATE 0,1,0,0,0,1,garbage,1,2",
                     "GSTA 0,1,1,0,garbage"):
            with self.subTest(line=line):
                self.session.start();self.session.receive(line)
                self.assertFalse(self.session.can_move)

    def test_parameter_limit_change_refreshes_cached_limits(self):
        self.ready();s=self.session;s.request("PL 42 150000")
        self.assertEqual(self.sent[-1],"PL 42,150000")
        s.receive("PL OK")
        self.assertEqual(self.sent[-1],"PGET 42")
        s.receive("PGET 42,150000")
        self.assertEqual(s.parameters[42],150000)

    def test_invalid_wire_commands_do_not_change_session_state(self):
        self.ready()
        s = self.session
        for command in ("ſtop", "GSTA " + "A" * 249, "GSTA\r\nSTOP", "HOME\0", "STOP 1"):
            with self.subTest(command=command[:12]):
                self.assertFalse(s.request(command))
                self.assertIsNone(s.pending)
        self.assertEqual(self.sent, [])
        self.assertTrue(s.can_move)

    def test_unverified_limit_is_not_used_to_accept_a_move(self):
        self.ready()
        s = self.session
        self.assertTrue(s.request("PL"))
        s.receive(parameter_snapshot())
        self.assertTrue(s.target_in_range("Z", 140000))
        self.assertTrue(s.request("PL 42,120000"))
        self.clock = 3.1
        s.tick()
        s.receive("GSTATE 0,1,0,0,0,1,E0000")
        self.assertFalse(s.target_in_range("Z", 140000))
        self.assertTrue(s.target_in_range("R", 1000))
        self.assertTrue(s.request("PGET 42"))
        s.receive("PGET 42,120000")
        self.assertTrue(s.target_in_range("Z", 100000))

    def test_reload_timeout_invalidates_limits_and_requests_readback(self):
        self.ready()
        s = self.session
        self.assertTrue(s.request("PL"))
        s.receive(parameter_snapshot())
        self.assertTrue(s.request("LOAD"))
        self.clock = 3.1
        s.tick()
        self.assertFalse(s.target_in_range("Z", 1000))
        s.receive("GSTATE 0,0,0,0,0,1,E0000")
        s.tick()
        self.assertEqual(self.sent[-1], "PL")
        s.receive(parameter_snapshot())
        self.assertTrue(s.target_in_range("Z", 1000))
        self.assertEqual(s.setting_text, "장비 설정 재조회 완료")
