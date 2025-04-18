# Cache Analysis Paper

The goal of our analysis is to evaluate how different cache designs affect the
cache hit rate for memory accesses during the execution of real programs.
Increased hit rate is beneficial because the access speed for data in cache
is much greater than the access speed for data in physical memory.

## Tests

These associativities and replacement strategies were compared:

- Direct-mapped (always replaced)
- 2-way set associative, FIFO replacement
- 2-way set associative, LRU replacement
- 4-way set associative, FIFO replacement
- 4-way set associative, LRU replacement
- 8-way set associative, FIFO replacement
- 8-way set associative, LRU replacement

The first test compared the effect of various cache sizes:

- 1024 bytes (1KB)
- 2048 bytes (2KB)
- 4096 bytes (4KB)
- 8192 bytes (8KB)
- 16384 bytes (16KB)

The second test compared the effect of various block sizes:

- 4 bytes
- 8 bytes
- 16 bytes
- 32 bytes
- 64 bytes
- 128 bytes

These parameter values were chosen because they span a range of values that
seem to be common among existing cache designs.

## Results

![Cache size plot](build/cache-size-plot.svg)

![Block size plot](build/block-size-plot.svg)

## Conclusions

In general, increasing set sizes lead to greater hit rates, with fully
associative designs resulting in the highest hit rates and
direct-mapped designs resulting in the lowest hit rates.
LRU replacement generally results in greater hit rates than FIFO replacement.
