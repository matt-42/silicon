---
layout: post
title: Hello world benchmark
---

Hello world benchmark
===========================

This benchmark compares the performances of several frameworks on a
simple case: on each request, serializes the json object
```{"message":"hello world."}``` and sends it back to the http client.

While it is rather simple and does not reflect a real world
application, it gives a brief insight on the performances of the
framework.

The full specification of the benchmark are hosted by Techempower
(session JSON serialization):

[https://www.techempower.com/benchmarks/#section=code](https://www.techempower.com/benchmarks/#section=code)

The reference code of the nodejs express and go revel version can be
found on the git repository:

[https://github.com/TechEmpower/FrameworkBenchmarks](https://github.com/TechEmpower/FrameworkBenchmarks)


### Configuration

Intel quad-core i5 2500K 3GHz, 20GB RAM

The benchmark tool wrk was running on the same machine with the following arguments:

```bash
 wrk -d 2s -c ${C} -t 1 "__url__"
```
with C varying from 1 to 1000

### Results

![Hello World benchmark results](/assets/hello_world_benchmark.png)
![Hello World benchmark results](/assets/hello_world_benchmark_latency.png)

[Access to the raw benchmark log](/docs/hello_world_benchmark_log.txt)
