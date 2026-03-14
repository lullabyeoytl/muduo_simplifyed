#!/usr/bin/env python3

import argparse
import socket
import threading
import time


def hold_connection(host: str, port: int, hold_seconds: float, payload: bytes, results: list, index: int) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(3.0)
    try:
        sock.connect((host, port))
        if payload:
            sock.sendall(payload)
            sock.recv(len(payload))
        time.sleep(hold_seconds)
        results[index] = "ok"
    except Exception as exc:  # noqa: BLE001
        results[index] = f"error: {exc}"
    finally:
        sock.close()


def main() -> int:
    parser = argparse.ArgumentParser(description="Create many concurrent TCP connections to observe SO_REUSEPORT distribution.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=2007)
    parser.add_argument("--connections", type=int, default=64)
    parser.add_argument("--hold", type=float, default=2.0, help="Seconds to keep each connection open.")
    parser.add_argument("--payload", default="ping", help="Optional payload to echo after connect.")
    args = parser.parse_args()

    payload = args.payload.encode() if args.payload else b""
    results = ["pending"] * args.connections
    threads = []

    start = time.time()
    for index in range(args.connections):
        thread = threading.Thread(
            target=hold_connection,
            args=(args.host, args.port, args.hold, payload, results, index),
            daemon=True,
        )
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

    ok_count = sum(result == "ok" for result in results)
    error_results = [result for result in results if result != "ok"]
    duration = time.time() - start

    print(f"completed={args.connections} ok={ok_count} errors={len(error_results)} duration={duration:.2f}s")
    for error in error_results[:10]:
        print(error)
    return 0 if not error_results else 1


if __name__ == "__main__":
    raise SystemExit(main())
