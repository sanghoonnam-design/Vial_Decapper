import unittest
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
        self.ready();s=self.session;s.request("PL 42,150000");s.receive("PL OK")
        self.assertEqual(self.sent[-1],"PGET 42")
        s.receive("PGET 42,150000")
        self.assertEqual(s.parameters[42],150000)
