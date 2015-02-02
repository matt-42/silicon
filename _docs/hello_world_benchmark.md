---
layout: post
---

Hello World Benchmark
===========================

This benchmark compares the performances of several frameworks on a simple example:
on each request, serializes the json object ```{"message":"hello world."}``` and sends it back
to the http client.

You can find the full specification of the benchmark here:

[https://www.techempower.com/benchmarks/#section=code](https://www.techempower.com/benchmarks/#section=code)

The reference code of the nodejs express and go revel version can be found on the
TechEmpower benchmark git repository:

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

[Access to the raw benchmark log](/docs/hello_world_raw_log.txt)
