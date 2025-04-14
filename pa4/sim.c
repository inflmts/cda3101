#define _POSIX_C_SOURCE 200809L
#include <errno.h>
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
usage: %s [options] <cache-bits> <set-bits> <line-bits> [<file>]\n\
\n\
Options:\n\
  -h    show this help and exit\n\
  -l    use LRU eviction policy (default is FIFO)\n\
", progname);
}

static int access_cache(uint32_t *cache, uint32_t tag, int num_lines, int set_size, int lru)
{
  uint32_t *set = cache + ((tag & (num_lines - 1)) & ~(set_size - 1));

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
  int lru = 0;
  int opt;
  while ((opt = getopt(argc, argv, "hl")) != -1) {
    switch (opt) {
      case 'h':
        usage(stdout);
        return 0;
      case 'l':
        lru = 1;
        break;
      default:
        usage(stderr);
        return 2;
    }
  }

  argc -= optind;
  argv += optind;

  const char *filename;
  switch (argc) {
    case 3:
      filename = NULL;
      break;
    case 4:
      filename = argv[3];
      break;
    default:
      usage(stderr);
      return 2;
  }

  int cache_bits = atoi(argv[0]);
  int set_bits = atoi(argv[1]);
  int line_bits = atoi(argv[2]);

  if (cache_bits < 0 || cache_bits > 24) {
    err("cache_bits must be between 0 and 24: %d", cache_bits);
    return 2;
  }

  if (line_bits < 0 || line_bits > cache_bits) {
    err("line_bits must be between 0 and cache_bits (%d): %d", cache_bits, line_bits);
    return 2;
  }

  int num_lines_exp = cache_bits - line_bits;
  int num_lines = 1 << num_lines_exp;

  if (set_bits < 0 || set_bits > num_lines_exp) {
    err("set_bits must be between 0 and num_lines_exp (%d): %d", num_lines_exp, set_bits);
    return 2;
  }

  int set_size = 1 << set_bits;

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

  // cache is already filled with zeroes

  int hits = 0;
  int counter = 0;
  unsigned char buf[4];

  while (fread(&buf, sizeof(buf), 1, f)) {
    uint32_t addr = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    uint32_t tag = addr >> line_bits;
    if (line_bits)
      tag |= 0x10000000;
    if (access_cache(cache, tag, num_lines, set_size, lru))
      ++hits;
    ++counter;
  }

  printf("Cache: %d bytes (%d lines, %d bytes/line), ", 1 << cache_bits, num_lines, 1 << line_bits);
  if (set_bits == 0)
    printf("fully associative\n");
  else if (set_bits == num_lines_exp)
    printf("direct-mapped\n");
  else
    printf("%d-way associative\n", set_size);

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
