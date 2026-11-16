"""Shared framing for Decapper commands and replies."""
import re


MAX_COMMAND_BYTES = 253
MAX_REPLY_BYTES = 64 * 1024
ANSI_SEQUENCE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def command_bytes(command):
    command = command.strip()
    if not command:
        raise ValueError("Cannot send an empty command.")
    if not command.isascii():
        raise ValueError("Commands must use ASCII characters.")
    if not command.isprintable():
        raise ValueError("Commands must be a single line of printable ASCII.")
    if len(command) > MAX_COMMAND_BYTES:
        raise ValueError(f"Command exceeds {MAX_COMMAND_BYTES} bytes.")
    return command.encode("ascii")


def build_command_payload(command, append_cr, append_lf):
    payload = command_bytes(command)
    if append_cr:
        payload += b"\r"
    if append_lf:
        payload += b"\n"
    return payload


class ResponseStream:
    def __init__(self, overflow):
        self.overflow = overflow
        self.reset()

    def reset(self):
        self.buffer = bytearray()
        self.discarding = False

    def feed(self, data):
        buffer = self.buffer
        parts = data.split(b"\n")
        for index, part in enumerate(parts):
            if buffer is not self.buffer:
                return  # The connection was reset while handling the previous reply.
            if not self.discarding:
                if len(buffer) + len(part) > MAX_REPLY_BYTES:
                    buffer.clear()
                    self.discarding = True
                    self.overflow()
                else:
                    buffer.extend(part)
            if index == len(parts) - 1:
                break
            if self.discarding:
                self.discarding = False
                continue
            text = buffer.decode("utf-8", errors="replace")
            buffer.clear()
            console_noise = "RND>" in text or "\x1b[" in text
            for line in ANSI_SEQUENCE.sub("", text).splitlines():
                if buffer is not self.buffer:
                    return
                line = line.strip()
                if not line or line.startswith("RND>") or line in {"[2K", "[K"}:
                    continue
                if not (console_noise and len(line) == 1 and line.isprintable()):
                    yield line
