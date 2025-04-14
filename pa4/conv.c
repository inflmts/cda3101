#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *progname = "compile-trace";

#define err(fmt, ...) fprintf(stderr, "%s: " fmt "\n", progname, ##__VA_ARGS__)
#define err_sys(fmt, ...) fprintf(stderr, "%s: " fmt ": %s\n", progname, ##__VA_ARGS__, strerror(errno))

static void usage(FILE *f)
{
  fprintf(f, "\
usage: %s [options] [<file>]\n\
\n\
Options:\n\
  -h    show this help and exit\n\
", progname);
}

static int compile_trace_file(FILE *f)
{
  char line[32];
  unsigned int lineno = 0;
  unsigned char buf[4];

  while (fgets(line, sizeof(line), f)) {
    char *s = line;
    ++lineno;

    // skip access type
    while (*s != ' ') {
      if (!*s)
        goto bad_input;
      ++s;
    }
    ++s;

    // read 32-bit address
    uint32_t addr = 0;
    if (*s != '0') goto bad_input;
    ++s;
    if (*s != 'x' && *s != 'X') goto bad_input;
    ++s;
    while (*s != ' ') {
      if (*s >= '0' && *s <= '9')
        addr = (addr << 4) | (*s - '0');
      else if (*s >= 'A' && *s <= 'F')
        addr = (addr << 4) | (*s - 'A' + 10);
      else if (*s >= 'a' && *s <= 'f')
        addr = (addr << 4) | (*s - 'a' + 10);
      else
        goto bad_input;
      ++s;
    }

    buf[0] = addr >> 24;
    buf[1] = addr >> 16;
    buf[2] = addr >> 8;
    buf[3] = addr;

    // skip the rest of the line (number of bytes requested)
    fwrite(&buf, sizeof(buf), 1, stdout);
    continue;

bad_input:
    err("invalid input at line %u", lineno);
    return 1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  if (argc)
    progname = argv[0];

  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
      case 'h':
        usage(stdout);
        return 0;
      default:
        usage(stderr);
        return 2;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc == 0) {
    return compile_trace_file(stdin);
  } else if (argc == 1) {
    FILE *f = fopen(argv[0], "r");
    if (!f) {
      err_sys("failed to open '%s'", argv[0]);
      return 1;
    }
    int ret = compile_trace_file(f);
    fclose(f);
    return ret;
  } else {
    usage(stderr);
    return 2;
  }
}
