#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Print the project version.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
# The version in CMakeLists.txt is the only copy - it generates
# system/project.h, and so what the firmware reports over sysex. Naming an
# artefact means reading it, and more than one thing needs to, so it is read
# here rather than by a copy of the same expression in each of them.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

version=$(sed -n 's/^[[:space:]]*VERSION[[:space:]]\+\([0-9]\+\.[0-9]\+\.[0-9]\+\).*/\1/p' \
  "$repo_root/CMakeLists.txt" | head -1)

if [ -z "$version" ]; then
  echo "error: no VERSION found in CMakeLists.txt" >&2
  exit 1
fi

echo "$version"
