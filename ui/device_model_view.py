"""Small orthographic mesh viewer; no OpenGL context or render loop required."""

from math import cos, sin, radians, sqrt

from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QColor, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget

from ui.device_model import build_device_mesh


class DeviceModelView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName('device_model_view')
        self.setMinimumSize(180, 240)
        self.setCursor(Qt.CursorShape.OpenHandCursor)
        self.setToolTip('드래그: 회전 · 휠: 확대/축소 · 더블클릭: 기본 시점')
        self._faces = build_device_mesh()
        self._last_position = None
        self.reset_view()

    def reset_view(self):
        self.yaw, self.pitch, self.zoom = -32.0, -16.0, 1.0
        self.update()

    def set_carriage_position(self, percent):
        """Change only the illustrative carriage; this never sends device commands."""
        self._faces = build_device_mesh(percent / 100)
        self.update()

    def _rotate(self, point):
        x, y, z = point
        y -= 167
        a, b = radians(self.yaw), radians(self.pitch)
        x, z = x*cos(a) + z*sin(a), -x*sin(a) + z*cos(a)
        return x, y*cos(b) - z*sin(b), y*sin(b) + z*cos(b)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.fillRect(self.rect(), QColor('#0c1627'))
        scale = min(self.width()/320, self.height()/430) * self.zoom

        def project(point):
            return QPointF(self.width()/2 + point[0]*scale,
                           self.height()/2 - point[1]*scale)

        # Draw the floor first; it is deliberately outside the equipment mesh.
        painter.setPen(QPen(QColor('#1c3048'), 1))
        for offset in range(-140, 141, 28):
            for start, end in (((offset,-8,-140),(offset,-8,140)),
                               ((-140,-8,offset),(140,-8,offset))):
                painter.drawLine(project(self._rotate(start)), project(self._rotate(end)))

        transformed = [(tuple(self._rotate(p) for p in points), color)
                       for points, color in self._faces]
        # Orthographic painter ordering is sufficient for these simple solid parts.
        transformed.sort(key=lambda face: sum(p[2] for p in face[0])/len(face[0]), reverse=True)
        for points, color in transformed:
            a, b, c = points[:3]
            u = tuple(b[i]-a[i] for i in range(3))
            v = tuple(c[i]-a[i] for i in range(3))
            normal = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
            length = sqrt(sum(n*n for n in normal)) or 1
            light = .58 + .42*abs(sum(n*l for n, l in zip(normal, (-.35,.65,-.67)))/length)
            base = QColor(color)
            shade = QColor(*(min(255, int(channel*light)) for channel in (base.red(),base.green(),base.blue())))
            painter.setBrush(shade)
            painter.setPen(QPen(shade.darker(140), .65))
            painter.drawPolygon(QPolygonF([project(p) for p in points]))
        painter.end()

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self._last_position = event.position()
            self.setCursor(Qt.CursorShape.ClosedHandCursor)
            event.accept()
        else:
            super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._last_position is not None and event.buttons() & Qt.MouseButton.LeftButton:
            delta = event.position() - self._last_position
            self.yaw = (self.yaw + delta.x()*.5) % 360
            self.pitch = max(-75, min(75, self.pitch + delta.y()*.4))
            self._last_position = event.position()
            self.update()
            event.accept()
        else:
            super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self._last_position = None
            self.setCursor(Qt.CursorShape.OpenHandCursor)
        super().mouseReleaseEvent(event)

    def wheelEvent(self, event):
        steps = max(-10, min(10, event.angleDelta().y()/120))
        self.zoom = max(.55, min(2.5, self.zoom * 1.12**steps))
        self.update()
        event.accept()

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.reset_view()
            event.accept()
        else:
            super().mouseDoubleClickEvent(event)
