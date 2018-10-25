#include "json.h"

int main(int argc, char *argv[]) {
    if (argc < 3) print_usage(argv[0]);
    int opt;
    char *h_realpath = NULL;
    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c': {
                h_realpath = get_realpath(optarg);
            }
                break;
            default:
                print_usage(argv[0]);
        }
    }
    if (!h_realpath) __exit(1)
    FILE *config_fp = fopen(h_realpath, "r");
    if (!config_fp) __exit(1)
    char *buf = calloc(512, sizeof(char));
    char *cfg_string = calloc(1, sizeof(char));
    size_t len = 0;
    while (fgets(buf, 512, config_fp)) {
        if (!buf || !cfg_string) __exit(1)
        len = strlen(cfg_string) + strlen(buf) + 1;
        cfg_string = realloc(cfg_string, len);
        strcat(cfg_string, buf);
    }
    free(buf);
    cfg_parser(cfg_string);
    return 0;
}

void print_usage(const char *filename) {
    fprintf(stderr, "Usage: %s -c <config file>\n", filename);
    exit(0);
}

char *get_realpath(char *optarg) {
    char *r = realpath(optarg, NULL);
    if (!r) __exit(1)
    return r;
}

int cfg_parser(char *cfg) {

    return 0;
}
