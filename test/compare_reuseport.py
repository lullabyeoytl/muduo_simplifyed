#!/usr/bin/env python3

import argparse
import os
import signal
import subprocess
import sys
import time


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER_BIN = os.path.join(ROOT, "build", "echo_server")
STRESS_BIN = os.path.join(ROOT, "test", "reuseport_stress.py")


def wait_for_server(proc: subprocess.Popen[str], timeout_seconds: float = 3.0) -> None:
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"server exited early with code {proc.returncode}")
        time.sleep(0.1)


def run_stress(port: int, connections: int, hold: float, rounds: int) -> list[float]:
    durations = []
    for _ in range(rounds):
        start = time.perf_counter()
        subprocess.run(
            [
                sys.executable,
                STRESS_BIN,
                "--port",
                str(port),
                "--connections",
                str(connections),
                "--hold",
                str(hold),
                "--payload",
                "ping",
            ],
            check=True,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        durations.append(time.perf_counter() - start)
    return durations


def run_mode(mode: str, port: int, threads: int, connections: int, hold: float, rounds: int) -> dict:
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = os.path.join(ROOT, "lib")
    server = subprocess.Popen(
        [SERVER_BIN, mode, str(port), str(threads)],
        cwd=ROOT,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        text=True,
        preexec_fn=os.setsid,
    )
    try:
        wait_for_server(server)
        durations = run_stress(port, connections, hold, rounds)
        return {
            "mode": mode,
            "durations": durations,
            "avg": sum(durations) / len(durations),
            "min": min(durations),
            "max": max(durations),
        }
    finally:
        os.killpg(os.getpgid(server.pid), signal.SIGINT)
        server.wait(timeout=3)


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare kNoReusePort and kReusePort with the local echo server.")
    parser.add_argument("--port-base", type=int, default=22070)
    parser.add_argument("--threads", type=int, default=3)
    parser.add_argument("--connections", type=int, default=128)
    parser.add_argument("--hold", type=float, default=0.2)
    parser.add_argument("--rounds", type=int, default=5)
    args = parser.parse_args()

    results = [
        run_mode("noreuseport", args.port_base, args.threads, args.connections, args.hold, args.rounds),
        run_mode("reuseport", args.port_base + 1, args.threads, args.connections, args.hold, args.rounds),
    ]

    print(f"connections={args.connections} hold={args.hold}s rounds={args.rounds} threads={args.threads}")
    for result in results:
        print(
            f"{result['mode']}: avg={result['avg']:.4f}s min={result['min']:.4f}s max={result['max']:.4f}s "
            f"rounds={[round(value, 4) for value in result['durations']]}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
