# Tiny-Muduo: A Light-weight C++ Network Library

multi-threaded C++ network library based on the non-blocking Reactor pattern. This project is a simplified implementation of the  library, designed to handle high-concurrency network I/O through a decoupled, event-driven architecture.

## Core Architecture: Multi-Reactor / Multi-Acceptor

The library follows the "One Loop Per Thread" model using a Multi-Reactor design:

1. Multi-Acceptor with `SO_REUSEPORT`: When `TcpServer` is created with `kReusePort`, every `EventLoop` owns its own `Acceptor` and listens on the same port. The kernel distributes new connections across loops directly, so there is no dedicated accept-only main reactor.

2. Compatibility Mode: When `kReusePort` is not enabled, the library falls back to a single acceptor on the base loop and dispatches accepted sockets to worker loops with round-robin.

3. Event Notification: Uses Linux `eventfd` for efficient cross-thread wakeup when work must be queued into another loop.

## Key Features
+ Modern C++: Extensive use of std::shared_ptr/unique_ptr for memory management, and std::function/bind for callback-based event handling.

+ Epoll Abstraction: Encapsulated epoll in a Poller class, providing Level-Triggered (LT) event notification.

+ Non-blocking IO: All IO operations are non-blocking, ensuring the library never stalls on a single slow connection.

+ Buffer Management: Implementation of a dynamic Buffer class with a 8-byte prependable zone to solve the TCP sticky packet and half-packet problems.

+ RAII Support: Socket and File Descriptor lifecycles are strictly managed via RAII to prevent resource leaks.

+ High Performance: Optimized with __thread (Thread Local Storage) to cache TIDs, reducing the frequency of expensive system calls.

## Reuseport Quick Check

Build the example server and then start it:

```bash
g++ -std=c++11 -I. test/test.cc -Llib -lmymuduo -pthread -o build/echo_server
LD_LIBRARY_PATH=./lib ./build/echo_server
```

In another terminal, create a burst of connections:

```bash
python3 test/reuseport_stress.py --connections 64 --hold 2
```

The example server logs the Linux thread id for each accepted connection, which makes it easy to verify whether connections are being distributed across loops.

You can also run a quick local comparison:

```bash
python3 test/compare_reuseport.py --connections 128 --hold 0.2 --rounds 5
```

This starts the same echo server twice, once with `kNoReusePort` and once with `kReusePort`, then reports per-round elapsed time for the same burst workload.

For an HTTP throughput comparison against nginx on a 12-core machine:

```bash
g++ -std=c++11 -I. test/http_bench.cc -Llib -lmymuduo -pthread -o build/http_bench_server
python3 test/compare_with_nginx.py --threads 12 --connections 500 --duration 10s --mode reuseport
```

This uses `wrk` to benchmark a tiny keep-alive HTTP server built on this project against a local nginx instance serving the same `hello world` payload.
