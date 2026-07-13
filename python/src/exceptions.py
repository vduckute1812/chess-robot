"""Project-wide exception types."""


class CameraError(Exception):
    """Raised when the webcam cannot be opened or read."""


class CalibrationError(Exception):
    """Raised when calibration cannot be completed."""


class BoardDetectionError(Exception):
    """Raised when the chessboard cannot be detected or warped."""
