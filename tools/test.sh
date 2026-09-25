#!/usr/bin/env bash
# Builds and runs the host (PC) unit tests in test/ with gcc: pure logic from
# main/ (parsers, mappings), no ESP32 needed. The `./gradlew test` of this repo.
# Needs IDF_PATH (for the Unity test framework ESP-IDF ships) and
# managed_components/ (for cJSON; created by the first `idf.py build`).
set -euo pipefail
cd "$(dirname "$0")/.."

: "${IDF_PATH:?source ~/esp/esp-idf/export.sh first (Unity comes from ESP-IDF)}"
UNITY="$IDF_PATH/components/unity/unity/src"
CJSON=managed_components/espressif__cjson/cJSON
[ -f "$CJSON/cJSON.c" ] || { echo "no $CJSON: run 'idf.py build' (or 'idf.py reconfigure') once" >&2; exit 2; }

OUT=build/host_tests
mkdir -p "$OUT"
CFLAGS=(-std=gnu17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer
        -Imain -Itest -I"$UNITY" -I"$CJSON" -DFIXTURES_DIR="\"$PWD/test/fixtures\"")

# test_<name>.c tests main/<name>.c.
failed=0
for t in test/test_*.c; do
    name=$(basename "$t" .c)
    src="main/${name#test_}.c"
    gcc "${CFLAGS[@]}" "$t" "$src" "$UNITY/unity.c" "$CJSON/cJSON.c" -lm -o "$OUT/$name"
    echo "== $name"
    "$OUT/$name" || failed=1
done
exit $failed
