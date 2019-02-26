!/bin/bash

cd ../transfer
rm -rf *
cd ../src
make clean
make init
make
cd ../transfer
./geniso.sh fwp_memcached.sh
