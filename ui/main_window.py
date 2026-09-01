from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPushButton,
    QWidget,
)


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Vial Decapper")
        self.setMinimumSize(800, 600)

        self.setMenuWidget(self._create_connection_bar())

        welcome_label = QLabel("PySide6 application template")
        welcome_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setCentralWidget(welcome_label)

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
