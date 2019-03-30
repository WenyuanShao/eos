#!/bin/sh

cp fwp_mng.o llboot.o
./cos_linker "llboot.o, ;echoserver_nf.o, :" ./gen_client_stub
