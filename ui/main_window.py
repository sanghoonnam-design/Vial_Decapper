from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPlainTextEdit,
    QPushButton,
    QSplitter,
    QVBoxLayout,
    QWidget,
)


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Vial Decapper")
        self.setMinimumSize(800, 600)

        self.setMenuWidget(self._create_connection_bar())
        self.setCentralWidget(self._create_workspace())

    def _create_workspace(self) -> QWidget:
        workspace = QWidget()
        layout = QVBoxLayout(workspace)
        layout.setContentsMargins(16, 0, 16, 16)

        separator = QFrame()
        separator.setObjectName("connection_separator")
        separator.setFrameShape(QFrame.Shape.HLine)
        separator.setFrameShadow(QFrame.Shadow.Sunken)
        layout.addWidget(separator)

        content_splitter = QSplitter(Qt.Orientation.Horizontal)
        content_splitter.setObjectName("content_splitter")

        visual_area = QWidget()
        visual_area.setObjectName("visual_area")
        content_splitter.addWidget(visual_area)

        right_splitter = QSplitter(Qt.Orientation.Vertical)
        right_splitter.setObjectName("right_splitter")

        input_area = QWidget()
        input_area.setObjectName("input_area")
        right_splitter.addWidget(input_area)
        right_splitter.addWidget(self._create_command_area())

        content_splitter.addWidget(right_splitter)
        content_splitter.setStretchFactor(0, 2)
        content_splitter.setStretchFactor(1, 3)
        right_splitter.setStretchFactor(0, 1)
        right_splitter.setStretchFactor(1, 1)
        layout.addWidget(content_splitter)

        return workspace

    def _create_command_area(self) -> QWidget:
        command_area = QWidget()
        command_area.setObjectName("command_area")
        layout = QVBoxLayout(command_area)

        layout.addWidget(QLabel("Command"))

        command_input = QPlainTextEdit()
        command_input.setObjectName("command_input")
        command_input.setPlaceholderText("Enter command")
        layout.addWidget(command_input)

        command_send_button = QPushButton("Send")
        command_send_button.setObjectName("command_send_button")
        layout.addWidget(command_send_button)

        return command_area

    def _create_connection_bar(self) -> QWidget:
        connection_bar = QWidget()
        layout = QHBoxLayout(connection_bar)
        layout.setContentsMargins(16, 10, 16, 10)

        title_label = QLabel("Vial Decapper")
        layout.addWidget(title_label)
        layout.addStretch()

        layout.addWidget(QLabel("TCP/IP"))

        ip_address_input = QLineEdit()
        ip_address_input.setObjectName("ip_address_input")
        ip_address_input.setPlaceholderText("IP Address")
        layout.addWidget(ip_address_input)

        port_input = QLineEdit()
        port_input.setObjectName("port_input")
        port_input.setPlaceholderText("Port")
        port_input.setMaximumWidth(90)
        layout.addWidget(port_input)

        connect_button = QPushButton("Connect")
        connect_button.setObjectName("connect_button")
        layout.addWidget(connect_button)

        return connection_bar
