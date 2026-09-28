#!/usr/bin/env bash
# Checks the enclosure model and exports it: fails if any module stand-in
# overlaps a printed part, then writes one STL per part to stl/<name>/
# (gitignored) and preview images to renders/<name>-*.png.
# Usage: enclosure/export.sh [file.scad] [var=value ...]   (needs openscad)
#   enclosure/export.sh                                  -> name "enclosure"
#   enclosure/export.sh enclosure_v2.scad carrier=whole  -> "enclosure_v2-whole"
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
if ! grep -q "Current top level object is empty" <<<"$log"; then
    echo "FAIL: parts overlap a module stand-in (open /tmp/enclosure_clash.stl)" >&2
    exit 1
fi

mkdir -p "stl/$name" renders
for part in shell base hood clamp test_front; do
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
echo OK
