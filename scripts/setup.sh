#!/usr/bin/env bash

meson subprojects download
meson subprojects packagefiles --apply
