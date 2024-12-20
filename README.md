# Multi-tenant Drogon

## Multi-tenancy

Multi-tenancy allows one server to be safely shared among many users, each of which cannot access each others or negatively affect the HTTP service.

## Design

Specialized sandboxes are instantiated for each request and immediately destroyed after the request ends in a ~microsecond.

- [x] Ephemeral requests guarantees no request may affect another
- [x] Hot-reloading of tenant programs avoiding service restart
- [x] All Drogon platforms are supported


Note: This project is still a work-in-progress.

## Benchmarks

Hello World multi-tenancy responses with 8, 32 and 64 threads.

```sh
$ ./wrk -c8 -t8 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  8 threads and 8 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     9.51us    3.08us   0.95ms   94.16%
    Req/Sec   100.91k    11.85k  119.43k    80.20%
  Latency Distribution
     50%    9.00us
     75%   10.00us
     90%   11.00us
     99%   17.00us
  8112849 requests in 10.10s, 1.62GB read
Requests/sec: 803251.42
Transfer/sec:    163.93MB

$ ./wrk -c16 -t16 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  16 threads and 16 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    13.66us    4.12us 530.00us   70.01%
    Req/Sec    70.82k    13.70k   96.97k    52.54%
  Latency Distribution
     50%   12.00us
     75%   16.00us
     90%   19.00us
     99%   23.00us
  11388772 requests in 10.10s, 2.27GB read
Requests/sec: 1127617.15
Transfer/sec:    230.13MB

$ ./wrk -c32 -t32 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  32 threads and 32 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    22.49us   20.93us   3.92ms   95.44%
    Req/Sec    45.34k    12.58k   87.98k    59.65%
  Latency Distribution
     50%   19.00us
     75%   26.00us
     90%   34.00us
     99%   59.00us
  14581049 requests in 10.10s, 2.91GB read
Requests/sec: 1443679.94
Transfer/sec:    294.64MB

$ ./wrk -c64 -t64 http://127.0.0.1:8080/z --latency
Running 10s test @ http://127.0.0.1:8080/z
  64 threads and 64 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    39.44us   36.99us   4.12ms   93.77%
    Req/Sec    27.27k     8.29k   67.72k    68.41%
  Latency Distribution
     50%   32.00us
     75%   47.00us
     90%   66.00us
     99%  156.00us
  17540692 requests in 10.10s, 3.50GB read
Requests/sec: 1736727.97
Transfer/sec:    354.44MB
```

With 64 threads, the sandboxes handle 1.7M req/s at an average of 39 micros/req.

## Drogon vanilla benchmarks

A simple Drogon hello world HTTP response

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

A vanilla Drogon response took 8.5 micros, while the sandboxed request required 9.5 micros. We can say that the total overhead of a fully integrated multi-tenancy solution is ~1 microsecond at 800k req/s. At 99% latency percentile, they both required 17 microseconds, however Drogon executed 10% more requests in the same amount of time.

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
