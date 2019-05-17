#!/bin/bash

wget https://www.python.org/ftp/python/3.6.6/Python-3.6.6.tar.xz
tar xf Python-3.6.6.tar.xz 
cd Python-3.6.6/
./configure --prefix $(readlink -f $PWD/../../)
make -j8
make install
cd ../../
# this is a bit ugly but only need once
module load OpenSSL
LD_PRELOAD=$EBROOTOPENSSL/lib/libssl.so bin/pip3 install globus-cli
module unload OpenSSL
bin/globus whoami
