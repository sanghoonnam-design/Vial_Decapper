from PySide6.QtCore import QObject, Signal
from PySide6.QtSerialPort import QSerialPort


class SerialCommandClient(QObject):
    connected = Signal()
    disconnected = Signal()
    received = Signal(bytes)
    sent = Signal(bytes)
    error = Signal(str)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self.serial = QSerialPort(self)
        self.serial.setBaudRate(115200)
        self.serial.readyRead.connect(self._read_available_data)
        self.serial.errorOccurred.connect(self._on_error)

    def open(self, port_name: str, baud_rate: int) -> None:
        self.serial.setPortName(port_name)
        self.serial.setBaudRate(baud_rate)
        if self.serial.open(QSerialPort.OpenModeFlag.ReadWrite):
            self.connected.emit()
        else:
            self.error.emit(self.serial.errorString())

    def close(self) -> None:
        if self.serial.isOpen():
            self.serial.close()
            self.disconnected.emit()

    def is_connected(self) -> bool:
        return self.serial.isOpen()

    def send_command(self, command: str, append_cr: bool, append_lf: bool) -> bytes:
        if not command.strip():
            self.error.emit("Cannot send an empty command.")
            return b""
        if not self.is_connected():
            self.error.emit("Cannot send: UART port is not connected.")
            return b""
        try:
            payload = command.strip().encode("ascii")
        except UnicodeEncodeError:
            self.error.emit("Commands must use ASCII characters.")
            return b""
        if append_cr:
            payload += b"\r"
        if append_lf:
            payload += b"\n"
        self.serial.write(payload)
        self.sent.emit(payload)
        return payload

    def _read_available_data(self) -> None:
        self.received.emit(bytes(self.serial.readAll()))

    def _on_error(self, _error: QSerialPort.SerialPortError) -> None:
        if self.serial.errorString():
            self.error.emit(self.serial.errorString())
