#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "commands.h"

#define VERSION "0.1.0"

static void print_usage(void) {
    printf("ohbobi p -- planner v%s\n", VERSION);
    printf("Usage:\n");
    printf("  ohbobi p              Show plan grid (default)\n");
    printf("  ohbobi p np           New plan (default: tomorrow)\n");
    printf("  ohbobi p np -w        New weekly plan\n");
    printf("  ohbobi p np -m        New monthly plan\n");
    printf("  ohbobi p np -y        New yearly plan\n");
    printf("  ohbobi p np -d DATE   New plan for DATE\n");
    printf("  ohbobi p sp           Show plan grid\n");
    printf("  ohbobi p sp -d DATE   Show grid starting at DATE\n");
    printf("  ohbobi p ep           Edit existing plan\n");
    printf("  ohbobi p ep -w        Edit weekly plan\n");
    printf("  ohbobi p ep -m        Edit monthly plan\n");
    printf("  ohbobi p ep -y        Edit yearly plan\n");
    printf("  ohbobi p ep -d DATE   Edit plan for DATE\n");
    printf("  ohbobi p -h, --help   Show this help\n");
    printf("  ohbobi p -v, --version Show version\n");
}

int main(int argc, char *argv[]) {
    if (!storage_init()) {
        fprintf(stderr, "error: could not initialize data storage\n");
        return 1;
    }

    if (argc < 2) {
        return cmd_sp(argc, argv);
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("ohbobi p (planner) v%s\n", VERSION);
        return 0;
    }

    if (strcmp(argv[1], "np") == 0) {
        return cmd_np(argc, argv);
    }

    if (strcmp(argv[1], "sp") == 0) {
        return cmd_sp(argc, argv);
    }

    if (strcmp(argv[1], "ep") == 0) {
        return cmd_ep(argc, argv);
    }

    print_usage();
    return 1;
}
