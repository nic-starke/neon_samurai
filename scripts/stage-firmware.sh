#!/usr/bin/env bash
#
# Stage a locally built firmware for the editor to offer, so the update flow
# can be tested without publishing a release.
#
# Writes the same firmware/index.json that .github/workflows/pages.yml writes
# at deploy time, so what is tested here is what users get.
#
#   scripts/stage-firmware.sh [version]
#
# Pass a version to label the build as something other than the project
# version - useful because the editor only offers an update when the staged
# version is newer than the one on the device.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

build="build/Release"
hex="$build/neosam.hex"

if [ ! -d "$build" ]; then
	echo "error: $build does not exist - configure it first" >&2
	exit 1
fi

cmake --build "$build" >/dev/null

if [ ! -f "$hex" ]; then
	echo "error: $hex was not produced" >&2
	exit 1
fi

version="${1:-$(sed -n 's/^  VERSION \([0-9.]*\)/\1/p' CMakeLists.txt | head -1)}"

if [ -z "$version" ]; then
	echo "error: could not determine a version" >&2
	exit 1
fi

# The version the firmware reports comes from the record compiled into it, not
# from the manifest. Labelling a build as something it will not report makes the
# update appear to finish on a version nothing is actually running.
built=""
if [ -f "$build/neosam.bin" ]; then
	built="$(python3 -c 'import sys,pathlib
b = pathlib.Path(sys.argv[1]).read_bytes()
i = b.find(b"NEON_SAMURAI")
print("%d.%d.%d" % (b[i+28], b[i+29], b[i+30]) if i > 0 else "")' "$build/neosam.bin")"
fi

if [ -n "$built" ] && [ "$built" != "$version" ]; then
	echo "warning: staging as $version, but this firmware reports $built." >&2
	echo "         The update will finish showing $built, not $version." >&2
	echo "         To test against a matching build, stage the project version" >&2
	echo "         and add ?forceUpdate to the editor URL." >&2
fi

name="neosam-$version.hex"

mkdir -p firmware

# Only ever remove what this script itself stages. The recovery bootloader
# lives in this directory too, and it is the only way back from a bad flash.
rm -f firmware/neosam-*.hex
cp "$hex" "firmware/$name"

cat > firmware/index.json <<JSON
{
  "version": "$version",
  "tag": "v$version",
  "file": "../firmware/$name",
  "sha256": "$(sha256sum "firmware/$name" | cut -d' ' -f1)",
  "size": $(stat -c%s "firmware/$name")
}
JSON

echo "staged $name ($(stat -c%s "firmware/$name") bytes)"
echo
echo "Serve the repository root - the editor reads ../firmware/index.json,"
echo "so serving webui/ alone puts the manifest above the document root:"
echo
echo "  python3 -m http.server 8420 --bind 127.0.0.1"
echo "  \$BROWSER http://127.0.0.1:8420/webui/index.html"
