#!/bin/bash
if [ -d "$BUILD_DIR" ]; then
  # ninja clang-format
  cd $BUILD_DIR
  ninja clean
  ninja
  ninja muffin.hex
  ninja muffin.bin
  ninja muffin.eep
  ninja BinSize
  ninja EepromSize
fi

