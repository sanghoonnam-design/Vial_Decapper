"""Styles shared by the Decapper workspace."""

WINDOW_STYLE = """
    QMainWindow { background: #101b25; color: #e4edf3; }
    QLabel, QCheckBox { color: #b9cbd6; }
    QWidget#connection_bar {
        background: #0d1720;
        border-bottom: 1px solid #2a414f;
    }
    QFrame#connection_separator { background: #2a414f; border: 0; }
    QLabel#app_title { color: #f4f8fb; font-size: 23px; font-weight: 700; }
    QLabel#brand_mark {
        background: #21443f;
        border: 1px solid #4d9387;
        border-radius: 8px;
        color: #a8e8dd;
        font-size: 24px;
    }
    QGroupBox {
        background: #172531;
        border: 1px solid #2f4656;
        border-radius: 10px;
        margin-top: 14px;
        padding: 9px;
        font-weight: 600;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 6px;
        color: #9ad3ce;
    }
    QGroupBox#input_area { padding: 6px; }
    QLineEdit, QPlainTextEdit, QComboBox, QSpinBox {
        background: #0d1923;
        border: 1px solid #365364;
        border-radius: 6px;
        padding: 5px 7px;
        color: #e4edf3;
        selection-background-color: #376c65;
    }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #72c9ba; }
    QComboBox QAbstractItemView {
        background: #172531;
        color: #e4edf3;
        selection-background-color: #284e50;
    }
    QPlainTextEdit#communication_log {
        background: #0c151d;
        color: #d2e2e9;
        font-family: 'Consolas', 'Courier New', monospace;
        padding: 8px;
    }
    QPushButton {
        background: #233846;
        border: 1px solid #3c596b;
        border-radius: 6px;
        color: #e4edf3;
        font-weight: 600;
        padding: 6px 10px;
    }
    QGroupBox#input_area QPushButton { padding: 4px 8px; }
    QPushButton:hover { background: #2c495a; border-color: #608595; }
    QPushButton:focus { border-color: #91d9ce; }
    QPushButton:pressed { background: #162a36; }
    QPushButton#connect_button, QPushButton#command_send_button {
        background: #28665e; border-color: #4b9588; color: #f2fffc;
    }
    QPushButton#connect_button:hover, QPushButton#command_send_button:hover { background: #347a70; }
    QPushButton[danger="true"] { background: #913d4e; border-color: #ba6070; color: #fff6f7; }
    QPushButton[danger="true"]:hover { background: #a9495c; }
    QPushButton[danger="true"]:pressed { background: #75303f; }
    QPushButton:disabled, QPushButton#command_send_button:disabled {
        background: #192833; color: #728895; border-color: #304653;
    }
    QPushButton[danger="true"]:disabled {
        background: #3a2c36; border-color: #60424d; color: #b18a97;
    }
    QTabWidget::pane { border: 1px solid #324c5d; border-radius: 6px; }
    QTabBar::tab {
        background: #1b2e3b; color: #b6c9d5; padding: 6px 13px;
        border: 1px solid #324c5d; border-bottom: 2px solid #324c5d;
    }
    QTabBar::tab:selected {
        background: #234349; color: #d9fff7; border-bottom-color: #75d2c1;
    }
    QTabBar::tab:!selected:hover { background: #263e4c; }
    QWidget#motion_page { background: #15232e; }
    QLabel#device_state {
        background: #13212c; color: #b9cbd6; border-left: 3px solid #66818f;
        border-radius: 4px; padding: 3px 7px; font-size: 13px; font-weight: 700;
    }
    QLabel#device_state[tone="ready"] { color: #a9e9dc; border-left-color: #69ccb8; }
    QLabel#device_state[tone="active"] { color: #acd7f1; border-left-color: #78b9e2; }
    QLabel#device_state[tone="waiting"] { color: #f3d49c; border-left-color: #d1a356; }
    QLabel#device_state[tone="fault"] { color: #ffb6bf; border-left-color: #e27888; }
    QLabel#device_fault { color: #ffb6bf; padding: 2px 7px; font-size: 12px; font-weight: 700; }
    QLabel#move_command_label, QLabel#rpos_command_label { color: #98cfc9; font-weight: 700; }
    QLabel#current_z_position_value { color: #d9fff7; font-size: 22px; font-weight: 700; }
    QLabel#visual_caption { color: #9eb2bf; font-size: 11px; }
    QTableWidget#diagnostic_table {
        background: #0d1923; alternate-background-color: #152630; color: #e4edf3;
        gridline-color: #2a414f; border: 1px solid #365364; border-radius: 5px;
        selection-background-color: #284e50;
    }
    QTableWidget#diagnostic_table QHeaderView::section {
        background: #223a48; color: #c4dce5; padding: 3px 7px;
        border: 0; border-bottom: 1px solid #365364;
    }
    QScrollBar:vertical { background: #101c26; width: 10px; }
    QScrollBar::handle:vertical { background: #466373; min-height: 24px; border-radius: 4px; }
    QScrollBar::handle:vertical:hover { background: #628393; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    QSlider::groove:horizontal { background: #2a4352; height: 4px; border-radius: 2px; }
    QSlider::handle:horizontal {
        background: #78cbbd; border: 1px solid #a2e2d7; width: 12px;
        margin: -4px 0; border-radius: 6px;
    }
    QSlider::handle:horizontal:disabled { background: #607786; border-color: #788e9a; }
    QSplitter::handle { background: #2a414f; }
"""

LONGRUN_STYLE = """
    QPushButton { font-size: 12px; padding: 3px 5px; }
    QSpinBox { font-size: 12px; padding: 2px 3px; }
    QTableWidget {
        background: #0d1923; alternate-background-color: #14252f;
        color: #d8e7ed; gridline-color: #2a414f;
        selection-background-color: #284e50; border: 1px solid #365364;
    }
    QHeaderView::section {
        background: #223a48; color: #c4dce5; padding: 2px;
        border: 0; border-bottom: 1px solid #365364;
    }
    QTableWidget QLineEdit { padding: 2px; border: 0; border-radius: 0; }
"""
