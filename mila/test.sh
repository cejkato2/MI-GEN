#!/bin/bash

mkdir -p tests/asm tests/exp tests/out

rm -f tests/out/*

for i in tests/in/*txt; do
  ./mila $i -O 2> /dev/null > tests/asm/$(basename $i)
done

for i in tests/asm/*txt; do
  echo -e "t\np\ng\nq" | tests/tm $i > tests/out/$(basename $i) 
done

(
 cd tests/out
 err_count=0
 for i in *; do
  diff -u ../exp/$i $i > /dev/null || { ((err_count++)); echo "ERROR occured in $i"; }
 done

 echo "Test was finished with $err_count errors"
)

