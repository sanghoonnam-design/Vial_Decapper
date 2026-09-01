import os
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import main


class MainTests(unittest.TestCase):
    def test_main_shows_window_and_returns_event_loop_result(self) -> None:
        with patch.object(main.QApplication, "exec", return_value=0) as exec_mock:
            self.assertEqual(main.main(), 0)

        exec_mock.assert_called_once()
