#!/usr/bin/env python3

import argparse
import math
import os
import re
import signal
import subprocess
import sys
import tempfile
import time


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB_DIR = os.path.join(ROOT, "lib")
BENCH_SERVER_BIN = os.path.join(ROOT, "build", "http_bench_server")
BENCH_DOCROOT = os.path.join(ROOT, "build", "bench_www")
BENCH_FILE = os.path.join(BENCH_DOCROOT, "index.html")


def build_bench_assets() -> None:
    os.makedirs(BENCH_DOCROOT, exist_ok=True)
    with open(BENCH_FILE, "w", encoding="utf-8") as file:
        file.write("hello world\n")


def wait_for_process(proc: subprocess.Popen[str], timeout_seconds: float = 3.0) -> None:
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"process exited early with code {proc.returncode}")
        time.sleep(0.1)


def parse_wrk_output(output: str) -> dict:
    requests_match = re.search(r"Requests/sec:\s+([0-9.]+)", output)
    transfer_match = re.search(r"Transfer/sec:\s+([0-9.]+\w+)", output)
    latency_match = re.search(r"Latency\s+([0-9.]+)(us|ms|s)", output)
    if requests_match is None:
        raise RuntimeError(f"unable to parse wrk output:\n{output}")
    latency_us = None
    latency_display = "n/a"
    if latency_match:
        latency_value = float(latency_match.group(1))
        latency_unit = latency_match.group(2)
        latency_display = f"{latency_value}{latency_unit}"
        if latency_unit == "us":
            latency_us = latency_value
        elif latency_unit == "ms":
            latency_us = latency_value * 1000.0
        else:
            latency_us = latency_value * 1_000_000.0
    return {
        "requests_per_sec": float(requests_match.group(1)),
        "transfer_per_sec": transfer_match.group(1) if transfer_match else "n/a",
        "latency": latency_display,
        "latency_us": latency_us,
        "raw": output,
    }


