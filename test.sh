#!/bin/bash
# Regression tests for cputemp. Run: make test
BIN=${BIN:-./cputemp}
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }

# Output is a bare number like 68.6, exit 0
out=$($BIN); rc=$?
[[ $rc == 0 && $out =~ ^[0-9]+\.[0-9]$ ]] && pass "prints bare number ($out)" || bad "output '$out' exit $rc"
out=$($BIN -f); rc=$?
[[ $rc == 0 && $out =~ ^[0-9]+\.[0-9]$ ]] && pass "-f prints bare number ($out)" || bad "-f output '$out' exit $rc"

# Removed flags, unknown flags and stray arguments are rejected with exit 1 and no stdout
for args in "-w" "-i 1" "-x" "foo" "-f foo"; do
    out=$($BIN $args 2>/dev/null); rc=$?
    [[ $rc == 1 && -z $out ]] && pass "'$args' rejected" || bad "'$args' exit $rc output '$out'"
done

$BIN -h >/dev/null 2>&1 && pass "-h exits 0" || bad "-h exit $?"

# F3: value is physically plausible, and -f is the Celsius value converted
c=$($BIN); f=$($BIN -f)
awk -v c="$c" 'BEGIN { exit !(c >= 20 && c <= 110) }' && pass "Celsius in 20..110 ($c)" || bad "Celsius out of range: $c"
awk -v c="$c" -v f="$f" 'BEGIN { d = f - (c * 9 / 5 + 32); exit !(d > -3 && d < 3) }' \
    && pass "-f matches Celsius ($c C -> $f F)" || bad "-f mismatch: $c C vs $f F"

# F2: a failed write must not report success
$BIN >&- 2>/dev/null; rc=$?
[[ $rc == 1 ]] && pass "closed stdout exits 1" || bad "closed stdout exit $rc"

exit $fail
