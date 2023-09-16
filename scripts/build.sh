#!/bin/bash
if [ -d "$BUILD_DIR" ]; then
  # ninja clang-format
  cd $BUILD_DIR

  if [[ $1 == clean ]]; then
    ninja clean
  fi
  
  if ninja; then
    ninja muffin.hex
    ninja muffin.bin
    ninja muffin.eep
    ninja BinSize
    # ninja EepromSize
  else
  echo "BUILD FAILED - Stopping here."

  fi
fi

