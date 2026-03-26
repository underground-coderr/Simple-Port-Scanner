# Simple Port Scanner

A fast, concurrent TCP port scanner written in C++ from scratch — no external
libraries, no nmap, no shortcuts. Built to understand how port scanning actually
works under the hood: raw sockets, non-blocking I/O, thread pools, and service
fingerprinting.

---

## Why I built this

Most people use nmap and never think about what happens underneath. The goal of
this project was to build a port scanner the hard way — managing threads manually,
handling socket timeouts without blocking, and grabbing raw banners directly from
services. It is a learning project as much as it is a working tool.

---

## What it does

- Spins up a thread pool and scans multiple ports simultaneously instead of one
  by one — scanning 1024 ports with 100 threads is dramatically faster than doing
  it sequentially
- Connects to each port using a non-blocking socket with a configurable timeout
  so filtered ports don't slow everything down
- Classifies every port as open, closed, or filtered based on what the OS reports
  back
- Once a port is confirmed open, optionally connects again and tries to read
  whatever the service announces — SSH version strings, HTTP server headers,
  FTP welcome messages etc.
- Looks up well-known port numbers and labels them (port 22 = ssh, port 80 =
  http, and so on)
- Prints everything in a clean table when the scan finishes

---

## How it works — the short version
```
CLI args → ScanConfig
               ↓
          PortScanner
               ↓
          ThreadPool  ← N worker threads pulling from a task queue
               ↓
          probe_port  ← non-blocking connect + select() for timeout
               ↓
         grab_banner  ← if open, reconnect and read what the service says
               ↓
          results[]   ← sorted by port, printed as a table
```

The non-blocking connect is the most important trick. We set the socket to
non-blocking mode, call connect() which returns immediately with EINPROGRESS,
then use select() to wait up to our timeout. If select() times out the port is
filtered. If the socket becomes writable we check SO_ERROR — zero means open,
ECONNREFUSED means closed.

---

## Project structure
```
Simple Port Scanner/
  src/
    common.hpp        — all shared types: PortStatus enum, ScanResult struct,
                        ScanConfig struct, and the port → service name table
    thread_pool.hpp   — a simple but real thread pool: a task queue protected
                        by a mutex, N worker threads sleeping on a condition
                        variable, woken up one at a time when work arrives
    probe.hpp         — the actual TCP probe logic: creates a non-blocking
                        socket, attempts connect, uses select() to implement
                        the timeout, returns OPEN / CLOSED / FILTERED
    banner.hpp        — once a port is open, reconnects and reads the raw text
                        the service sends back, with per-service probe strings
                        for services that need a nudge (like HTTP)
    scanner.hpp       — the coordinator: resolves the hostname once, feeds
                        ports into the thread pool, collects results, shows
                        a live progress counter while scanning
    main.cpp          — CLI argument parsing, kicks off the scan, prints the
                        final results table
 
```

---

## Building

**Windows (MinGW / MSYS2)**
```bash
g++ -std=c++17 -O2 -Wall -pthread -o scanner src/main.cpp -lws2_32
```

**Linux / macOS**
```bash
g++ -std=c++17 -O2 -Wall -pthread -o scanner src/main.cpp
```

You need g++ with C++17 support. No other dependencies.

---

## Usage
```
scanner [options] <host>

options:
  -p <start>-<end>   port range              (default: 1-1024)
  -t <n>             number of threads        (default: 100)
  -T <ms>            connect timeout in ms    (default: 2000)
  -b                 grab banners             (default: on)
  -B                 skip banner grabbing
  -c                 also show closed ports
  -f                 also show filtered ports
  -h                 show this help
```

---

## Examples
```bash
# scan default range 1-1024 on a host
./scanner scanme.nmap.org

# scan every possible port with 200 threads
./scanner -p 1-65535 -t 200 scanme.nmap.org

# quick scan with short timeout and no banner grabbing
./scanner -p 1-1024 -B -T 500 192.168.1.1

# scan a specific range
./scanner -p 8000-9000 192.168.1.1

# show absolutely everything including closed and filtered ports
./scanner -p 1-1024 -c -f scanme.nmap.org

# scan common web ports only
./scanner -p 80-443 scanme.nmap.org
```

---

## Example output
```
scanning scanme.nmap.org (45.33.32.156)  ports 1-1024  threads 100

  progress: 1024/1024  (100%)

port    status      service         banner
------------------------------------------------------------------------
22      open        ssh             SSH-2.0-OpenSSH_6.6.1p1 Ubuntu-2ubuntu2.13
80      open        http            HTTP/1.0 200 OK
------------------------------------------------------------------------
2 open port(s) found
```

---

## Things to know

**Threads vs speed** — more threads means faster scans but also more load on
your machine and more noise on the network. 100 is a safe default. 500+ is fine
for local networks. Going too high on the internet can cause your OS to run out
of available sockets.

**Timeouts** — the default 2000ms timeout is conservative. On a local network
you can drop it to 200-500ms and the scan will be much faster. On the internet
keep it at 1000ms or higher or you will get false filtered results for slow hosts.

**Banner grabbing** — not every open port will return a banner. Some services
only respond after you send a valid request. Some require TLS. Some just stay
silent. A missing banner does not mean the port is not open.

**Windows note** — uses Winsock2 on Windows instead of POSIX sockets. The
behaviour is identical but the socket API calls are slightly different under
the hood. The `-lws2_32` linker flag is required on Windows.

---

## Legal

Only scan hosts you own or have explicit written permission to scan.
Unauthorized port scanning is illegal in many countries regardless of intent.

`scanme.nmap.org` is a host run by the nmap project specifically for people
to legally test scanners against. Use it freely.

---

## Author

**Rehan Khan**
Built as a 6th semester Compiler Construction course project.

---

© 2026 Rehan Khan. All rights reserved.
Unauthorized copying, distribution, or modification of this project without explicit permission is prohibited.
