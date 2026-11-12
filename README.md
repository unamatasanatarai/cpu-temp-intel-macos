# cputemp

**Your Intel Mac's CPU temperature as one clean number, ready for scripts.**

`cputemp` reads the CPU die temperature straight from the Apple System Management Controller (SMC) and prints it as a single value, like `68.6`. It has no menu bar app, no background daemon and no dependencies beyond macOS itself. It's one small native binary that answers one question.

![License: MIT](https://img.shields.io/badge/license-MIT-blue)
![Platform: macOS Intel](https://img.shields.io/badge/platform-macOS%20(Intel)-lightgrey)
![Language: C](https://img.shields.io/badge/language-C-555)

```console
$ cputemp
68.6
$ cputemp -f
155.5
```

---

## Why cputemp

macOS has no built-in command that prints the CPU temperature. The usual workarounds are GUI monitoring apps, or tools whose output is so verbose that you need `grep`, `awk` and some guessing to pull out a number.

`cputemp` prints one number and nothing else. You can drop it into a shell prompt, a status bar, a cron job or a thermal guard script without parsing anything.

## What You Get

| | |
|---|---|
| **Script-ready output** | Prints one number with one decimal place, followed by a newline. No labels, units or banners to strip out. |
| **Die temperature first** | It tries the CPU die sensors first (`TC0D`, `TC0F`, `TC0E`, `TC0H`). It only falls back to the proximity sensor `TC0P`, which can read 30–40 °C lower under load, when no die sensor is available. |
| **Sanity-checked readings** | It accepts only temperature-typed keys (signed fixed-point such as `sp78`, or 32-bit float) with readings between 0 and 150 °C, so a bad or missing sensor won't produce a misleading number. |
| **Celsius or Fahrenheit** | Pass `-f` to get Fahrenheit. |
| **Reliable exit codes** | Exits `0` only when a valid temperature was written. Unknown flags, stray arguments, a missing SMC, no usable sensor and failed output writes all exit `1`, with the error on stderr. |
| **Tiny and native** | A few hundred lines of C that talk to IOKit directly, with the SMC code shared in `smc.c`. It needs only Apple's built-in IOKit framework. |

## How It Works

```text
1. Open a connection to the AppleSMC service through IOKit
2. Query each CPU temperature key in priority order
3. Check that the key holds a temperature type: signed fixed-point (e.g. sp78) or 32-bit float
4. Return the first reading between 0 and 150 °C
5. Print it to stdout in °C, or in °F with -f
```

## See Every Sensor: `osxtemps`

When one number isn't enough, `osxtemps` walks every key the SMC exposes and lists each temperature sensor that returns a plausible reading (0–150 °C). The output has one line per sensor: the SMC key, the value, and a description for keys it recognizes.

```console
$ osxtemps
TA0P   38.1  Ambient
TB0T   34.9  Battery (max)
TC0E   77.6  CPU die (PECI, filtered)
TC0P   57.8  CPU proximity
TC1C   72.0  CPU core 1
TH0x   38.6  Drive (max)
TM0P   49.2  Memory proximity
TPCD   60.0  Platform controller hub die
Ts0P   33.4  Palm rest
...
```

It reads signed fixed-point keys (`sp78` and related types) and the 32-bit float keys used by T2 Macs (the float path is not yet verified on T2 hardware). Unknown keys are still listed, just without a description. `-f` switches to Fahrenheit. Exit codes follow the same rules as `cputemp`.

```bash
osxtemps | awk '$1 == "TPCD" { print $2 }'   # pick out one sensor
```

## Use It in Scripts

The output is a bare number, so it plugs straight into shell logic:

```bash
temp=$(cputemp) || exit 1
echo "CPU is at ${temp} °C"
```

If no reading is available, the command fails with a non-zero exit code. It never prints a wrong value, so `|| exit 1` and `set -e` work the way you would expect.

## Quick Start

> **Requirements:** An Intel-based Mac and the Xcode Command Line Tools (`clang`). Apple Silicon Macs do not expose these SMC keys, so `cputemp` reports `could not open AppleSMC (Intel Macs only)` or `no CPU temperature sensor found` there.

```bash
git clone https://github.com/unamatasanatarai/cpu-temp-intel-macos.git
cd cpu-temp-intel-macos
make
./cputemp
./osxtemps
```

To install both binaries system-wide in `/usr/local/bin`:

```bash
sudo make install
```

## Usage

```text
Usage: cputemp [-f]
  Prints the CPU temperature in Celsius, e.g. 68.6
  -f  print Fahrenheit instead
```

| Flag | Effect |
|------|--------|
| *(none)* | Print the temperature in Celsius |
| `-f` | Print the temperature in Fahrenheit |
| `-h` | Show help and exit `0` |

## Tested Behavior

`make test` runs a regression suite against the real hardware. It checks that:

- The output is a bare number in both Celsius and Fahrenheit
- Removed flags, unknown flags and stray arguments are rejected with exit `1` and empty stdout
- `-h` exits `0`
- The Celsius reading falls within a plausible 20–110 °C range
- The `-f` output matches the Celsius reading after conversion
- A closed stdout produces exit `1` instead of a false success
- Every `osxtemps` line is well-formed, `-f` lists the same sensors, and it follows the same flag and exit-code rules

## Make Targets

| Command | Description |
|---------|-------------|
| `make` | Build `cputemp` and `osxtemps` with `clang -O2 -Wall -Wextra` |
| `make test` | Build both binaries, then run the regression suite |
| `make install` | Install both binaries to `/usr/local/bin` |
| `make clean` | Remove the built binaries |

## License

Released under the [MIT License](LICENSE).
