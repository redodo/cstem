# cstem: bloomon code challenge using SIMD in C

This program uses 256-bit SIMD vectors to efficiently solve the bloomon code challenge.
It uses the same algorithm as [redodo/ranger](https://github.com/redodo/ranger) but is
written in C instead of Rust to reduce overhead.

## How to run it

Please see the commands below for the various ways of running cstem:

| Commands           | Description                                      |
|--------------------|--------------------------------------------------|
| `make run`         | Compile and run cstem                            |
| `make test`        | Compile and test against code challenge verifier |
| `make benchmark`   | Compile and run benchmark                        |
| `make build`       | Compile release build                            |

| Debug commands     | Description                               |
|--------------------|-------------------------------------------|
| `make build-debug` | Compile debug build                       |
| `make dump`        | Compile debug build and output objdump    |
| `make gdb`         | Compile debug build and run with gdb      |
| `make valgrind`    | Compile debug build and run with valgrind |

## Benchmark

This benchmark was run on a **Lenovo ThinkPad X1 Carbon Gen 8** with an
**Intel Core i7-10610U**:

```
multitime -n100 -q -s0 -i "cat samples/1m.txt" ./cstem
===> multitime results
1: -i "cat samples/1m.txt" -q ./cstem
            Mean        Std.Dev.    Min         Median      Max
real        0.058       0.001       0.056       0.058       0.065
user        0.057       0.002       0.051       0.058       0.064
sys         0.001       0.002       0.000       0.000       0.006
```
