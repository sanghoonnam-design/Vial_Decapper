import os
import time
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from PySide6.QtNetwork import QHostAddress, QTcpServer
from PySide6.QtWidgets import QApplication
from ui.longrun_panel import LongrunPanel


class MultiLongrunNetworkTests(unittest.TestCase):
    def test_two_local_modules_execute_independently_and_all_stop_reaches_both(self):
        app = QApplication.instance() or QApplication([])
        server = QTcpServer()
        self.assertTrue(server.listen(QHostAddress.SpecialAddress.AnyIPv4, 0))
        self.addCleanup(server.close)
        panel = LongrunPanel()
        self.addCleanup(panel.close_connections)
        panel.port_input.setValue(server.serverPort())
        panel.set_ip(0, "127.0.0.1")
        panel.add_module("127.0.0.2")
        peers = []

        def wait_for(predicate):
            end = time.monotonic() + 3
            while not predicate() and time.monotonic() < end:
                app.processEvents()
                for peer in peers:
                    data = bytes(peer["socket"].readAll())
                    peer["buffer"] += data
                    while b"\n" in peer["buffer"]:
                        command, peer["buffer"] = peer["buffer"].split(b"\n", 1)
                        command = command.strip().decode()
                        peer["commands"].append(command)
                        if command.startswith("LR "):
                            peer["started"] = True
                        if command == "STOP":
                            peer["stopped"] = True
                        state = ("0,1,0,0,0,1,E0000,1,3" if peer["stopped"] else
                                 "1,1,0,0,16,0,E0000,1,1" if peer["started"] else
                                 "0,1,0,0,0,1,E0000,0,0")
                        replies = {"VERSION": "1.0", "GSTA": "0,1,1,0,E0000",
                                   "SPEED": "100", "PCONFIG": "1,35",
                                   "GSTATE": state, "STOP": "stop", "GLR": "0,3,0,0,0"}
                        body = replies.get(command)
                        if body is not None:
                            peer["socket"].write(f"{command} {body}\r\n".encode())
                time.sleep(.001)
            self.assertTrue(predicate(), "Local multi-module operation timed out")

        panel.connect_selected()
        for _ in range(2):
            wait_for(lambda: server.hasPendingConnections())
            peers.append({"socket": server.nextPendingConnection(), "commands": [], "buffer": b"",
                          "started": False, "stopped": False})
        wait_for(lambda: all(m.session.can_move and not m.session.pending for m in panel.modules))
        self.assertTrue(panel.start_selected(3))
        wait_for(lambda: all("LR 3" in peer["commands"] for peer in peers))
        for index, peer in enumerate(peers):
            peer["socket"].write(b"LR LR\r\nGSTATE 1,1,0,0,16,0,E0000,1,1\r\n")
            peer["socket"].write(f"GLR {index},3,0,0,1\r\n".encode())
        wait_for(lambda: all(m.session.longrun for m in panel.modules))
        self.assertEqual(sorted(m.session.longrun[0] for m in panel.modules), [0, 1])
        panel.set_checked(0, False)
        panel.stop_all()
        wait_for(lambda: all("STOP" in peer["commands"] for peer in peers))
        wait_for(lambda: all(not m.session.stop_wait for m in panel.modules))
