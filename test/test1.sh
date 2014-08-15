#!/bin/sh

echo "Running test units.."

./bingram ../test/t1 > ../test/t1.out.1.json
jsonlint -v ../test/t1.out.1.json

./bingram -v ../test/t1 | grep -v "bg_"> ../test/t1.out.2.json
jsonlint -v ../test/t1.out.2.json

./bingram -i ../test/t1 > ../test/t1.out.3.json
jsonlint -v ../test/t1.out.3.json


echo "Done"

