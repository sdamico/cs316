#! /bin/bash

echo ./gen_ref_seq $1 $1.ref $1.ref.ascii
./gen_ref_seq $1 $1.ref $1.ref.ascii
./gen_query_seq $1.ref $2 $1.$2.queries $1.$2.queries.ascii
echo ./gen_query_seq $1.ref $2 $1.$2.queries $1.$2.queries.ascii
echo ./gen_tables $1.ref $3 $1.$3.interval $1.$3.position $1.$3.interval.ascii $1.$3.position.ascii
./gen_tables $1.ref $3 $1.$3.interval $1.$3.position $1.$3.interval.ascii $1.$3.position.ascii

mv $1.* ../../baseline/bin
