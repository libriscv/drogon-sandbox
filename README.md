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
    Latency     9.32us    2.44us 235.00us   91.45%
    Req/Sec   102.69k    15.58k  123.47k    84.41%
  Latency Distribution
     50%    9.00us
     75%   10.00us
     90%   11.00us
     99%   18.00us
  8255685 requests in 10.10s, 1.65GB read
Requests/sec: 817438.99
Transfer/sec:    166.83MB

$ ./wrk -c16 -t16 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  16 threads and 16 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    11.96us    3.40us 577.00us   82.96%
    Req/Sec    80.60k    14.62k  112.99k    69.37%
  Latency Distribution
     50%   11.00us
     75%   13.00us
     90%   17.00us
     99%   21.00us
  12959120 requests in 10.10s, 2.58GB read
Requests/sec: 1283098.97
Transfer/sec:    261.86MB

$ ./wrk -c32 -t32 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  32 threads and 32 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    30.10us  103.81us   3.99ms   98.47%
    Req/Sec    48.79k    12.17k   70.01k    63.33%
  Latency Distribution
     50%   17.00us
     75%   24.00us
     90%   32.00us
     99%  316.00us
  15675777 requests in 10.10s, 3.12GB read
Requests/sec: 1552154.56
Transfer/sec:    316.77MB

$ ./wrk -c64 -t64 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  64 threads and 64 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    51.66us  282.19us  14.30ms   98.78%
    Req/Sec    30.96k    12.06k   83.88k    76.31%
  Latency Distribution
     50%   28.00us
     75%   42.00us
     90%   60.00us
     99%  452.00us
  19893994 requests in 10.10s, 3.96GB read
Requests/sec: 1969675.07
Transfer/sec:    401.98MB
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
