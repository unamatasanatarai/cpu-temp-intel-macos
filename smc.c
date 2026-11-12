// smc.c - minimal read-only access to the Apple System Management Controller.
#include "smc.h"

#include <string.h>

#define KERNEL_INDEX_SMC 2
#define SMC_CMD_READ_BYTES 5
#define SMC_CMD_READ_INDEX 8
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
    return ((UInt32)(unsigned char)s[0] << 24) | ((UInt32)(unsigned char)s[1] << 16) |
           ((UInt32)(unsigned char)s[2] << 8) | (UInt32)(unsigned char)s[3];
}

static void key_to_str(UInt32 k, char out[5]) {
    out[0] = (char)(k >> 24);
    out[1] = (char)(k >> 16);
    out[2] = (char)(k >> 8);
    out[3] = (char)k;
    out[4] = '\0';
}

kern_return_t smc_open(void) {
    io_service_t svc = IOServiceGetMatchingService(0, IOServiceMatching("AppleSMC"));
    if (!svc) return kIOReturnNotFound;
    kern_return_t r = IOServiceOpen(svc, mach_task_self(), 0, &conn);
    IOObjectRelease(svc);
    return r;
}

void smc_close(void) {
    IOServiceClose(conn);
}

static kern_return_t smc_call(SMCKeyData_t *in, SMCKeyData_t *out) {
    size_t outSize = sizeof(SMCKeyData_t);
    return IOConnectCallStructMethod(conn, KERNEL_INDEX_SMC, in, sizeof(*in), out, &outSize);
}

// Reads a key's type, size and raw bytes. Returns 0 on success.
static int smc_read(const char *key, UInt32 *type, UInt32 *size, unsigned char bytes[32]) {
    SMCKeyData_t in = {0}, out = {0};
    in.key = str_to_key(key);
    in.data8 = SMC_CMD_READ_KEYINFO;
    if (smc_call(&in, &out) != kIOReturnSuccess || out.result != 0) return -1;
    if (out.keyInfo.dataSize == 0 || out.keyInfo.dataSize > 32) return -1;
    *type = out.keyInfo.dataType;
    *size = out.keyInfo.dataSize;

    in.keyInfo.dataSize = out.keyInfo.dataSize;
    in.data8 = SMC_CMD_READ_BYTES;
    memset(&out, 0, sizeof(out));
    if (smc_call(&in, &out) != kIOReturnSuccess || out.result != 0) return -1;
    memcpy(bytes, out.bytes, 32);
    return 0;
}

UInt32 smc_key_count(void) {
    UInt32 type, size;
    unsigned char b[32];
    if (smc_read("#KEY", &type, &size, b) != 0 || size != 4) return 0;
    return ((UInt32)b[0] << 24) | ((UInt32)b[1] << 16) | ((UInt32)b[2] << 8) | b[3];
}

int smc_key_at(UInt32 index, char out[5]) {
    SMCKeyData_t in = {0}, res = {0};
    in.data8 = SMC_CMD_READ_INDEX;
    in.data32 = index;
    if (smc_call(&in, &res) != kIOReturnSuccess || res.result != 0) return -1;
    key_to_str(res.key, out);
    return 0;
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int smc_read_temp(const char *key, double *celsius) {
    UInt32 type, size;
    unsigned char b[32];
    if (smc_read(key, &type, &size, b) != 0) return -1;

    char t[5];
    key_to_str(type, t);

    // Signed fixed point "spIF": I integer bits, F fraction bits (hex digits), 2 bytes.
    if (t[0] == 's' && t[1] == 'p' && size == 2) {
        int ib = hexval(t[2]), fb = hexval(t[3]);
        if (ib < 0 || fb < 0 || ib + fb != 15) return -1;
        int16_t raw = (int16_t)((b[0] << 8) | b[1]);
        *celsius = raw / (double)(1 << fb);
        return 0;
    }
    // 32-bit float in host (little-endian) byte order, as used by Intel Macs with a
    // T2 chip. Unverified on T2 hardware; a wrong byte order would yield values
    // outside any sane range (or NaN), which callers filter out.
    if (strcmp(t, "flt ") == 0 && size == 4) {
        float f;
        memcpy(&f, b, 4);
        *celsius = f;
        return 0;
    }
    return -1;
}
