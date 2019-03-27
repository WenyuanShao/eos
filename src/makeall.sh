!/bin/bash

cd ../transfer
rm -rf *
cd ../src
make clean
make init
make
cd ../transfer
./geniso.sh echo_nf.sh
