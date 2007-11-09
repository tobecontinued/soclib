#!/bin/sh

cd $(dirname $0)/..
svn up
export PATH=$PWD/bin:$PATH
cd testcase
make clean
make || exit 1
exit 0
