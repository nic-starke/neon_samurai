# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Build environment for NEON_SAMURAI - used by CI and as a devcontainer.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
# Arch, rather than the Ubuntu the runners boot with, because of what the two
# ship for this particular toolchain:
#
#   Ubuntu 24.04   avr-gcc 7.3.0    clang 18   clang-format 18
#   Arch           avr-gcc 16.1.0   clang 22   clang-format 22
#
# Nine major versions of avr-gcc is not a detail. Checking the firmware with a
# compiler that far from the one it is flashed with means CI reports warnings
# that do not apply to the real build and misses ones that do - which is
# exactly how a bad `noreturn` on main, a clang-format that disagreed with the
# tree, and an analyser checker that had been renamed all reached main.
#
# The image is the pin. Arch itself is a rolling release, so the versions here
# are whatever was current when this was last built and pushed; rebuilding is
# a deliberate act, and until then every run uses the same toolchain.

FROM docker.io/library/archlinux:base

# --no-confirm on a full sync first: the base image's package database is a
# snapshot and installing against a stale one fails on any package that has
# moved since.
RUN pacman -Syu --noconfirm --needed \
      avr-gcc \
      base-devel \
      clang \
      cmake \
      git \
      ninja \
      python \
      python-markdown \
      python-yaml

# avr-libc is held at a specific revision rather than taken from the current
# repository.
#
# Revision 2 relocated the toolchain from /usr/avr to /usr/lib/avr, but
# avr-gcc 16.1.0-1 still resolves its search path to the old location, so the
# pair as currently published cannot link: avr-ld reports "cannot find -lc".
# Revision 1 is where avr-gcc expects to find it, and is what the firmware is
# developed against.
#
# Revisit when Arch ships an avr-gcc built for the new layout.
ARG AVR_LIBC_PKG=avr-libc-2.3.2-1-any.pkg.tar.zst
RUN pacman -U --noconfirm \
      "https://archive.archlinux.org/packages/a/avr-libc/${AVR_LIBC_PKG}" \
 && pacman -Scc --noconfirm

# Fail the image build rather than ship one that cannot link, which is how the
# mismatch above went unnoticed until CMake tried to use it.
RUN echo 'int main(void){return 0;}' > /tmp/probe.c \
 && avr-gcc -mmcu=atxmega128a4u /tmp/probe.c -o /tmp/probe \
 && rm -f /tmp/probe.c /tmp/probe

# Record what went in, so a run's log can be checked against the image it used
# without having to reproduce the build.
RUN { \
      echo "avr-gcc      $(avr-gcc -dumpversion)"; \
      echo "clang        $(clang --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
      echo "clang-format $(clang-format --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
      echo "cmake        $(cmake --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
      echo "ninja        $(ninja --version)"; \
      echo "python       $(python3 --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
    } > /etc/neon-samurai-toolchain

# The runner checks out into a directory it owns; git refuses to operate in a
# tree owned by another user unless told it is expected.
RUN git config --system --add safe.directory '*'

CMD ["/bin/bash"]
