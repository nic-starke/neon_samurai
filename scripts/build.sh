currentdir=$PWD

  # ninja clang-format
  mkdir -p /workspaces/muffin/build
  
  cd /workspaces/muffin/build

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
  exit 1
  fi

cd $currentdir