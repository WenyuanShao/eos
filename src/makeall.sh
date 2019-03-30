!/bin/bash

cd ../transfer
rm -rf *
cd ../src
make clean
#make init
#cp components/lib/axtls/_stage/libaxtls.a components/lib
cp -r components/lib/dpdk/build/lib/* components/lib
cp components/lib/click/libclick.a components/lib
make
cd ../transfer
./geniso.sh echo_nf.sh
