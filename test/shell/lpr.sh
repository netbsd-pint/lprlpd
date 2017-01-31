#! /usr/pkg/bin/bash

echo "Attempting to print a txt file"
lpr t1.txt

echo "Print 2 copies of a txt file"
lpr -#2 t1.txt

echo "Print with lpr via pipes!"
cat t1.txt | lpr

echo "Try to print a file that does not exist. SHOULD FAIL."
lpr dne.txt
