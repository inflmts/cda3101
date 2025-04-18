#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *progname = "sim";

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define err(fmt, ...) fprintf(stderr, "%s: " fmt "\n", progname, ##__VA_ARGS__)
#define err_sys(fmt, ...) fprintf(stderr, "%s: " fmt ": %s\n", progname, ##__VA_ARGS__, strerror(errno))

static int setup_io(const char *input_file, const char *output_file)
{
  if (input_file && !freopen(input_file, "r", stdin)) {
    err_sys("failed to open '%s'", input_file);
    return -1;
  }
  if (output_file && !freopen(output_file, "w", stdout)) {
    err_sys("failed to open '%s'", output_file);
    return -1;
  }
  return 0;
}

static int cache_access(uint32_t *set, uint32_t tag, int set_size, int lru)
{
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

#define CACHE_QUIET (1 << 0)
#define CACHE_TRACE (1 << 1)

struct cache_params
{
  int cache_bits;
  int set_bits;
  int line_bits;
  int lru;
};

struct cache_result
{
  unsigned int hits;
  unsigned int total;
};

static int cache_simulate(
    FILE *input, const struct cache_params *params,
    struct cache_result *result, int flags)
{
  int cache_bits = params->cache_bits;
  int set_bits = params->set_bits;
  int line_bits = params->line_bits;
  int lru = params->lru;
  int num_lines_exp = cache_bits - line_bits;
  unsigned int num_lines = 1 << num_lines_exp;
  if (set_bits < 0)
    set_bits = num_lines_exp;
  unsigned int set_size = 1 << set_bits;

  // calloc ensures that cache is initialized with zeroes
  uint32_t *cache = calloc(num_lines, sizeof(uint32_t));
  if (!cache) {
    err("failed to allocate cache with %d lines", num_lines);
    return -1;
  }

  unsigned int hits = 0;
  unsigned int total = 0;
  uint32_t addr;

  while (fread(&addr, sizeof(addr), 1, input)) {
    addr = ntohl(addr);
    uint32_t tag = (addr >> line_bits) | 0x80000000;
    uint32_t *set = cache + (tag & (num_lines - 1) & ~(set_size - 1));
    if (cache_access(set, tag, set_size, lru)) {
      ++hits;
      if (flags & CACHE_TRACE)
        fprintf(stderr, "%08"PRIx32" hit\n", addr);
    } else {
      if (flags & CACHE_TRACE)
        fprintf(stderr, "%08"PRIx32" miss\n", addr);
    }
    ++total;
  }

  free(cache);

  if (ferror(input)) {
    err("read failure");
    return -1;
  }

  // print results
  if (!(flags & CACHE_QUIET)) {
    float hitrate = total ? (float)hits / (float)total : 0.0f;
    fprintf(stderr, "Cache: %d (%d x %d), ", 1 << cache_bits, num_lines, 1 << line_bits);
    if (set_bits == 0)
      fprintf(stderr, "direct");
    else if (set_bits == num_lines_exp)
      fprintf(stderr, "full");
    else
      fprintf(stderr, "%u-way", set_size);
    fprintf(stderr, ", %s: %f (%d/%d)\n", lru ? "LRU" : "FIFO", hitrate, hits, total);
  }

  result->hits = hits;
  result->total = total;
  return 0;
}

struct plot_line
{
  int set_bits;
  int lru;
  const char *description;
  const char *color;
};

static const struct plot_line plot_lines[] = {
  { 0,  0, "direct-mapped", "#f00" },
  { 1,  0, "2-way set assoc FIFO", "#880" },
  { 1,  1, "2-way set assoc LRU", "#dd0" },
  { 2,  0, "4-way set assoc FIFO", "#080" },
  { 2,  1, "4-way set assoc LRU", "#0f0" },
  { 3,  0, "8-way set assoc FIFO", "#048" },
  { 3,  1, "8-way set assoc LRU", "#08f" },
  { -1, 0, "fully assoc FIFO", "#808" },
  { -1, 1, "fully assoc LRU", "#f0f" },
};

#define NUM_PLOT_LINES ARRAY_SIZE(plot_lines)

static void draw_plot(
    int *args, int n, float *values,
    const char *x_label)
{
  static const int m = NUM_PLOT_LINES;

  int x_min = INT_MAX;
  int x_max = INT_MIN;
  float y_min = HUGE_VALF;
  float y_max = -HUGE_VALF;

  for (int idx = 0, i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      int x = args[j];
      float y = values[idx++];
      if (x < x_min)
        x_min = x;
      if (x > x_max)
        x_max = x;
      if (y < y_min)
        y_min = y;
      if (y > y_max)
        y_max = y;
    }
  }

  int x_granularity = 1;
  while (x_max / x_granularity - x_min / x_granularity > 20)
    x_granularity <<= 1;
  x_min = x_min / x_granularity * x_granularity;
  x_max = (x_max + x_granularity - 1) / x_granularity * x_granularity;

  int y_granularity = 1;
  int y_mini;
  int y_maxi;
  while (1) {
    y_mini = (int)(y_min * y_granularity);
    y_maxi = (int)(y_max * y_granularity) + 1;
    if (y_maxi - y_mini >= 5)
      break;
    y_granularity *= 10;
  }
  y_min = (float)y_mini / y_granularity;
  y_max = (float)y_maxi / y_granularity;

  static const float width = 800;
  static const float height = 400;
  static const float graph_left = 80;
  static const float graph_right = 600;
  static const float graph_top = 20;
  static const float graph_bottom = 350;
  static const float graph_width = graph_right - graph_left;
  static const float graph_height = graph_bottom - graph_top;

  float x_factor = graph_width / (x_max - x_min);
  float x_offset = graph_left - (float)x_min * x_factor;
  float y_factor = graph_height / (y_min - y_max);
  float y_offset = graph_bottom - (float)y_min * y_factor;

  // draw labels
  printf(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%f\" height=\"%f\">\n"
    "<text x=\"0\" y=\"0\" text-anchor=\"middle\" transform=\"translate(%f,%f) rotate(-90)\">Hit Rate</text>\n"
    "<text x=\"%f\" y=\"%f\" text-anchor=\"middle\">%s</text>\n"
    ,
    width, height,
    graph_left - 60.0f, (graph_top + graph_bottom) * 0.5f,
    (graph_left + graph_right) * 0.5f, graph_bottom + 40.0f, x_label);

  // draw legend
  for (int i = 0; i < m; i++) {
    float y = graph_top + 30.0f + i * 20.0f;
    printf(
      "<rect x=\"%f\" y=\"%f\" width=\"14\" height=\"14\" fill=\"%s\" />\n"
      "<text x=\"%f\" y=\"%f\" font-size=\"14\">%s</text>\n"
      ,
      graph_right + 20.0f, y - 14.0f, plot_lines[i].color,
      graph_right + 40.0f, y, plot_lines[i].description);
  }

  // draw vertical subdivisions
  for (int i = y_mini; i <= y_maxi; i++) {
    float y = (float)i / y_granularity;
    float fy = y * y_factor + y_offset;
    printf(
      "<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"end\" font-size=\"10\">%.3f</text>\n"
      "<line stroke=\"#ddd\" stroke-width=\"1\" x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" />\n"
      ,
      graph_left - 15.0f, fy, y,
      graph_left, fy, graph_right, fy);
  }

  // draw horizontal subdivisions
  for (int x = x_min; x <= x_max; x += x_granularity) {
    float fx = x * x_factor + x_offset;
    printf(
      "<text x=\"%f\" y=\"%f\" text-anchor=\"middle\" font-size=\"10\">%d</text>\n"
      "<line stroke=\"#ddd\" stroke-width=\"1\" x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" />\n"
      ,
      fx, graph_bottom + 20.0f, x,
      fx, graph_top, fx, graph_bottom);
  }

  for (int i = 0; i < m; i++) {
    // draw line
    printf("<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2\" points=\"",
      plot_lines[i].color
    );
    for (int j = 0; j < n; j++) {
      printf(" %f,%f",
        args[j] * x_factor + x_offset,
        values[i * n + j] * y_factor + y_offset);
    }
    printf("\" />\n<g fill=\"%s\">\n",
      plot_lines[i].color
    );

    // draw points
    for (int j = 0; j < n; j++) {
      printf("<circle cx=\"%f\" cy=\"%f\" r=\"3\" />\n",
        args[j] * x_factor + x_offset,
        values[i * n + j] * y_factor + y_offset);
    }
    printf("</g>\n");
  }

  printf(
    "</svg>\n"
  );
}

static int generate_cache_size_plot(int flags)
{
  static const int sizes[] = { 11, 12, 13, 14, 15 };
  int args[ARRAY_SIZE(sizes)];
  for (int i = 0; i < ARRAY_SIZE(sizes); i++)
    args[i] = 1 << sizes[i];

  struct cache_params params = { 0, 0, 6, 0 };
  struct cache_result result;
  float values[ARRAY_SIZE(plot_lines)][ARRAY_SIZE(args)];
  for (int i = 0; i < ARRAY_SIZE(plot_lines); i++) {
    for (int j = 0; j < ARRAY_SIZE(sizes); j++) {
      params.cache_bits = sizes[j];
      params.set_bits = plot_lines[i].set_bits;
      params.lru = plot_lines[i].lru;
      rewind(stdin);
      if (cache_simulate(stdin, &params, &result, flags))
        return -1;
      values[i][j] = (float)result.hits / result.total;
    }
  }

  draw_plot(args, ARRAY_SIZE(args), (float *)values, "Cache Size (bytes)");
  return 0;
};

static int generate_block_size_plot(int flags)
{
  static const int sizes[] = { 4, 5, 6, 7, 8 };
  int args[ARRAY_SIZE(sizes)];
  for (int i = 0; i < ARRAY_SIZE(sizes); i++)
    args[i] = 1 << sizes[i];

  struct cache_params params = { 14, 0, 0, 0 };
  struct cache_result result;
  float values[ARRAY_SIZE(plot_lines)][ARRAY_SIZE(args)];
  for (int i = 0; i < ARRAY_SIZE(plot_lines); i++) {
    for (int j = 0; j < ARRAY_SIZE(sizes); j++) {
      params.set_bits = plot_lines[i].set_bits;
      params.line_bits = sizes[j];
      params.lru = plot_lines[i].lru;
      rewind(stdin);
      if (cache_simulate(stdin, &params, &result, flags))
        return -1;
      values[i][j] = (float)result.hits / result.total;
    }
  }

  draw_plot(args, ARRAY_SIZE(args), (float *)values, "Block Size (bytes)");
  return 0;
};

static void usage(FILE *f)
{
  fprintf(f, "\
usage: %s [options] [<file>]\n\
\n\
Options:\n\
  -h      show this help and exit\n\
  -l      use LRU eviction policy (default is FIFO)\n\
  -c N    set 2^N cache size in bytes (max 24, default 12)\n\
  -b N    set 2^N block size in bytes (min 2, default 2)\n\
  -s N    set 2^N associativity (0=direct, -1=full, default 3)\n\
  -q      don't print results\n\
  -v      log all accesses\n\
  -P type generate 'cachesize' or 'blocksize' plot\n\
  -o file specify output file\n\
", progname);
}

int main(int argc, char **argv)
{
  if (argc)
    progname = argv[0];

  int lru = 0;
  int cache_bits = 12;
  int line_bits = 2;
  int set_bits = 3;
  int flags = 0;
  const char *plot = NULL;
  const char *output_file = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "hlc:b:s:qvP:o:")) != -1) {
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
      case 'q':
        flags |= CACHE_QUIET;
        break;
      case 'v':
        flags |= CACHE_TRACE;
        break;
      case 'P':
        plot = optarg;
        break;
      case 'o':
        output_file = optarg;
        break;
      default:
        usage(stderr);
        return 2;
    }
  }

  argc -= optind;
  argv += optind;

  const char *input_file;
  if (argc == 0) {
    input_file = NULL;
  } else if (argc == 1) {
    input_file = argv[0];
  } else {
    usage(stderr);
    return 2;
  }

  if (plot) {
    if (!strcmp(plot, "cachesize")) {
      setup_io(input_file, output_file);
      return generate_cache_size_plot(flags) ? 1 : 0;
    }
    if (!strcmp(plot, "blocksize")) {
      setup_io(input_file, output_file);
      return generate_block_size_plot(flags) ? 1 : 0;
    }
    err("invalid plot type '%s'", plot);
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
  if (set_bits < 0 || set_bits > num_lines_exp)
    set_bits = num_lines_exp;

  setup_io(input_file, output_file);

  struct cache_params params = { cache_bits, set_bits, line_bits, lru };
  struct cache_result result;
  if (cache_simulate(stdin, &params, &result, flags))
    return 1;

  return 0;
}
