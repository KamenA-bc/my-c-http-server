[![progress-banner](https://backend.codecrafters.io/progress/http-server/ca9d6868-7213-480b-9757-d5a9e4b526da)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

This is a starting point for C solutions to the
["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview).

[HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) is the
protocol that powers the web. In this challenge, you'll build a HTTP/1.1 server
that is capable of serving multiple clients.

Along the way you'll learn about TCP servers,
[HTTP request syntax](https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html),
and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.


Core Features

    Full Concurrency: Utilizes POSIX Threads (pthreads) in a Master-Worker architecture to handle thousands of simultaneous client connections without blocking the main event loop.

    Manual Protocol Parsing: Dynamically parses raw HTTP/1.1 requests, extracting HTTP methods, URL paths, and specific headers (like User-Agent) using safe string manipulation.

    Binary File Transmission: Safely serves both human-readable text and raw binary payloads (like images or compiled code) from the hard drive, utilizing dynamic memory allocation (malloc/free) and memcpy to bypass standard C-string null-byte truncation traps.

    Memory Safety: Rigorously manages Stack vs. Heap memory lifecycles, ensuring zero memory leaks, preventing dangling pointers, and autonomously cleaning up detached zombie threads.

Architecture

1. The Dispatcher (Main Thread)
The server runs an infinite event loop using standard POSIX sockets (socket, bind, listen, accept). When a new client connects, the dispatcher dynamically allocates memory on the Heap for the client's file descriptor and hands it off to a new detached worker thread, instantly returning to listen for the next connection.

2. The Worker (Thread Pool)
Each worker thread safely unpacks the client's socket descriptor, frees the intermediate Heap memory to prevent leaks, and takes over the connection.

3. The Unified Output Pipeline
To maximize efficiency and respect the DRY (Don't Repeat Yourself) principle, the routing logic prepares either Stack-based text responses or Heap-based binary payloads. A generic pointer safely redirects the final data to a single, unified send() call before the thread cleans up its memory and terminates.
