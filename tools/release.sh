#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Bump the project version and tag a release.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
#   tools/release.sh patch     1.1.0 -> 1.1.1
#   tools/release.sh minor     1.1.0 -> 1.2.0
#   tools/release.sh major     1.1.0 -> 2.0.0
#   tools/release.sh 1.4.2     set it outright
#
# The version in CMakeLists.txt is the only copy - it is what generates
# system/project.h, and so what the firmware reports over sysex. Pushing the
# tag is what publishes the release; this script stops short of that.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [ $# -ne 1 ]; then
  sed -n '9,17p' "$0" | sed 's/^# \?//'
  exit 1
fi

# tools/version.sh is the one place that knows how to read the version, and it
# fails loudly if there is not one. Duplicating the expression here is how the
# two drift apart.
current=$(tools/version.sh)

IFS=. read -r major minor patch <<<"$current"

case "$1" in
  major) next="$((major + 1)).0.0" ;;
  minor) next="${major}.$((minor + 1)).0" ;;
  patch) next="${major}.${minor}.$((patch + 1))" ;;
  [0-9]*.[0-9]*.[0-9]*) next="$1" ;;
  *)
    echo "error: expected major, minor, patch, or an explicit version" >&2
    exit 1
    ;;
esac

if [ -n "$(git status --porcelain)" ]; then
  echo "error: the working tree has uncommitted changes" >&2
  git status --short >&2
  exit 1
fi

if git rev-parse -q --verify "refs/tags/v${next}" >/dev/null; then
  echo "error: tag v${next} already exists" >&2
  exit 1
fi

echo "$current -> $next"

sed -i "0,/^\([[:space:]]*VERSION[[:space:]]\+\)${current}/s//\1${next}/" CMakeLists.txt

confirm=$(tools/version.sh || true)
if [ "$confirm" != "$next" ]; then
  echo "error: the version in CMakeLists.txt did not update cleanly" >&2
  git checkout -- CMakeLists.txt
  exit 1
fi

git add CMakeLists.txt
git commit -m "release: version ${next}"
git tag -a "v${next}" -m "NEON_SAMURAI ${next}"

echo
echo "Tagged v${next}. To publish:"
echo "  git push origin main --follow-tags"
