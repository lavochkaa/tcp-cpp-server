#!/usr/bin/env python3
"""Linux smoke test: TCP and HTTP must never claim the same username."""

import socket
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def connect_when_ready(process, port):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"server exited with status {process.returncode}")
        try:
            sock = socket.create_connection(("127.0.0.1", port), timeout=1)
            sock.settimeout(3)
            return sock
        except OSError:
            time.sleep(0.05)
    raise TimeoutError(f"server did not listen on port {port}")


def recv_until(sock, delimiter):
    data = b""
    while delimiter not in data:
        chunk = sock.recv(4096)
        if not chunk:
            raise AssertionError(f"connection closed before {delimiter!r}: {data!r}")
        data += chunk
    return data


def tcp_login(process, username, barrier=None):
    sock = connect_when_ready(process, 8081)
    try:
        assert recv_until(sock, b"Login: ") == b"Login: "
        if barrier:
            barrier.wait()
        sock.sendall(username.encode("ascii") + b"\n")
        reply = recv_until(sock, b"\n")
        if reply == b"Authorization successful!\n":
            return True, sock
        assert reply == b"User already logged in!\n", reply
        sock.close()
        return False, None
    except BaseException:
        sock.close()
        raise


def http_request(process, request):
    with connect_when_ready(process, 8080) as sock:
        sock.sendall(request)
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                return response
            response += chunk
            assert len(response) < 65536, "HTTP response exceeded smoke-test limit"


def http_login(process, username, barrier=None):
    body = b"login=" + username.encode("ascii")
    request = (b"POST /login HTTP/1.1\r\nHost: localhost\r\n"
               + b"Content-Type: application/x-www-form-urlencoded\r\n"
               + b"Content-Length: " + str(len(body)).encode("ascii")
               + b"\r\nConnection: close\r\n\r\n" + body)
    if barrier:
        barrier.wait()
    response = http_request(process, request)
    if response.startswith(b"HTTP/1.1 302"):
        cookie = next((line.split(b";", 1)[0] for line in response.split(b"\r\n")
                       if line.startswith(b"Set-Cookie: session_id=")), None)
        assert cookie, response
        return True, cookie.removeprefix(b"Set-Cookie: ")
    assert response.startswith(b"HTTP/1.1 200") and b"User already logged in." in response, response
    return False, None


def logout(process, cookie):
    response = http_request(process, b"GET /logout HTTP/1.1\r\nHost: localhost\r\nCookie: "
                            + cookie + b"\r\nConnection: close\r\n\r\n")
    assert response.startswith(b"HTTP/1.1 302"), response


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 tests/cross_interface_login_test.py ./server")
    binary = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parents[1]
    process = subprocess.Popen([str(binary)], cwd=root / "src",
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        with connect_when_ready(process, 8080), connect_when_ready(process, 8081):
            pass

        accepted, tcp = tcp_login(process, "tcp_first")
        assert accepted
        try:
            assert http_login(process, "tcp_first") == (False, None)
        finally:
            tcp.close()

        accepted, cookie = http_login(process, "web_first")
        assert accepted
        try:
            assert tcp_login(process, "web_first") == (False, None)
        finally:
            logout(process, cookie)

        with ThreadPoolExecutor(max_workers=2) as pool:
            for attempt in range(12):
                username = f"race_{attempt}"
                barrier = threading.Barrier(3, timeout=3)
                tcp_future = pool.submit(tcp_login, process, username, barrier)
                http_future = pool.submit(http_login, process, username, barrier)
                barrier.wait()
                tcp_accepted, tcp = tcp_future.result(timeout=5)
                http_accepted, cookie = http_future.result(timeout=5)
                try:
                    assert tcp_accepted != http_accepted, (username, tcp_accepted, http_accepted)
                finally:
                    if tcp:
                        tcp.close()
                    if cookie:
                        logout(process, cookie)
        print("cross-interface login: two sequential cases and 12 races passed")
    finally:
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == "__main__":
    main()
