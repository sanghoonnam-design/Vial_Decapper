from PySide6.QtCore import Qt
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QFrame,
    QGroupBox,
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
        self.setStyleSheet(
            """
            QMainWindow { background: #0a1020; color: #e6edf7; }
            QWidget#connection_bar {
                background: #0e1930;
                border-bottom: 1px solid #223452;
            }
            QLabel#app_title { color: #f4f8ff; font-size: 24px; font-weight: 700; }
            QLabel#brand_mark {
                background: #102a47;
                border: 1px solid #38bdf8;
                border-radius: 10px;
                color: #67e8f9;
                font-size: 26px;
            }
            QGroupBox {
                background: #111c31;
                border: 1px solid #243858;
                border-radius: 10px;
                margin-top: 14px;
                padding: 12px;
                font-weight: 600;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
                color: #7dd3fc;
            }
            QLineEdit, QPlainTextEdit {
                background: #091323;
                border: 1px solid #2b4266;
                border-radius: 6px;
                padding: 7px;
                color: #dbeafe;
                selection-background-color: #2563eb;
            }
            QPushButton {
                background: #1b3150;
                border: none;
                border-radius: 6px;
                color: #cfe8ff;
                font-weight: 600;
                padding: 8px 14px;
            }
            QPushButton#connect_button, QPushButton#command_send_button {
                background: #0284c7;
                color: #f0f9ff;
            }
            QPushButton:hover { background: #27466f; }
            QPushButton#connect_button:hover, QPushButton#command_send_button:hover {
                background: #0ea5e9;
            }
            QSplitter::handle { background: #223452; }
            QLabel#visual_pattern { color: #38bdf8; font-size: 76px; }
            QLabel#visual_caption { color: #6b88ad; font-size: 12px; font-weight: 600; }
            """
        )

        self.setMenuWidget(self._create_connection_bar())
        self.setCentralWidget(self._create_workspace())

    def _create_workspace(self) -> QWidget:
        workspace = QWidget()
        layout = QVBoxLayout(workspace)
        layout.setContentsMargins(20, 0, 20, 20)
        layout.setSpacing(16)

        separator = QFrame()
        separator.setObjectName("connection_separator")
        separator.setFrameShape(QFrame.Shape.HLine)
        separator.setFrameShadow(QFrame.Shadow.Sunken)
        layout.addWidget(separator)

        content_splitter = QSplitter(Qt.Orientation.Horizontal)
        content_splitter.setObjectName("content_splitter")

        visual_area = QGroupBox("Vial Decapper Image")
        visual_area.setObjectName("visual_area")
        visual_layout = QVBoxLayout(visual_area)
        visual_layout.addStretch()

        visual_pattern = QLabel("◈")
        visual_pattern.setObjectName("visual_pattern")
        visual_pattern.setAlignment(Qt.AlignmentFlag.AlignCenter)
        visual_layout.addWidget(visual_pattern)

        visual_caption = QLabel("DEVICE IMAGE PREVIEW")
        visual_caption.setObjectName("visual_caption")
        visual_caption.setAlignment(Qt.AlignmentFlag.AlignCenter)
        visual_layout.addWidget(visual_caption)
        visual_layout.addStretch()
        content_splitter.addWidget(visual_area)

        right_splitter = QSplitter(Qt.Orientation.Vertical)
        right_splitter.setObjectName("right_splitter")

        input_area = QGroupBox("Teaching Point")
        input_area.setObjectName("input_area")
        right_splitter.addWidget(input_area)
        right_splitter.addWidget(self._create_command_area())

        content_splitter.addWidget(right_splitter)
        content_splitter.setStretchFactor(0, 1)
        content_splitter.setStretchFactor(1, 1)
        content_splitter.setSizes([500, 500])
        right_splitter.setStretchFactor(0, 1)
        right_splitter.setStretchFactor(1, 1)
        right_splitter.setSizes([500, 500])
        layout.addWidget(content_splitter)

        return workspace

    def _create_command_area(self) -> QWidget:
        command_area = QGroupBox("Command")
        command_area.setObjectName("command_area")
        layout = QVBoxLayout(command_area)

        command_row = QHBoxLayout()

        command_input = QLineEdit()
        command_input.setObjectName("command_input")
        command_input.setPlaceholderText("Enter command")
        command_input.setMaximumHeight(32)
        command_row.addWidget(command_input)

        command_send_button = QPushButton("Send")
        command_send_button.setObjectName("command_send_button")
        command_row.addWidget(command_send_button)
        layout.addLayout(command_row)

        communication_log = QPlainTextEdit()
        communication_log.setObjectName("communication_log")
        communication_log.setReadOnly(True)
        communication_log.setPlaceholderText("TX / RX communication log")
        layout.addWidget(communication_log)

        return command_area

    def _create_connection_bar(self) -> QWidget:
        connection_bar = QWidget()
        connection_bar.setObjectName("connection_bar")
        layout = QHBoxLayout(connection_bar)
        layout.setContentsMargins(20, 14, 20, 14)
        layout.setSpacing(10)

        title_label = QLabel("Vial Decapper")
        title_label.setObjectName("app_title")
        title_label.setFont(QFont("Segoe UI", 22, QFont.Weight.Bold))
        brand_mark = QLabel("◈")
        brand_mark.setObjectName("brand_mark")
        brand_mark.setAlignment(Qt.AlignmentFlag.AlignCenter)
        brand_mark.setFixedSize(40, 40)
        layout.addWidget(brand_mark)
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
