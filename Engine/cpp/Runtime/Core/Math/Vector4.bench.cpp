// Vector4.bench.cpp

// Currently tests just the Vector4::dot, simd vs scalar.
// This tests throughput, dependency-chain latency, compare/count.
// This is mostly just for an example of how to use it until we
// expand on it a fair amount more.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <cstddef>
#include <cstdio>
#include <random>
#include <vector>

import core.math;

namespace {

constexpr auto scalar_dot = [](const draco::math::Vector4& x, const draco::math::Vector4& y) {
    return x.x * y.x + x.y * y.y + x.z * y.z + x.w * y.w;
};

constexpr auto simd_dot = [](const draco::math::Vector4& x, const draco::math::Vector4& y) {
    return draco::math::dot(x, y);
};

// Build config output:
void print_metadata() {
#if defined(__clang__)
    std::printf("# compiler : clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    std::printf("# compiler : gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    std::printf("# compiler : msvc %d\n", _MSC_VER);
#endif

#if defined(__FAST_MATH__)
    std::printf("# fast-math: on\n");
#else
    std::printf("# fast-math: off\n");
#endif

    std::printf("# sizeof(Vector4) = %zu\n", sizeof(draco::math::Vector4));
    std::printf("# alignof(Vector4) = %zu\n", alignof(draco::math::Vector4));
}

// 4 accumulators over the first n elements (n % 4 == 0) so nothing serializes.
void throughput4(ankerl::nanobench::Bench& bench, const char* name, auto dot, const std::vector<draco::math::Vector4>& a, const std::vector<draco::math::Vector4>& b, std::size_t n) {
    bench.run(name, [&, n] {
        draco::f32 acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
        for (std::size_t i = 0; i < n; i += 4) {
            acc0 += dot(a[i + 0], b[i + 0]);
            acc1 += dot(a[i + 1], b[i + 1]);
            acc2 += dot(a[i + 2], b[i + 2]);
            acc3 += dot(a[i + 3], b[i + 3]);
        }
        ankerl::nanobench::doNotOptimizeAway(acc0 + acc1 + acc2 + acc3);
    });
}

void benchmark_dot() {
    constexpr std::size_t COUNT = 1024;
    static_assert(COUNT % 4 == 0, "throughput loop is unrolled by 4");

    std::mt19937 rng(0xDEADBABE); // ebin
    std::uniform_real_distribution<draco::f32> dist(-1.0f, 1.0f);

    const auto rnd = [&] { return dist(rng); };

    std::vector<draco::math::Vector4> a(COUNT), b(COUNT);

    for (std::size_t i = 0; i < COUNT; ++i) {
        a[i] = { rnd(), rnd(), rnd(), rnd() };
        b[i] = { rnd(), rnd(), rnd(), rnd() };
    }

    const auto configure = [&](ankerl::nanobench::Bench& bench, const char* title) {
        bench
            .title(title)
            .relative(true)
            .unit("dot")
            .batch(COUNT)
            .epochs(20) // tigher median.
            .epochIterations(10000); // force reproduciblility across runs.
    };

    // Throughput: independent dots, peak case.
    ankerl::nanobench::Bench throughputBench;
    configure(throughputBench, "dot throughput");
    throughput4(throughputBench, "scalar", scalar_dot, a, b, COUNT);
    throughput4(throughputBench, "simd", simd_dot, a, b, COUNT);

    // Latency: each dot result feeds the next dot input, so calls don't overlap.
    // Includes scalar-to-vector feedback overhead.
    ankerl::nanobench::Bench chainBench;
    configure(chainBench, "dot latency chain");

    const auto latencyChain = [&](const char* name, auto dot) {
        chainBench.run(name, [&] {
            draco::math::Vector4 v = a[0];
            draco::f32 s = 0.25f;

            for (std::size_t i = 0; i < COUNT; ++i) {
                v = { s, s, s, s };
                s = dot(v, b[i]);
            }

            ankerl::nanobench::doNotOptimizeAway(s);
            ankerl::nanobench::doNotOptimizeAway(v);
        });
    };

    latencyChain("scalar", scalar_dot);
    latencyChain("simd", simd_dot);

    // Compare/count: dot feeds a threshold test (culling/visibility). May
    // compile branchlessly, so it is compare/count cost, not branch prediction.
    ankerl::nanobench::Bench compareBench;
    configure(compareBench, "dot compare/count");

    const auto compareCount = [&](const char* name, auto dot) {
        compareBench.run(name, [&] {
            int hits = 0;

            for (std::size_t i = 0; i < COUNT; ++i) {
                if (dot(a[i], b[i]) > 0.0f) ++hits;
            }

            ankerl::nanobench::doNotOptimizeAway(hits);
        });
    };

    compareCount("scalar", scalar_dot);
    compareCount("simd", simd_dot);
}

} // namespace

int main() {
    print_metadata();
    benchmark_dot();
}
