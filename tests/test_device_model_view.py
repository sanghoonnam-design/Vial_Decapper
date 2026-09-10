import os
import unittest

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')

from PySide6.QtCore import QPoint, QPointF, Qt
from PySide6.QtGui import QWheelEvent
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication, QSlider
from unittest.mock import Mock

from ui.device_model_view import DeviceModelView
from ui.device_model import build_device_mesh


class DeviceModelViewTests(unittest.TestCase):
    def test_carriage_travel_only_translates_lower_parts_front_to_back(self):
        rear = build_device_mesh(0)
        front = build_device_mesh(1)
        moving = []
        fixed = []
        for (before, color), (after, other_color) in zip(rear, front):
            self.assertEqual(color, other_color)
            if before == after:
                fixed.extend(before)
            else:
                moving.extend(before)
                for a, b in zip(before, after):
                    self.assertEqual(a[:2], b[:2])
                    self.assertAlmostEqual(b[2] - a[2], -65)
        self.assertTrue(moving)
        self.assertTrue(fixed)
        self.assertLess(max(p[1] for p in moving), 190)
        self.assertEqual(build_device_mesh(-1), rear)
        self.assertEqual(build_device_mesh(2), front)

    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_drag_changes_render_and_double_click_restores_view(self):
        view = DeviceModelView()
        self.addCleanup(view.close)
        view.resize(400, 600)
        view.show()
        self.app.processEvents()
        original = view.grab().toImage()
        QTest.mousePress(view, Qt.MouseButton.LeftButton, pos=QPoint(100, 100))
        QTest.mouseMove(view, QPoint(180, 130))
        QTest.mouseRelease(view, Qt.MouseButton.LeftButton, pos=QPoint(180, 130))
        self.assertNotEqual(original, view.grab().toImage())
        QTest.mouseDClick(view, Qt.MouseButton.LeftButton, pos=QPoint(180, 130))
        self.assertEqual(original, view.grab().toImage())

    def test_wheel_changes_render_and_extreme_input_remains_bounded(self):
        view = DeviceModelView()
        self.addCleanup(view.close)
        view.resize(400, 600)
        original = view.grab().toImage()
        def scroll(delta):
            event = QWheelEvent(QPointF(100, 100), QPointF(100, 100), QPoint(),
                                QPoint(0, delta), Qt.MouseButton.NoButton,
                                Qt.KeyboardModifier.NoModifier, Qt.ScrollPhase.NoScrollPhase, False)
            QApplication.sendEvent(view, event)
        scroll(120)
        self.assertNotEqual(original, view.grab().toImage())
        for _ in range(20):
            scroll(12000)
        maximum = view.grab().toImage()
        scroll(12000)
        self.assertEqual(maximum, view.grab().toImage())
        for _ in range(20):
            scroll(-12000)
        minimum = view.grab().toImage()
        scroll(-12000)
        self.assertEqual(minimum, view.grab().toImage())
        self.assertNotEqual(maximum, minimum)

    def test_preview_slider_moves_model_without_sending_machine_commands(self):
        from ui.main_window import MainWindow
        window = MainWindow()
        self.addCleanup(window.close)
        window.command_client.send_command = Mock()
        window.serial_client.send_command = Mock()
        slider = window.findChild(QSlider, 'carriage_preview_slider')
        self.assertIsNotNone(slider)
        view = window.device_model_view
        before = view.grab().toImage()
        slider.setValue(slider.maximum())
        self.assertNotEqual(before, view.grab().toImage())
        window.command_client.send_command.assert_not_called()
        window.serial_client.send_command.assert_not_called()
