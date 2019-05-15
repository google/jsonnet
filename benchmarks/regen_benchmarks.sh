#!/bin/bash

set -e
set -x

../jsonnet -S gen_big_object.jsonnet > bench.05.gen.jsonnet

for i in *.gen.jsonnet; do
	../jsonnet fmt -i "$i"
done
