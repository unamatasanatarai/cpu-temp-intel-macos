// osxtemps - list every temperature sensor the SMC exposes on Intel Macs.
//
// Build: make   (or: clang -O2 -o osxtemps osxtemps.c smc.c -framework IOKit)
// Usage: osxtemps [-f]    prints one "KEY  VALUE  description" line per sensor

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "smc.h"

// Descriptions for commonly seen keys. Meanings vary a little between Mac
// models; keys not listed here are still printed, just without a description.
static const struct {
    const char *key, *desc;
} KNOWN[] = {
    {"TA0P", "Ambient"},
    {"TA0V", "Ambient"},
    {"TA1P", "Ambient 2"},
    {"TB0T", "Battery (max)"},
    {"TB1T", "Battery 1"},
    {"TB2T", "Battery 2"},
    {"TB3T", "Battery 3"},
    {"TBXT", "Battery (max)"},
    {"TC0C", "CPU core (PECI)"},
    {"TC0D", "CPU die"},
    {"TC0E", "CPU die (PECI, filtered)"},
    {"TC0F", "CPU die (PECI, filtered, adjusted)"},
    {"TC0H", "CPU heatsink"},
    {"TC0P", "CPU proximity"},
    {"TC1C", "CPU core 1"},
    {"TC2C", "CPU core 2"},
    {"TC3C", "CPU core 3"},
    {"TC4C", "CPU core 4"},
    {"TC5C", "CPU core 5"},
    {"TC6C", "CPU core 6"},
    {"TC7C", "CPU core 7"},
    {"TC8C", "CPU core 8"},
    {"TCGC", "CPU integrated GPU (PECI)"},
    {"TCSA", "CPU system agent (PECI)"},
    {"TCSC", "CPU system agent (PECI)"},
    {"TCXC", "CPU (PECI, max)"},
    {"TCXr", "CPU (PECI, raw)"},
    {"TG0D", "GPU die"},
    {"TG0H", "GPU heatsink"},
    {"TG0P", "GPU proximity"},
    {"TG1D", "GPU 2 die"},
    {"TH0A", "Drive A"},
    {"TH0B", "Drive B"},
    {"TH0C", "Drive C"},
    {"TH0F", "Drive (filtered)"},
    {"TH0P", "Drive bay proximity"},
    {"TH0V", "Drive (virtual)"},
    {"TH0a", "Drive A (raw)"},
    {"TH0b", "Drive B (raw)"},
    {"TH0c", "Drive C (raw)"},
    {"TH0x", "Drive (max)"},
    {"TI0P", "Thunderbolt 1 proximity"},
    {"TI1P", "Thunderbolt 2 proximity"},
    {"TL0P", "LCD proximity"},
    {"TM0P", "Memory proximity"},
    {"TM0S", "Memory slot 1"},
    {"TM1S", "Memory slot 2"},
    {"TN0D", "Northbridge die"},
    {"TN0P", "Northbridge proximity"},
    {"TPCD", "Platform controller hub die"},
    {"TW0P", "Wireless proximity"},
    {"Ta0P", "Airflow"},
    {"TaLC", "Airflow left"},
    {"TaRC", "Airflow right"},
    {"Th0H", "Heatpipe / fin stack 0"},
    {"Th1H", "Heatpipe / fin stack 1"},
    {"Th2H", "Heatpipe / fin stack 2"},
    {"Tm0P", "Mainboard proximity"},
    {"Tp0P", "Power supply proximity"},
    {"Ts0P", "Palm rest"},
    {"Ts0S", "Memory bank proximity"},
    {"Ts1P", "Palm rest 2"},
    {"Ts1S", "Memory bank proximity 2"},
    {"Ts2S", "Memory bank proximity 3"},
};

static const char *describe(const char *key) {
    for (size_t i = 0; i < sizeof(KNOWN) / sizeof(KNOWN[0]); i++)
        if (strcmp(KNOWN[i].key, key) == 0) return KNOWN[i].desc;
    return "";
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [-f]\n"
            "  Lists every SMC temperature sensor in Celsius, one per line:\n"
            "  KEY  VALUE  description\n"
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

    UInt32 n = smc_key_count();
    int found = 0, write_err = 0;
    for (UInt32 i = 0; i < n; i++) {
        char key[5];
        double c;
        // Temperature keys all start with 'T'; skip the rest without reading them.
        if (smc_key_at(i, key) != 0 || key[0] != 'T') continue;
        if (smc_read_temp(key, &c) != 0) continue;
        // Absent or unpopulated sensors report 0, negative or saturated values.
        if (!(c > 0 && c < 150)) continue;
        found++;
        const char *desc = describe(key);
        if (printf("%s  %5.1f%s%s\n", key, fahrenheit ? c * 9 / 5 + 32 : c,
                   *desc ? "  " : "", desc) < 0)
            write_err = 1;
    }
    smc_close();

    if (n == 0) {
        fprintf(stderr, "error: could not enumerate SMC keys\n");
        return 1;
    }
    if (fflush(stdout) == EOF || write_err) {
        perror("error: could not write output");
        return 1;
    }
    if (!found) {
        fprintf(stderr, "error: no temperature sensors found\n");
        return 1;
    }
    return 0;
}
