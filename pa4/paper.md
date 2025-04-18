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
- fully associative, FIFO replacement
- fully associative, LRU replacement

The first test compared the effect of various cache sizes,
using a block size of 64 bytes:

- 2048 bytes (2KB)
- 4096 bytes (4KB)
- 8192 bytes (8KB)
- 16384 bytes (16KB)
- 32768 bytes (32KB)

The second test compared the effect of various block sizes,
using a cache size of 16384 bytes (16KB):

- 16 bytes
- 32 bytes
- 64 bytes
- 128 bytes
- 256 bytes

These parameter values were chosen because they span a range of values that
seem to be common among existing cache designs.

## Results

![Cache size plot](build/cache-size-plot.svg)

![Block size plot](build/block-size-plot.svg)

## Conclusions

In general, increasing set sizes lead to greater hit rates, with fully
associative designs resulting in the highest hit rates and
direct-mapped designs resulting in the lowest hit rates.
Designs with a LRU replacement policy generally resulted in greater hit rates
than designs with FIFO replacement.

Increasing cache size always results in greater hit rates.
Increasing block size leads to increased hit rates only up to a point,
after which the decreased number of lines outweights the effect of more
addresses being available in a single line.
