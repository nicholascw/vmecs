#define _DEFAULT_SOURCE

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>

#define _exit(X) {perror("__OUTPUT__"); exit(X);}

typedef struct _cfg_entry {
    char *key;
} cfg_entry;

void print_usage(const char *filename);

char *get_realpath(char *optarg);

int cfg_parser(char *cfg);
