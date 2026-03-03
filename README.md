# Tiny-Muduo: A Light-weight C++ Network Library

multi-threaded C++ network library based on the non-blocking Reactor pattern. This project is a simplified implementation of the  library, designed to handle high-concurrency network I/O through a decoupled, event-driven architecture.

## Core Architecture: Multi-Reactor

The library follows the "One Loop Per Thread" model using a Multi-Reactor design:

1. Main-Reactor (Acceptor): Resides in the main thread, responsible only for accepting new connections.

2. Sub-Reactors (EventLoops): A thread pool of IO loops. Once a new connection is accepted, the Main-Reactor dispatches it to a Sub-Reactor using a Round-Robin algorithm.

3. Event Notification: Uses Linux eventfd for efficient cross-thread wakeup, allowing the Main-Loop to notify Sub-Loops of new tasks without the overhead of pipes or sockets.

## Key Features
+ Modern C++: Extensive use of std::shared_ptr/unique_ptr for memory management, and std::function/bind for callback-based event handling.

+ Epoll Abstraction: Encapsulated epoll in a Poller class, providing Level-Triggered (LT) event notification.

+ Non-blocking IO: All IO operations are non-blocking, ensuring the library never stalls on a single slow connection.

+ Buffer Management: Implementation of a dynamic Buffer class with a 8-byte prependable zone to solve the TCP sticky packet and half-packet problems.

+ RAII Support: Socket and File Descriptor lifecycles are strictly managed via RAII to prevent resource leaks.

+ High Performance: Optimized with __thread (Thread Local Storage) to cache TIDs, reducing the frequency of expensive system calls.
