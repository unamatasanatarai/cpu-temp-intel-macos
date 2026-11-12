// smc.h - minimal read-only access to the Apple System Management Controller.
#ifndef SMC_H
#define SMC_H

#include <IOKit/IOKitLib.h>

// Opens/closes the AppleSMC connection. smc_open returns kIOReturnSuccess on success.
kern_return_t smc_open(void);
void smc_close(void);

// Number of keys the SMC exposes (from "#KEY"), or 0 on failure.
UInt32 smc_key_count(void);

// Writes the 4-character key at `index` into out[5]. Returns 0 on success.
int smc_key_at(UInt32 index, char out[5]);

// Reads a temperature key. Accepts signed fixed-point types (sp78, sp87, ...)
// and 4-byte floats ("flt "). Returns 0 and stores °C in *celsius on success,
// -1 if the key is missing or is not a temperature-shaped type.
int smc_read_temp(const char *key, double *celsius);

#endif
