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

# osxtemps: every line is "KEY  VALUE[  description]", and it shares cputemp's contract
TBIN=${TBIN:-./osxtemps}
out=$($TBIN); rc=$?
bad_lines=$(grep -cvE '^T[A-Za-z0-9]{3}  +[0-9]+\.[0-9]( {2}.+)?$' <<<"$out")
[[ $rc == 0 && -n $out && $bad_lines == 0 ]] && pass "osxtemps lists $(wc -l <<<"$out" | tr -d ' ') sensors" || bad "osxtemps exit $rc, $bad_lines malformed lines"
# Separate runs can differ if a sensor flickers to an invalid value, so allow one key of drift
keys_c=$(cut -d' ' -f1 <<<"$out" | sort); keys_f=$($TBIN -f | cut -d' ' -f1 | sort)
drift=$(comm -3 <(echo "$keys_c") <(echo "$keys_f") | grep -c .)
[[ $drift -le 1 ]] && pass "osxtemps -f lists the same sensors (drift $drift)" || bad "osxtemps -f key drift $drift"
for args in "-x" "foo" "-f foo"; do
    out=$($TBIN $args 2>/dev/null); rc=$?
    [[ $rc == 1 && -z $out ]] && pass "osxtemps '$args' rejected" || bad "osxtemps '$args' exit $rc output '$out'"
done
$TBIN -h >/dev/null 2>&1 && pass "osxtemps -h exits 0" || bad "osxtemps -h exit $?"
$TBIN >&- 2>/dev/null; rc=$?
[[ $rc == 1 ]] && pass "osxtemps closed stdout exits 1" || bad "osxtemps closed stdout exit $rc"

exit $fail
