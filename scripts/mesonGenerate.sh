if [ ! -d "$BUILD_DIR" ]; then
  mkdir $BUILD_DIR
fi

# A possible fix for clock-skew/out of sync errors.
# Make sure to delete the build folder first, you may also need to execute the following line prior to first build
# find . -type f | xargs touch

meson --cross-file cross-file.txt $BUILD_DIR