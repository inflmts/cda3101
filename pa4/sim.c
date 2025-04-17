#define _POSIX_C_SOURCE 200809L
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

struct cache_params
{
  int tag_bits;
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
    FILE *input, struct cache_params *params,
    int verbose, struct cache_result *result)
{
  int tag_bits = params->tag_bits;
  int set_bits = params->set_bits;
  int line_bits = params->line_bits;
  int lru = params->lru;
  unsigned int set_size = 1 << set_bits;
  unsigned int num_lines = set_size << tag_bits;

  // print cache information
  fprintf(stderr, "Cache: %d bytes (%u lines, %d bytes/line), ", num_lines << line_bits, num_lines, 1 << line_bits);
  if (set_bits == 0)
    fprintf(stderr, "direct-mapped");
  else if (tag_bits == 0)
    fprintf(stderr, "fully associative");
  else
    fprintf(stderr, "%u-way associative", set_size);
  fprintf(stderr, ", %s\n", lru ? "LRU" : "FIFO");

  // calloc ensures that cache is initialized with zeroes
  uint32_t *cache = calloc(num_lines, sizeof(uint32_t));
  if (!cache) {
    err("failed to allocate cache with %d lines", num_lines);
    return -1;
  }

  unsigned int hits = 0;
  unsigned int total = 0;
  unsigned char buf[4];

  while (fread(&buf, sizeof(buf), 1, input)) {
    uint32_t addr = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    uint32_t tag = (addr >> line_bits) | 0x80000000;
    uint32_t *set = cache + (tag & (num_lines - 1) & ~(set_size - 1));
    if (cache_access(set, tag, set_size, lru)) {
      ++hits;
      if (verbose)
        fprintf(stderr, "%08"PRIx32" hit\n", addr);
    } else {
      if (verbose)
        fprintf(stderr, "%08"PRIx32" miss\n", addr);
    }
    ++total;
  }

  free(cache);

  if (ferror(input)) {
    err("read failure");
    return -1;
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

static void plot_draw(
    FILE *output,
    const struct plot_line *lines, int m,
    int *args, int n, float *values,
    const char *x_label)
{
  int x_min = INT_MAX;
  int x_max = INT_MIN;
  float y_min = HUGE_VALF;
  float y_max = -HUGE_VALF;

  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      int x = args[j];
      float y = values[i * n + j];
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

  const float width = 600;
  const float height = 400;
  const float margin_left = 80;
  const float margin_right = 20;
  const float margin_top = 20;
  const float margin_bottom = 45;
  const float graph_width = width - margin_left - margin_right;
  const float graph_height = height - margin_top - margin_bottom;

  float x_factor = graph_width / (x_max - x_min);
  float x_offset = margin_left - (float)x_min * x_factor;
  float y_factor = graph_height / (y_min - y_max);
  float y_offset = (height - margin_bottom) - (float)y_min * y_factor;

  fprintf(output,
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%f\" height=\"%f\">\n"
    "<text x=\"0\" y=\"20\" text-anchor=\"middle\" transform=\"translate(0,%f) rotate(-90)\">Hit Rate</text>\n"
    "<text x=\"%f\" y=\"%f\" text-anchor=\"middle\">%s</text>\n"
    ,
    width, height,
    height * 0.5f,
    margin_left + graph_width * 0.5f, height - 5.0f, x_label);

  // draw vertical subdivisions
  for (int i = y_mini; i <= y_maxi; i++) {
    float y = (float)i / y_granularity;
    float fy = y * y_factor + y_offset;
    fprintf(output,
      "<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"end\" font-size=\"8pt\">%.3f</text>\n"
      "<line stroke=\"#ddd\" stroke-width=\"1\" x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" />\n"
      ,
      margin_left - 20.0f, fy, y,
      margin_left, fy, width - margin_right, fy);
  }

  // draw horizontal subdivisions
  for (int x = x_min; x <= x_max; x += x_granularity) {
    float fx = x * x_factor + x_offset;
    fprintf(output,
      "<text x=\"%f\" y=\"%f\" text-anchor=\"middle\" font-size=\"8pt\">%d</text>\n"
      "<line stroke=\"#ddd\" stroke-width=\"1\" x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" />\n"
      ,
      fx, height - 25.0f, x,
      fx, margin_top, fx, height - margin_bottom);
  }

  for (int i = 0; i < m; i++) {
    // draw line
    fprintf(output,
      "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2\" points=\"",
      lines[i].color
    );
    for (int j = 0; j < n; j++) {
      fprintf(output,
        " %f,%f",
        args[j] * x_factor + x_offset,
        values[i * n + j] * y_factor + y_offset);
    }
    fprintf(output,
      "\" />\n<g fill=\"%s\">\n",
      lines[i].color
    );

    // draw points
    for (int j = 0; j < n; j++) {
      fprintf(output,
        "<circle cx=\"%f\" cy=\"%f\" r=\"3\" />\n",
        args[j] * x_factor + x_offset,
        values[i * n + j] * y_factor + y_offset);
    }
    fprintf(output, "</g>\n");
  }

  fprintf(output,
    "</svg>\n"
  );
}

static const struct plot_line lines[] = {
  { 0, 0, "direct-mapped", "red" },
  { 1, 0, "2-way associative FIFO", "orange" },
  { 1, 1, "2-way associative LRU", "yellow" },
  { 2, 0, "4-way associative FIFO", "green" },
  { 2, 1, "4-way associative LRU", "cyan" },
  { 3, 0, "8-way associative FIFO", "blue" },
  { 3, 1, "8-way associative LRU", "purple" },
};

static int generate_cache_size_plot(FILE *input, FILE *output)
{
  static const int sizes[] = { 10, 11, 12, 13, 14 };
  int args[ARRAY_SIZE(sizes)];
  for (int i = 0; i < ARRAY_SIZE(sizes); i++)
    args[i] = 1 << sizes[i];

  struct cache_params params = { 0, 0, 3, 0 };
  struct cache_result result;
  float values[ARRAY_SIZE(lines)][ARRAY_SIZE(args)];
  for (int i = 0; i < ARRAY_SIZE(lines); i++) {
    for (int j = 0; j < ARRAY_SIZE(sizes); j++) {
      params.tag_bits = sizes[j] - lines[i].set_bits - params.line_bits;
      params.set_bits = lines[i].set_bits;
      params.lru = lines[i].lru;
      rewind(input);
      if (cache_simulate(input, &params, 0, &result))
        return -1;
      values[i][j] = (float)result.hits / result.total;
    }
  }

  plot_draw(output, lines, ARRAY_SIZE(lines), args, ARRAY_SIZE(args), (float *)values, "Cache Size (bytes)");
  return 0;
};

static int generate_block_size_plot(FILE *input, FILE *output)
{
  static const int sizes[] = { 2, 3, 4, 5, 6, 7 };
  int args[ARRAY_SIZE(sizes)];
  for (int i = 0; i < ARRAY_SIZE(sizes); i++)
    args[i] = 1 << sizes[i];

  struct cache_params params = { 0, 0, 0, 0 };
  struct cache_result result;
  float values[ARRAY_SIZE(lines)][ARRAY_SIZE(args)];
  for (int i = 0; i < ARRAY_SIZE(lines); i++) {
    for (int j = 0; j < ARRAY_SIZE(sizes); j++) {
      params.tag_bits = 12 - lines[i].set_bits - sizes[j];
      params.set_bits = lines[i].set_bits;
      params.line_bits = sizes[j];
      params.lru = lines[i].lru;
      rewind(input);
      if (cache_simulate(input, &params, 0, &result))
        return -1;
      values[i][j] = (float)result.hits / result.total;
    }
  }

  plot_draw(output, lines, ARRAY_SIZE(lines), args, ARRAY_SIZE(args), (float *)values, "Block Size (bytes)");
  return 0;
};

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
  -P T  generate 'cache' or 'block' plot\n\
        (cannot combine with -l -c -b -s)\n\
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
  int verbose = 0;
  const char *plot = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "hlc:b:s:vP:")) != -1) {
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
      case 'P':
        plot = optarg;
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

  if (plot) {
    if (!strcmp(plot, "cache")) {
      FILE *input = stdin;
      if (filename) {
        input = fopen(filename, "r");
        if (!input) {
          err_sys("failed to open '%s'", filename);
          return 1;
        }
      }
      int ret = generate_cache_size_plot(input, stdout);
      fclose(input);
      return ret ? 1 : 0;
    }
    if (!strcmp(plot, "block")) {
      FILE *input = stdin;
      if (filename) {
        input = fopen(filename, "r");
        if (!input) {
          err_sys("failed to open '%s'", filename);
          return 1;
        }
      }
      int ret = generate_block_size_plot(input, stdout);
      fclose(input);
      return ret ? 1 : 0;
    }
    err("invalid argument for -P: '%s'", plot);
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

  FILE *input = stdin;

  if (filename) {
    input = fopen(filename, "r");
    if (!input) {
      err_sys("failed to open '%s'", filename);
      return 1;
    }
  }

  int tag_bits = cache_bits - set_bits - line_bits;
  struct cache_params params = { tag_bits, set_bits, line_bits, lru };
  struct cache_result result;
  if (cache_simulate(input, &params, verbose, &result)) {
    fclose(input);
    return 1;
  }

  // print results
  printf("Hit rate: ");
  if (result.total) {
    float hitrate = (float)result.hits / (float)result.total;
    printf("%f (%d/%d)\n", hitrate, result.hits, result.total);
  } else {
    printf("no input\n");
  }

  return 0;
}
