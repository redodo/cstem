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
