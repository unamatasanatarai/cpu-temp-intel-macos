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
| **Sanity-checked readings** | It accepts only valid `sp78` temperature keys with readings between 0 and 150 °C, so a bad or missing sensor won't produce a misleading number. |
| **Celsius or Fahrenheit** | Pass `-f` to get Fahrenheit. |
| **Reliable exit codes** | Exits `0` only when a valid temperature was written. Unknown flags, stray arguments, a missing SMC, no usable sensor and failed output writes all exit `1`, with the error on stderr. |
| **Tiny and native** | About 130 lines of C that talk to IOKit directly. It needs only Apple's built-in IOKit framework. |

## How It Works

```text
1. Open a connection to the AppleSMC service through IOKit
2. Query each CPU temperature key in priority order
3. Check that the key is a signed 8.8 fixed-point (sp78) value
4. Return the first reading between 0 and 150 °C
5. Print it to stdout in °C, or in °F with -f
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
```

To install it system-wide in `/usr/local/bin`:

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

## Make Targets

| Command | Description |
|---------|-------------|
| `make` | Build `cputemp` with `clang -O2 -Wall -Wextra` |
| `make test` | Build the binary, then run the regression suite |
| `make install` | Install the binary to `/usr/local/bin/cputemp` |
| `make clean` | Remove the built binary |

## License

Released under the [MIT License](LICENSE).
