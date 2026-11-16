from PySide6.QtCore import QObject, Signal
from PySide6.QtNetwork import QAbstractSocket, QTcpSocket

from ui.command_wire import build_command_payload


class TcpCommandClient(QObject):
    connected = Signal()
    disconnected = Signal()
    received = Signal(bytes)
    sent = Signal(bytes)
    error = Signal(str)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self.socket = QTcpSocket(self)
        self.socket.connected.connect(self.connected)
        self.socket.disconnected.connect(self.disconnected)
        self.socket.readyRead.connect(self._read_available_data)
        self.socket.errorOccurred.connect(lambda _: self.error.emit(self.socket.errorString()))

    build_command_payload = staticmethod(build_command_payload)

    def connect_to_host(self, host: str, port: int) -> None:
        self.socket.connectToHost(host, port)

    def disconnect_from_host(self) -> None:
        # abort() closes immediately, including Connecting/Closing states;
        # waiting for a silent device to complete a graceful close can leave
        # the UI apparently stuck on Disconnect.
        if self.socket.state() != QAbstractSocket.SocketState.UnconnectedState:
            self.socket.abort()

    def is_connected(self) -> bool:
        return self.socket.state() == QAbstractSocket.SocketState.ConnectedState

    def send_command(self, command: str, append_cr: bool, append_lf: bool) -> bytes:
        try:
            payload = self.build_command_payload(command, append_cr, append_lf)
        except ValueError as error:
            self.error.emit(str(error))
            return b""
        if not self.is_connected():
            self.error.emit("Cannot send: TCP socket is not connected.")
            return b""
        written = self.socket.write(payload)
        if written != len(payload):
            self.error.emit(f"TCP write failed or incomplete ({written}/{len(payload)} bytes): {self.socket.errorString()}")
            self.disconnect_from_host()
            return b""
        # Accepted into the local transport buffer; device execution needs an RX response.
        self.sent.emit(payload)
        return payload

    def _read_available_data(self) -> None:
        self.received.emit(bytes(self.socket.readAll()))
