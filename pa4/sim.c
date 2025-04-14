#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *progname = "sim";

#define err(fmt, ...) fprintf(stderr, "%s: " fmt "\n", progname, ##__VA_ARGS__)
#define err_sys(fmt, ...) fprintf(stderr, "%s: " fmt ": %s\n", progname, ##__VA_ARGS__, strerror(errno))

static void usage(FILE *f)
{
  fprintf(f, "\
usage: %s [options] [<file>]\n\
\n\
Options:\n\
  -h    show this help and exit\n\
  -l    use LRU eviction policy (default is FIFO)\n\
  -c N  set 2^N cache size in bytes (max 24, default 12)\n\
  -b N  set 2^N block size in bytes (min 2, default 2)\n\
  -s N  set 2^N associativity (0=direct, -1=full, default 3)\n\
  -v    log all accesses\n\
", progname);
}

static int access_cache(uint32_t *cache, uint32_t tag, int num_lines, int set_size, int lru)
{
  uint32_t *set = cache + (tag & (num_lines - 1) & ~(set_size - 1));

  for (int i = 0; i < set_size; i++) {
    if (set[i] == 0) {
      // miss, but available spot
      set[i] = tag;
      return 0;
    }
    if (set[i] == tag) {
      // hit
      if (lru) {
        for (; i < set_size - 1 && set[i + 1]; ++i) set[i] = set[i + 1];
        set[i] = tag;
      }
      return 1;
    }
  }

  // miss, eviction required
  for (int i = 1; i < set_size; i++) set[i - 1] = set[i];
  set[set_size - 1] = tag;
  return 0;
}

int main(int argc, char **argv)
{
  if (argc)
    progname = argv[0];

  int lru = 0;
  int cache_bits = 12;
  int line_bits = 2;
  int set_bits = 3;
  int verbose = 0;
  int opt;
  while ((opt = getopt(argc, argv, "hlc:b:s:v")) != -1) {
    switch (opt) {
      case 'h':
        usage(stdout);
        return 0;
      case 'l':
        lru = 1;
        break;
      case 'c':
        cache_bits = atoi(optarg);
        break;
      case 'b':
        line_bits = atoi(optarg);
        break;
      case 's':
        set_bits = atoi(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        usage(stderr);
        return 2;
    }
  }

  argc -= optind;
  argv += optind;

  const char *filename;
  if (argc == 0) {
    filename = NULL;
  } else if (argc == 1) {
    filename = argv[0];
  } else {
    usage(stderr);
    return 2;
  }

  if (cache_bits < 2 || cache_bits > 24) {
    err("cache_bits must be between 2 and 24: %d", cache_bits);
    return 2;
  }

  if (line_bits < 2 || line_bits > cache_bits) {
    err("line_bits must be between 2 and cache_bits (%d): %d", cache_bits, line_bits);
    return 2;
  }

  int num_lines_exp = cache_bits - line_bits;
  int num_lines = 1 << num_lines_exp;

  if (set_bits < 0 || set_bits > num_lines_exp)
    set_bits = num_lines_exp;

  int set_size = 1 << set_bits;

  // print cache information
  printf("Cache: %d bytes (%d lines, %d bytes/line), ", 1 << cache_bits, num_lines, 1 << line_bits);
  if (set_bits == 0)
    printf("direct-mapped");
  else if (set_bits == num_lines_exp)
    printf("fully associative");
  else
    printf("%d-way associative", set_size);
  printf(", ");
  if (lru)
    printf("LRU");
  else
    printf("FIFO");
  printf("\n");

  FILE *f = stdin;

  if (filename) {
    f = fopen(filename, "r");
    if (!f) {
      err_sys("failed to open '%s'", filename);
      return 1;
    }
  }

  uint32_t *cache = calloc(num_lines, sizeof(*cache));
  if (!cache) {
    err("failed to allocate cache with %d lines", num_lines);
    fclose(f);
    return 1;
  }

  // Note: calloc ensures that cache is already filled with zeroes.

  int hits = 0;
  int counter = 0;
  unsigned char buf[4];

  while (fread(&buf, sizeof(buf), 1, f)) {
    uint32_t addr = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    uint32_t tag = (addr >> line_bits) | 0x80000000;
    if (access_cache(cache, tag, num_lines, set_size, lru)) {
      ++hits;
      if (verbose)
        printf("%08"PRIx32" hit\n", addr);
    } else {
      if (verbose)
        printf("%08"PRIx32" miss\n", addr);
    }
    ++counter;
  }

  // print results
  printf("Hit rate: ");
  if (counter) {
    float hitrate = (float)hits / (float)counter;
    printf("%f (%d/%d)\n", hitrate, hits, counter);
  } else {
    printf("no input\n");
  }

  free(cache);
  fclose(f);
  return 0;
}
