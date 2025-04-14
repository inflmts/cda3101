# Cache Analysis

There are two programs in this directory:

- `conv` (`conv.c`) converts the trace files ([below](#important-information))
  into a more compact binary form. The format is a sequence of big-endian 4-byte
  unsigned integers, each representing a memory address. Because the type of access
  (load or store) and number of bytes requested are not used in this project,
  they are not included in the binary traces. Compiled versions of the Canvas
  trace files are available in the `trace` directory.
- `sim` (`sim.c`) performs the actual cache simulation on the compiled trace
  files. It cannot read the text trace files directly.

A [Ninja](https://ninja-build.org) build script is provided for use with GCC.
This places the executables in the `build` directory:

```
ninja
```

Usage is available in the source files and with `conv -h` and `sim -h`.

## Examples

Convert a text trace to binary:

```
conv input.trace > output
```

Perform a cache simulation with the default parameters:

```
sim output
```

Use a 8KB (2^13 bytes), 16-way associative cache with a block size of 8:

```
sim -c 13 -s 4 -b 3 output
```

Note that these values are provided as powers of 2.

Print each individual memory access and whether it hit or missed:

```
sim -v output
```

---

## Instructions

**The analysis paper MUST be turned in separately so it will go through turnitin.
(Don't put everything in one .zip file)**

### Objective

For this assignment, you will use a cache simulator to analyze the performance
(hit rate) of four cache designs. You will analyze the hit rate of the caches in
two real devices. You will do this in an analysis paper which should answer the
following questions:

1. How does the performance of a cache change with its associativity?
  (e.g. direct mapped vs n-way associative vs fully associative)
2. How does the performance of a cache change with cache size?
3. How does the performance change with replacement policy?
4. How does the performance change with the line size?
5. What is the performance of the cache design of two real devices?

Here is cache simulator code. You are welcome to update this to your liking.
You are also welcome to write your own code, in whatever language you like.

[sim.c](sim.c)

Do yourself a favor: Look at course material to learn about caches. You CANNOT
get everything you need to know about how to do this project from just the video
or project description. Applicable slide decks from lectures are Memory Module:
Parts 3, 4 and 5.

### Important Information

The format of the memory trace file is the following:

- The first field is “l” or “s” depending on whether the processor is loading
  from or storing to memory. You don't need to do anything differently for load or store.
- The second field is a 32-bit memory address.
- The third field is the number of bytes being requested.
- You don't need to do anything with the number of bytes being requested. The
  only field you care about is the address. You may assume that each request
  will be contained in one block.

**Comparing results is welcomed and encouraged.**

## Guidelines

### Items to Examine

1. **Parameters**

   - Number of bytes in the cache (a positive power of 2)
   - Number of blocks in each set (associativity)
   - Number of bytes in a block (a positive power of 2, must be at least 4)
   - 1 set of n blocks is fully associative
   - n sets of 1 block is direct mapped
   - n sets of m blocks is m-way set associative

2. Replacement policy (for caches that aren't direct mapped)

   - Least recently used (LRU)
   - First in, first out (FIFO)

3. **Examine effect of changing cache type**

   - Direct mapped vs. Set associative vs. Fully associative

4. **Examine effect of replacement type**

5. **Examine effect of cache size**

6. **Examine the effect of line/block size**

7. **Examine the performance of the caches of two real devices**

   - Example: older and new iPhone, older and new laptop - is cache different? Did hit rate improve?
   - Another example: thermostat vs laptop - is cache different? What design choices were made for each one?
   - You can analyze the L1 cache only.

## Analysis Paper

### Objective

Write a paper with your analysis of cache designs.

### Sections to Include

1. **Introduction**

   - Describe the purpose of the analysis. What is the goal of your analysis? What are you analyzing?

2. **Description of Tests**

   - What were the parameters for each test? Include the values of parameters:
     cache size, block size, associativity, replacement strategy.
   - Make sure to specify the associativity for set associative.
   - Why did you choose these parameter values?

3. **Results**

   - What were the hit rates for the different configurations?
   - Create plots to show your results.
     - x-y plot of hit rate vs cache size, with separate lines for FIFO and LRU
     - x-y plot of hit rate vs block size, with separate lines for FIFO and LRU
     - Make sure your plots are labeled on the x and y axes.
       y axis should be hit rate, x axis is independent variable.
       Give units on the independent variable (e.g. bytes)
     - Show points for the hit rate of your real devices.
     - Make sure we can see everything

4. **Conclusions**

   - How does cache design (direct mapped/set associative/fully associative) affect hit rate?
   - How does hit rate change with the two replacement policies?
   - How does cache size affect hit rate?
   - How does line/block size affect hit rate?
   - What was the hit rate for the two real devices?
     What was the difference in the two devices?
     Did one perform better than the other?
     What design choices were made?

**There are lots of example problems in the material. Look at those examples and
understand them before you start designing your analysis.**

## Submission

You must submit the following:

1. A screen capture (.mov or .mp4) showing a sample run.
   Your name must appear somewhere in the video. Sound is not necessary.
2. Your analysis paper.

**The analysis paper must be turned in separately so it will go through turnitin.
(Don't put everything in one .zip file)**
