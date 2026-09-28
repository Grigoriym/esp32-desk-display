#!/usr/bin/env bash
# Checks the enclosure model and exports it: fails if any module stand-in
# overlaps a printed part, then writes one STL per part to stl/ (gitignored)
# and preview images to renders/.
# Usage: enclosure/export.sh   (needs openscad on PATH)
set -euo pipefail
cd "$(dirname "$0")"
scad=enclosure.scad

echo "clash check"
# an empty result makes openscad exit non-zero: that is the pass case
log=$(openscad -D 'part="clash"' -o /tmp/enclosure_clash.stl "$scad" 2>&1 || true)
if ! grep -q "Current top level object is empty" <<<"$log"; then
    echo "FAIL: parts overlap a module stand-in (open /tmp/enclosure_clash.stl)" >&2
    exit 1
fi

mkdir -p stl renders
for part in shell base hood clamp test_front; do
    echo "stl/$part.stl"
    log=$(openscad -D "part=\"$part\"" -o "stl/$part.stl" "$scad" 2>&1)
    if grep -qiE "warning|error" <<<"$log"; then
        echo "$log" >&2
        exit 1
    fi
done

render() { # name, camera, extra -D args...
    local name=$1 cam=$2
    shift 2
    echo "renders/$name.png"
    openscad "$@" -o "renders/$name.png" --imgsize=1200,900 --camera="$cam" "$scad" >/dev/null 2>&1
}
render front 40,42,27,65,0,330,260
render back 40,42,27,60,0,150,260
render inside 40,42,27,50,0,300,230 -D cut=30
render section 40,42,27,90,0,270,150 --projection=o -D cut=40
echo OK
