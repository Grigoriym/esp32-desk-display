#!/usr/bin/env bash
# Checks the enclosure model and exports it: fails if any module stand-in
# overlaps a printed part, then writes one STL per part to stl/<name>/
# (gitignored) and preview images to renders/<name>-*.png.
# Usage: enclosure/export.sh [file.scad] [var=value ...]   (needs openscad)
#   enclosure/export.sh                                  -> name "enclosure"
#   enclosure/export.sh enclosure.scad cut=40            -> "enclosure-40"
set -euo pipefail
cd "$(dirname "$0")"
scad=${1:-enclosure.scad}
shift || true
name=$(basename "$scad" .scad)
defs=()
for kv in "$@"; do
    defs+=(-D "${kv%%=*}=\"${kv#*=}\"")
    name+="-${kv#*=}"
done

echo "clash check"
# an empty result makes openscad exit non-zero: that is the pass case
log=$(openscad "${defs[@]}" -D 'part="clash"' -o /tmp/enclosure_clash.stl "$scad" 2>&1 || true)
if grep -qE "WARNING: (Ignoring unknown|undefined operation|Unable to convert)" <<<"$log"; then
    echo "$log" | grep WARNING >&2
    echo "FAIL: model warnings (a variable used before it's defined?)" >&2
    exit 1
fi
if grep -q "ERROR:" <<<"$log"; then
    echo "$log" | grep -E "ERROR:" >&2
    echo "FAIL: a check in the model failed" >&2
    exit 1
fi
grep -o "CHECKS: .*" <<<"$log" | sed 's/"$//' || true
echo "values not measured (ASSUMED in $scad):"
grep -n "ASSUMED" "$scad" | grep -v "^[0-9]*:// Sizes\|marked ASSUMED" | sed 's/^/  /' || true
if ! grep -q "Current top level object is empty" <<<"$log"; then
    echo "FAIL: parts overlap (a stand-in or each other; open /tmp/enclosure_clash.stl)" >&2
    exit 1
fi

# wiped first, so parts the model no longer has don't linger
rm -rf "stl/$name"
mkdir -p "stl/$name" renders
# the parts to print: the model's Customizer list for `part`, minus views
parts=$(sed -n 's/^part = .*\/\/ \[\(.*\)\].*/\1/p' "$scad" | tr -d ' ' | tr ',' '\n' | grep -vxE "assembly|clash|dims")
for part in $parts; do
    echo "stl/$name/$part.stl"
    log=$(openscad "${defs[@]}" -D "part=\"$part\"" -o "stl/$name/$part.stl" "$scad" 2>&1)
    if grep -qiE "warning|error" <<<"$log"; then
        echo "$log" >&2
        exit 1
    fi
done

render() { # view, camera, extra openscad args...
    local view=$1 cam=$2
    shift 2
    echo "renders/$name-$view.png"
    openscad "${defs[@]}" "$@" -o "renders/$name-$view.png" --imgsize=1200,900 --camera="$cam" "$scad" >/dev/null 2>&1
}
render front 40,42,27,65,0,330,260
render back 40,42,27,60,0,150,260
render inside 40,42,27,50,0,300,230 -D cut=30
render section 40,42,27,90,0,270,150 --projection=o -D cut=40
if grep -q "^explode = " "$scad"; then
    render exploded 40,42,45,65,0,320,330 -D explode=40 -D explode_front=30 -D show_labels=false
fi
echo OK
