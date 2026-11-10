// cputemp - print the CPU temperature on Intel Macs by reading the SMC.
//
// Build: make   (or: clang -O2 -o cputemp cputemp.c -framework IOKit)
// Usage: cputemp [-f]     prints e.g. "68.6" (Celsius), or Fahrenheit with -f

#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KERNEL_INDEX_SMC 2
#define SMC_CMD_READ_BYTES 5
#define SMC_CMD_READ_KEYINFO 9

typedef struct {
    char major, minor, build, reserved[1];
    UInt16 release;
} SMCKeyData_vers_t;

typedef struct {
    UInt16 version, length;
    UInt32 cpuPLimit, gpuPLimit, memPLimit;
} SMCKeyData_pLimitData_t;

typedef struct {
    UInt32 dataSize;
    UInt32 dataType;
    char dataAttributes;
} SMCKeyData_keyInfo_t;

typedef struct {
    UInt32 key;
    SMCKeyData_vers_t vers;
    SMCKeyData_pLimitData_t pLimitData;
    SMCKeyData_keyInfo_t keyInfo;
    char result, status, data8;
    UInt32 data32;
    unsigned char bytes[32];
} SMCKeyData_t;

static io_connect_t conn;

static UInt32 str_to_key(const char *s) {
    return ((UInt32)s[0] << 24) | ((UInt32)s[1] << 16) | ((UInt32)s[2] << 8) | (UInt32)s[3];
}

static kern_return_t smc_open(void) {
    io_service_t svc = IOServiceGetMatchingService(0, IOServiceMatching("AppleSMC"));
    if (!svc) return kIOReturnNotFound;
    kern_return_t r = IOServiceOpen(svc, mach_task_self(), 0, &conn);
    IOObjectRelease(svc);
    return r;
}

static kern_return_t smc_call(SMCKeyData_t *in, SMCKeyData_t *out) {
    size_t outSize = sizeof(SMCKeyData_t);
    return IOConnectCallStructMethod(conn, KERNEL_INDEX_SMC, in, sizeof(*in), out, &outSize);
}

// Reads an "sp78" (signed 8.8 fixed point) temperature key.
// Returns -1 if the key is missing or has any other data type.
static double smc_read_temp(const char *key) {
    SMCKeyData_t in = {0}, out = {0};
    in.key = str_to_key(key);
    in.data8 = SMC_CMD_READ_KEYINFO;
    if (smc_call(&in, &out) != kIOReturnSuccess || out.result != 0) return -1;
    if (out.keyInfo.dataType != str_to_key("sp78") || out.keyInfo.dataSize != 2) return -1;

    in.keyInfo.dataSize = out.keyInfo.dataSize;
    in.data8 = SMC_CMD_READ_BYTES;
    memset(&out, 0, sizeof(out));
    if (smc_call(&in, &out) != kIOReturnSuccess || out.result != 0) return -1;

    int16_t raw = (int16_t)((out.bytes[0] << 8) | out.bytes[1]);
    return raw / 256.0;
}

// CPU temperature keys, in order of preference. The die keys (TC0D, TC0F,
// TC0E, TC0H) come first; TC0P is a proximity sensor near the CPU that reads
// 30-40 °C lower than the die under load, so it is only a last resort.
static const char *CPU_KEYS[] = {"TC0D", "TC0F", "TC0E", "TC0H", "TC0P", NULL};

static double cpu_temp(void) {
    for (int i = 0; CPU_KEYS[i]; i++) {
        double t = smc_read_temp(CPU_KEYS[i]);
        if (t > 0 && t < 150) return t;
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
    IOServiceClose(conn);
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
