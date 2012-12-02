#generating the tables:

mkdir -p ./baseline/exact//baseline/exact/data
mkdir -p ./baseline/exact//baseline/exact/out

./tools/bin/gen_query_seq /baseline/exact/data/Chr1.ref 100 100000 /baseline/exact/data/Chr1.100.q
./tools/bin/gen_query_seq /baseline/exact/data/Chr1.ref 200 100000 /baseline/exact/data/Chr1.200.q
./tools/bin/gen_query_seq /baseline/exact/data/Chr1.ref 400 100000 /baseline/exact/data/Chr1.400.q

./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 5 /baseline/exact/data/Chr1.5.it /baseline/exact/data/Chr1.5.pt
./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 7 /baseline/exact/data/Chr1.7.it /baseline/exact/data/Chr1.7.pt
./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 9 /baseline/exact/data/Chr1.9.it /baseline/exact/data/Chr1.9.pt
./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 11 /baseline/exact/data/Chr1.11.it /baseline/exact/data/Chr1.11.pt
./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 13 /baseline/exact/data/Chr1.13.it /baseline/exact/data/Chr1.13.pt
./tools/bin/gen_tables /baseline/exact/data/Chr1.ref 15 /baseline/exact/data/Chr1.15.it /baseline/exact/data/Chr1.15.pt

#run benchmarks

./baseline/exact/bin/baseline 5 /baseline/exact/data/Chr1.5.it /baseline/exact/data/Chr1.5.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.5
./baseline/exact/bin/baseline 7 /baseline/exact/data/Chr1.7.it /baseline/exact/data/Chr1.7.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.7
./baseline/exact/bin/baseline 9 /baseline/exact/data/Chr1.9.it /baseline/exact/data/Chr1.9.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.9
./baseline/exact/bin/baseline 11 /baseline/exact/data/Chr1.11.it /baseline/exact/data/Chr1.11.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.11
./baseline/exact/bin/baseline 13 /baseline/exact/data/Chr1.13.it /baseline/exact/data/Chr1.13.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.13
./baseline/exact/bin/baseline 15 /baseline/exact/data/Chr1.15.it /baseline/exact/data/Chr1.15.pt /baseline/exact/data/Chr1.100.q > /baseline/exact/out/Chr1.100.15

