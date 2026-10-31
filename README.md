# Limit Order Book Engine

## Build and test

For regular correctness checks, configure a Debug build with the optional
full-queue invariant scan enabled:

```powershell
cmake -G "MinGW Makefiles" -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DLOB_PARANOID=ON
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

For benchmarks, use a Release build so assertions and debug instrumentation
are not included:

```powershell
cmake -G "MinGW Makefiles" -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

`LOB_PARANOID` is off by default. When enabled, each `PriceLevel` mutation
recomputes the queue volume in O(n); it is intended for small correctness runs,
not benchmarks.

## OrderBook skeleton

`OrderBook` is initialized with a first price and a number of consecutive price
levels. It creates separate bid and ask levels and assigns the price to every
level at construction. Matching and order-management operations are not yet
implemented.
