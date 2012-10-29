#!/bin/sh

sed 's/ctr=//;s/e=//;s/u=//;s/w=//;s/Lost sync/0 0 0 0\n0 0 0 0/;/Resynchronized/d;/Miosix/d;/Starting/d;/Available/d;/#node.\.bin/d;/ (miss)/d' "$1" > "$1.dat"

