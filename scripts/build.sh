currentdir=$PWD

  # ninja clang-format
  mkdir -p /workspaces/neosam/build
  
  cd /workspaces/neosam/build

  if [[ $1 == clean ]]; then
    ninja clean
  fi
  
  if ninja; then
    ninja neosam.hex
    ninja neosam.bin
    ninja neosam.eep
    ninja BinSize
    # ninja EepromSize
  else
  echo "BUILD FAILED - Stopping here."
  exit 1
  fi

cd $currentdir