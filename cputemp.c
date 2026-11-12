// cputemp - print the CPU temperature on Intel Macs by reading the SMC.
//
// Build: make   (or: clang -O2 -o cputemp cputemp.c smc.c -framework IOKit)
// Usage: cputemp [-f]     prints e.g. "68.6" (Celsius), or Fahrenheit with -f

#include <stdio.h>
#include <unistd.h>

#include "smc.h"

// CPU temperature keys, in order of preference. The die keys (TC0D, TC0F,
// TC0E, TC0H) come first; TC0P is a proximity sensor near the CPU that reads
// 30-40 °C lower than the die under load, so it is only a last resort.
static const char *CPU_KEYS[] = {"TC0D", "TC0F", "TC0E", "TC0H", "TC0P", NULL};

static double cpu_temp(void) {
    for (int i = 0; CPU_KEYS[i]; i++) {
        double t;
        if (smc_read_temp(CPU_KEYS[i], &t) == 0 && t > 0 && t < 150) return t;
    }
    return -1;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [-f]\n"
            "  Prints the CPU temperature in Celsius, e.g. 68.6\n"
            "  -f  print Fahrenheit instead\n",
            prog);
}

int main(int argc, char **argv) {
    int fahrenheit = 0, opt;
    while ((opt = getopt(argc, argv, "fh")) != -1) {
        switch (opt) {
            case 'f': fahrenheit = 1; break;
            case 'h': usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "error: unexpected argument '%s'\n", argv[optind]);
        usage(argv[0]);
        return 1;
    }

    if (smc_open() != kIOReturnSuccess) {
        fprintf(stderr, "error: could not open AppleSMC (Intel Macs only)\n");
        return 1;
    }
    double c = cpu_temp();
    smc_close();
    if (c < 0) {
        fprintf(stderr, "error: no CPU temperature sensor found\n");
        return 1;
    }
    if (printf("%.1f\n", fahrenheit ? c * 9 / 5 + 32 : c) < 0 || fflush(stdout) == EOF) {
        perror("error: could not write output");
        return 1;
    }
    return 0;
}
