currentdir=$PWD

  # ninja clang-format
  mkdir -p ../_build

  cd ../_build

  if [[ $1 == clean ]]; then
    ninja clean
  fi
  
  if ninja; then
    ninja neon_samurai.hex
    ninja neon_samurai.bin
    ninja neon_samurai.eep
    ninja BinSize
    # ninja EepromSize
  else
  echo "BUILD FAILED - Stopping here."
  exit 1
  fi

cd $currentdir