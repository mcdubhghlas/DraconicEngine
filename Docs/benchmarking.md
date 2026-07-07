# Benchmarking

As a major goal for draconic is performance, we need a way to benchmark
everything. Therefore, I have opted into adding nanobench in to help with this.

Simply add a `FILENAME.bench.cpp` next to the FILENAME.cpp that you are
testing

Here is a sample of how the benchmark runs:
```cpp
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

import core.math;

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("Vector4::dot").relative(true);

    // runtime value:
    float x = 1.0f;
    bench.run("dot", [&] {
        float r = /* call the thing */;
        ankerl::nanobench::doNotOptimizeAway(r);
    });
}
```

It can be built with this flag: `-DBUILD_BENCHMARKS=ON`

This builds all of the *.bench.cpp:
`cmake --build build/release --target benchmarks`

This will build and run them all in a sequence:
`cmake --build build/release --target run-benchmarks`


## Optional configurations

I need to note: Defaults are sane.

```cpp
ankerl::nanobench::Bench b;

// More samples, higher confidence.
// If err% is too high, increase the value.
// NOTE: It will increase the time cost.
b.eopchs(21); // default: 11

// Longer epochs provide steadier output.
// Edit this for fast operations (i.e. dot)
b.minEpochTime(std::chrono::milliseconds(10));

// Forces at least this amount of iterations per epoch.
// Use if you need more runs.
b.minEpochIterations(1'000'000);

// Fixed, exact iterations here.
// Use this for reproducibility. This is just for determinism.
b.epochIterations(100'000);

// Throwaway runs before the timer starts.
// Use this if it touches memory / has branches
b.warmup(100);
```
