#!/bin/bash

mkdir -p tests/asm tests/exp tests/out tests/optasm tests/optout

echo "Deleting old outputs..."
rm -f tests/out/* tests/optout/* tests/asm/* tests/optasm/*

echo "Compiling..."
for i in tests/in/*txt; do
  ./mila $i 2> /dev/null > tests/asm/$(basename $i)
done

for i in tests/in/*txt; do
  ./mila $i -O 2> /dev/null > tests/optasm/$(basename $i)
done

echo "Running interpret..."
for i in tests/asm/*txt; do
  echo -e "t\np\ng\nq" | tests/tm $i > tests/out/$(basename $i) 
done

for i in tests/asm/*txt; do
  echo -e "t\np\ng\nq" | tests/tm $i > tests/optout/$(basename $i) 
done

echo "Checking results..."
(
 cd tests/out
 err_count=0
 for i in *; do
  diff -u ../exp/$i $i > /dev/null || { ((err_count++)); echo "ERROR occured in $i"; }
 done
 cd ../optout
 for i in *; do
  diff -u ../exp/$i $i > /dev/null || { ((err_count++)); echo "ERROR occured in $i - opt"; }
 done

 echo "Test was finished with $err_count errors"
)

