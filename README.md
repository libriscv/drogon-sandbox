# Drogon Sandbox

## Multi-tenancy

Multi-tenancy allows one server to be safely shared among many users, each of which cannot access each others or negatively affect the HTTP service.

## Design

Specialized sandboxes are instantiated for each request and immediately destroyed after the request, all within a single microsecond.

- [x] Ephemeral sandboxes gives hard guarantee that no request leaves traces in another
- [x] Hot-reloading of tenant programs avoiding service restart
- [x] Enforced memory- and CPU- limits

Note: This project is written like a production system, but contains only the necessary parts for realistic benchmarking. A real production system would have implemented a lot of observability, logging, metering etc.

## Benchmarks

Sandboxed 'Hello World' responses with 8, 32 and 64 threads.

```sh
$ ./wrk -c8 -t8 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  8 threads and 8 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     9.34us    2.86us 535.00us   92.37%
    Req/Sec   102.71k    10.52k  114.48k    87.25%
  Latency Distribution
     50%    9.00us
     75%   10.00us
     90%   11.00us
     99%   17.00us
  8257941 requests in 10.10s, 1.65GB read
Requests/sec: 817653.30
Transfer/sec:    166.87MB

$ ./wrk -c16 -t16 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  16 threads and 16 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    12.09us    3.69us 395.00us   79.30%
    Req/Sec    79.84k    10.44k  101.40k    69.43%
  Latency Distribution
     50%   11.00us
     75%   13.00us
     90%   17.00us
     99%   22.00us
  12839750 requests in 10.10s, 2.56GB read
Requests/sec: 1271316.75
Transfer/sec:    259.46MB

$ ./wrk -c32 -t32 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  32 threads and 32 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    19.82us   12.48us   3.02ms   93.20%
    Req/Sec    50.32k     9.68k   67.46k    59.62%
  Latency Distribution
     50%   17.00us
     75%   24.00us
     90%   30.00us
     99%   47.00us
  16184956 requests in 10.10s, 3.23GB read
Requests/sec: 1602566.73
Transfer/sec:    327.06MB

$ ./wrk -c64 -t64 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  64 threads and 64 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    38.25us  126.75us   8.85ms   99.10%
    Req/Sec    30.95k    13.14k   69.93k    73.36%
  Latency Distribution
     50%   29.00us
     75%   42.00us
     90%   61.00us
     99%  155.00us
  19907514 requests in 10.10s, 3.97GB read
Requests/sec: 1971164.72
Transfer/sec:    402.29MB
```

With 64 threads, the sandboxes handle ~2M req/s at an average of 51 micros/req.

## Drogon vanilla benchmarks

A simple Drogon hello world HTTP response, with no sandboxes involved:

```sh
$ ./wrk -c8 -t8 http://127.0.0.1:8080/ --latency
Running 10s test @ http://127.0.0.1:8080/
  8 threads and 8 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     8.54us    3.37us 534.00us   83.19%
    Req/Sec   111.76k    27.06k  146.04k    65.97%
  Latency Distribution
     50%    7.00us
     75%    9.00us
     90%   14.00us
     99%   17.00us
  8985045 requests in 10.10s, 1.79GB read
Requests/sec: 889613.44
Transfer/sec:    181.56MB
```

A vanilla Drogon response took 8.5 micros, while the sandboxed request required 9.3 micros. We can say that the total overhead of a fully integrated multi-tenancy solution is ~1 microsecond at 817k req/s.

## Pythran test program

The test program is a simple Pythran to C++ transpilation.

```py
def on_init():
	print("Hello from Python")

page = """	this is a very
	long string if I had the
	energy to type more and more ...
"""

def on_client_request():
	def generator():
		return "text/plain", page
	return generator
```
