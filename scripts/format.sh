#!/bin/bash

# Make sure the .clang-format file is in the ROOT directory of the project (in the same directory as the src folder), then call this script from with a terminal in root
echo "Running clang-format"
find ./src -iname *.h -o -iname *.c | xargs clang-format -i -style=file
echo "Done"
