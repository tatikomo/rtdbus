#!/bin/bash

WORKDIR=${HOME}/ITG/sandbox/rtdbus

cat ${WORKDIR}/xdb/impl/dat/attr_list.txt \
| grep -v ^# \
| while read a; \
do \
  types=$(grep $a ${WORKDIR}/xdb/rtap/dat/dict/*.dat | awk '{print $4}' | sort | uniq); \
  for x in $types; do echo "\"$a\", $x"; done \
done
