# [SPARQL Querying Benchmarks](https://sites.google.com/site/sqbenchmarks/)
## Materials for hands-on session

## [BSBM: Berlin SPARQL Benchmark](http://wifo5-03.informatik.uni-mannheim.de/bizer/berlinsparqlbenchmark/)

Running data generator:
```
./generate --help
./generate -s nt -pc 1000 -fc -dir td_data -fn dataset -nof 1 -ud -tc 1000 -ppt 1
```

Running virtuoso docker with test data:
```
mkdir -p db/toLoad
cp dataset.nt db/toLoad
docker run -it --name bsbm-virtuoso-benchmark -p 8890:8890 -p 1111:1111 -e DBA_PASSWORD=dba -e SPARQL_UPDATE=true -e DEFAULT_GRAPH=http://bsbm/ -v $(pwd)/db:/data tenforce/virtuoso:1.1.0-virtuoso7.2.4
```

Running SPARQL benchmark:
```
./testdriver -runs 500 -idir td_data -dg http://bsbm/ -mt 5 -u http://localhost:8890/sparql -udataset dataset_update.nt http://localhost:8890/sparql
```

## SP2Bench

Running data generator:
```
./sp2b_gen --help
./sp2b_gen -t 100000 dataset.n3
```

Running virtuoso docker with test data:
```
mkdir -p db/toLoad
cp dataset.n3 db/toLoad
docker run -it --name virtuoso-sp2bench -p 8890:8890 -p 1111:1111 -e DBA_PASSWORD=dba -e SPARQL_UPDATE=true -e DEFAULT_GRAPH=http://sp2bench/ -v $(pwd)/db:/data tenforce/virtuoso:1.1.0-virtuoso7.2.4
```

Running SPARQL benchmark (need IGUANA benchmark):
```
```

## LUBM

The original generator does not include run instructions, we will use modified (and improved) version from [rvesse lubm-uba repository](https://github.com/rvesse/lubm-uba)

Running generator:
```
mvn package
./generate.sh --help
mkdir -p generated/
./generate.sh --quiet --timing --compress -u 1 --consolidate Full  --format NTRIPLES --threads 8 -o generated/
```

Running virtuoso docker with test data:
```
mkdir -p db/toLoad
cp generated/* db/toLoad
docker run -it --name virtuoso-lubm -p 8890:8890 -p 1111:1111 -e DBA_PASSWORD=dba -e SPARQL_UPDATE=true -e DEFAULT_GRAPH=http://lubm/ -v $(pwd)/db:/data tenforce/virtuoso:1.1.0-virtuoso7.2.4
```

Running SPARQL benchmark (need IGUANA benchmark):
```
```


## WatDiv
## DBPSB
## FEASIBLE
