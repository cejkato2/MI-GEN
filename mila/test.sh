#!/bin/bash

mkdir -p tests/asm tests/exp tests/out

for i in tests/in/*txt; do
  ./mila $i -O 2> /dev/null > tests/asm/$(basename $i)
done

set -x
for i in tests/asm/*txt; do
  echo -e "t\np\ng\nq" | tests/tm $i > tests/out/$(basename $i) 
done


