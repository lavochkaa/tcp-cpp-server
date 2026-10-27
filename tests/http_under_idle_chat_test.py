#!/usr/bin/env python3
"""Linux smoke test: idle chat connections must not block HTTP requests."""

import socket
import subprocess
import sys
import time
from pathlib import Path


def connect_when_ready(process, port):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"server exited with status {process.returncode}")
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=1)
        except OSError:
            time.sleep(0.05)
    raise TimeoutError(f"server did not listen on port {port}")


def recv_until(sock, delimiter):
    data = b""
    while delimiter not in data:
        chunk = sock.recv(64)
        if not chunk:
            raise AssertionError(f"connection closed before {delimiter!r}: {data!r}")
        data += chunk
    return data


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 tests/http_under_idle_chat_test.py ./server")

    binary = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parents[1]
    process = subprocess.Popen(
        [str(binary)], cwd=root / "src", stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    clients = []
    try:
        for _ in range(8):
            client = connect_when_ready(process, 8081)
            clients.append(client)
            client.settimeout(3)
            prompt = recv_until(client, b"Login: ")
            if prompt != b"Login: ":
                raise AssertionError(f"unexpected chat prompt: {prompt!r}")

        with connect_when_ready(process, 8080) as http:
            http.settimeout(3)
            http.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            response = recv_until(http, b"\r\n")
            if not response.startswith(b"HTTP/1.1 200 OK\r\n"):
                raise AssertionError(f"HTTP unavailable with eight idle chat clients: {response!r}")
    finally:
        for client in clients:
            client.close()
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == "__main__":
    main()
