#!/bin/sh
json_root=$PWD
git clone https://github.com/json-c/json-c.git src
cd src
./autogen.sh --configure --prefix=$json_root
make; make check; make install
cd -
