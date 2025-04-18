# Cache Analysis Paper

<https://github.com/inflmts/cda3101/tree/main/pa4>

The goal of our analysis is to evaluate how different cache designs affect the
cache hit rate for memory accesses during the execution of a real program.
Increased hit rate is beneficial because the access speed for data in cache
is much greater than the access speed for data in physical memory.

## Tests

All tests use the `gcc` trace file.

The first two plots compare the effect of these associativities and replacement
strategies:

- Direct-mapped (always replaced)
- 2-way set associative, FIFO replacement
- 2-way set associative, LRU replacement
- 4-way set associative, FIFO replacement
- 4-way set associative, LRU replacement
- 8-way set associative, FIFO replacement
- 8-way set associative, LRU replacement
- fully associative, FIFO replacement
- fully associative, LRU replacement

The first plot compares the effect of various cache sizes,
using a block size of 64 bytes:

- 2048 bytes (2KB)
- 4096 bytes (4KB)
- 8192 bytes (8KB)
- 16384 bytes (16KB)
- 32768 bytes (32KB)

The second plot compares the effect of various block sizes,
using a cache size of 16384 bytes (16KB):

- 16 bytes
- 32 bytes
- 64 bytes
- 128 bytes
- 256 bytes

These parameter values were chosen because they span a range of values that
seem to be common among existing cache designs.

Additionally, two cache designs from real devices are compared:

- **Columbia supercomputer** (L1): 16KB, 4-way set associative, 64-byte blocks
- **Casio fx-CG50 calculator** (L1): 32KB, 4-way set associative, 32-byte blocks

## Results

**Comparing cache sizes:** `-P cachesize`

![Cache size plot](build/cache-size-plot.svg)

**Comparing block sizes:** `-P blocksize`

![Block size plot](build/block-size-plot.svg)

**Columbia supercomputer**: `-c 14 -s 2 -b 6 -l`

- Hit rate: 0.987773 (509378/515683)

**Casio fx-CG50**: `-c 15 -s 2 -b 5 -l`

- Hit rate: 0.984863 (507877/515683)

## Conclusions

In general, increasing set size led to greater hit rates, with fully
associative designs resulting in the highest performance and
direct-mapped designs resulting in the lowest performance.
This is expected because less conflicts will occur that require lines to be
replaced.

LRU replacement generally resulted in greater hit rates than FIFO replacement.

Increasing cache size always led to increased hit rate. More lines in the cache
leads to less conflicts.

Increasing block size led to increased hit rates only up to a point,
after which the decreased number of total lines outweighed the greater number of
addresses available in a single line.

The hit rate for the supercomputer cache design was greater than the calculator
cache design (a difference of approximately 0.3%). The increased block size
appears to have outweighed the effect of a smaller cache.
