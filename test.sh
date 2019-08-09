#!/bin/bash

# Run the program twice, first to compress the file into "comp",
# then to decompress "comp" into "x"
./fg2019 -C $1 comp >&2 log
./fg2019 -D comp x >&2 log

# If the decompressed file "x" is the same as the original file,
# the program works correctly.
if ! diff -q x $1 &>/dev/null; then
  >&2 echo "different"
else
  >&2 echo "same"
fi