def run_wrk(url: str, threads: int, connections: int, duration: str) -> dict:
    result = subprocess.run(
        [
            "wrk",
            f"-t{threads}",
            f"-c{connections}",
            f"-d{duration}",
            "--latency",
            url,
        ],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return parse_wrk_output(result.stdout)


def start_mymuduo_server(mode: str, port: int, threads: int) -> subprocess.Popen[str]:
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = LIB_DIR
    return subprocess.Popen(
        [BENCH_SERVER_BIN, mode, str(port), str(threads)],
        cwd=ROOT,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        text=True,
        preexec_fn=os.setsid,
    )


def write_nginx_conf(port: int, workers: int) -> str:
    conf = f"""
worker_processes {workers};
pid {os.path.join(ROOT, 'build', 'nginx-bench.pid')};
events {{
    worker_connections 16384;
    multi_accept on;
    accept_mutex off;
}}
http {{
    access_log off;
    error_log {os.path.join(ROOT, 'build', 'nginx-bench-error.log')} warn;
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 75;
    keepalive_requests 100000;
    sendfile_max_chunk 512k;
    server_tokens off;
    default_type text/plain;
    server {{
        listen 127.0.0.1:{port} reuseport backlog=65535;
        server_name localhost;
        location / {{
            root {BENCH_DOCROOT};
            index index.html;
            add_header Connection keep-alive always;
        }}
    }}
}}
"""
    temp = tempfile.NamedTemporaryFile(
        mode="w", suffix=".conf", prefix="nginx-bench-", dir=os.path.join(ROOT, "build"), delete=False
    )
    temp.write(conf)
    temp.close()
    return temp.name


def start_nginx(conf_path: str) -> subprocess.Popen[str]:
    return subprocess.Popen(
        ["nginx", "-c", conf_path, "-p", ROOT, "-g", "daemon off;"],
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        text=True,
        preexec_fn=os.setsid,
    )


def stop_process_group(proc: subprocess.Popen[str], sig: int = signal.SIGINT) -> None:
    os.killpg(os.getpgid(proc.pid), sig)
    proc.wait(timeout=3)


def benchmark_mymuduo(mode: str, port: int, threads: int, connections: int, duration: str) -> dict:
    proc = start_mymuduo_server(mode, port, threads)
    try:
        wait_for_process(proc)
        result = run_wrk(f"http://127.0.0.1:{port}/", threads, connections, duration)
        result["target"] = f"mymuduo-{mode}"
        return result
    finally:
        stop_process_group(proc)


def benchmark_nginx(port: int, workers: int, wrk_threads: int, connections: int, duration: str) -> dict:
    conf_path = write_nginx_conf(port, workers)
    proc = start_nginx(conf_path)
    try:
        wait_for_process(proc)
        result = run_wrk(f"http://127.0.0.1:{port}/", wrk_threads, connections, duration)
        result["target"] = "nginx"
        return result
    finally:
        stop_process_group(proc, signal.SIGTERM)
        os.unlink(conf_path)


def aggregate_results(target: str, rounds: list[dict]) -> dict:
    req_values = [round_result["requests_per_sec"] for round_result in rounds]
    latency_values = [round_result["latency_us"] for round_result in rounds if round_result["latency_us"] is not None]
    avg_req = sum(req_values) / len(req_values)
    avg_latency_us = sum(latency_values) / len(latency_values) if latency_values else None
    variance = sum((value - avg_req) ** 2 for value in req_values) / len(req_values)
    return {
        "target": target,
        "avg_req": avg_req,
        "best_req": max(req_values),
        "worst_req": min(req_values),
        "stddev_req": math.sqrt(variance),
        "avg_latency_us": avg_latency_us,
        "rounds": rounds,
        "transfer_per_sec": rounds[-1]["transfer_per_sec"],
    }


def format_latency(us_value: float | None) -> str:
    if us_value is None:
        return "n/a"
    if us_value >= 1000.0:
        return f"{us_value / 1000.0:.2f}ms"
    return f"{us_value:.2f}us"


def run_target(target: str, port: int, threads: int, connections: int, duration: str, rounds: int) -> dict:
    round_results = []
    for _ in range(rounds):
        if target == "nginx":
            round_results.append(benchmark_nginx(port, threads, threads, connections, duration))
        elif target == "mymuduo-reuseport":
            round_results.append(benchmark_mymuduo("reuseport", port, threads, connections, duration))
        else:
            round_results.append(benchmark_mymuduo("noreuseport", port, threads, connections, duration))
    return aggregate_results(target, round_results)


def print_markdown_table(results: list[dict], nginx_req: float) -> None:
    print("| Target | Avg req/s | Best req/s | Worst req/s | Stddev req/s | Avg latency | Relative to nginx |")
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
    for result in results:
        print(
            f"| {result['target']} | {result['avg_req']:.2f} | {result['best_req']:.2f} | "
            f"{result['worst_req']:.2f} | {result['stddev_req']:.2f} | "
            f"{format_latency(result['avg_latency_us'])} | {result['avg_req'] / nginx_req:.4f}x |"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare mymuduo HTTP throughput with nginx using wrk.")
    parser.add_argument("--threads", type=int, default=12)
    parser.add_argument("--connections", type=int, default=2000)
    parser.add_argument("--duration", default="15s")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--mymuduo-port", type=int, default=21080)
    parser.add_argument("--nginx-port", type=int, default=21082)
    args = parser.parse_args()

    build_bench_assets()

    results = [
        run_target("mymuduo-reuseport", args.mymuduo_port, args.threads, args.connections, args.duration, args.rounds),
        run_target("mymuduo-noreuseport", args.mymuduo_port + 1, args.threads, args.connections, args.duration, args.rounds),
        run_target("nginx", args.nginx_port, args.threads, args.connections, args.duration, args.rounds),
    ]

    print(
        f"wrk_threads={args.threads} connections={args.connections} duration={args.duration} rounds={args.rounds}"
    )
    print("Response shape alignment:")
    print("- Body: hello world\\n")
    print("- Content-Type: text/plain")
    print("- Content-Length: 12")
    print("- Connection: keep-alive")
    print("- Note: nginx will still emit its own built-in Date/Server headers.")
    print_markdown_table(results, results[2]["avg_req"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
