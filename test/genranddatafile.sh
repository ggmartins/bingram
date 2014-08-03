#!/bin/bash

if [ $# -ne 2 ]; then \
  echo "usage: $(basename $0) <size> <filename>"
  exit 1;
fi

head -c $1 </dev/urandom >$2

