#!/bin/sh
bingram_root=$PWD
cd json
sh ./autobuild.sh
cd $bingram_root

./autogen.sh --configure 
make
