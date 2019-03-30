#!/bin/bash

make clean
make CC=/usr/local/musl/bin/musl-gcc
cd _stage
taskset -c 1 ./axhttpd
