#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
CORE="$ROOT/NativeCore"; BUILD="$CORE/build/host"
cmake -S "$CORE" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --config Release
ctest --test-dir "$BUILD" -C Release --output-on-failure
case "$(uname -s)" in
 MINGW*|MSYS*|CYGWIN*) SOURCE="$(find "$BUILD" -type f -name 'WildboundCore.dll' -print -quit)"; DEST="$ROOT/Assets/Plugins/x86_64"; mkdir -p "$DEST"; cp "$SOURCE" "$DEST/WildboundCore.dll" ;;
 Linux*) SOURCE="$(find "$BUILD" -type f -name 'libWildboundCore.so' -print -quit)"; DEST="$ROOT/Assets/Plugins/Linux/x86_64"; mkdir -p "$DEST"; cp "$SOURCE" "$DEST/libWildboundCore.so" ;;
 Darwin*) SOURCE="$(find "$BUILD" -type f -name 'libWildboundCore.dylib' -print -quit)"; DEST="$ROOT/Assets/Plugins/macOS"; mkdir -p "$DEST"; cp "$SOURCE" "$DEST/libWildboundCore.dylib" ;;
 *) echo "Built core; plugin installation unsupported for this host." >&2 ;;
esac
echo "Native C++ core built and tests passed."
