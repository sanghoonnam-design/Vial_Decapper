import os
import unittest
from unittest.mock import Mock

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from ui.tcp_command_client import TcpCommandClient


class TcpCommandClientTests(unittest.TestCase):
    def test_payload_uses_only_selected_terminators(self) -> None:
        self.assertEqual(TcpCommandClient.build_command_payload("GSTA", False, False), b"GSTA")
        self.assertEqual(TcpCommandClient.build_command_payload("GSTA", True, False), b"GSTA\r")
        self.assertEqual(TcpCommandClient.build_command_payload("GSTA", False, True), b"GSTA\n")
        self.assertEqual(TcpCommandClient.build_command_payload("GSTA", True, True), b"GSTA\r\n")

    def test_disconnect_aborts_a_connection_attempt(self) -> None:
        client = TcpCommandClient()
        client.socket.state = Mock(return_value=client.socket.SocketState.ConnectingState)
        client.socket.abort = Mock()
        client.socket.disconnectFromHost = Mock()

        client.disconnect_from_host()

        client.socket.abort.assert_called_once_with()
        client.socket.disconnectFromHost.assert_not_called()
