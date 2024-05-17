# Helper Makefile for students

all:
	./waf

configure:
	./waf configure

clean:
	./waf distclean

project1ls40:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/40-ls.sce --inet-topo=./scratch/topologies/40.topo --project=1"
project1dv40:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/40-dv.sce --inet-topo=./scratch/topologies/40.topo --project=1"

project1ls29:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/29-ls.sce --inet-topo=./scratch/topologies/29.topo --project=1"
project1dv29:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/29-dv.sce --inet-topo=./scratch/topologies/29.topo --project=1"

project1ls10:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/10-ls.sce --inet-topo=./scratch/topologies/10.topo --project=1"
project1dv10:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/10-dv.sce --inet-topo=./scratch/topologies/10.topo --project=1"

testls10:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/10-ls.sce --inet-topo=./scratch/topologies/10.topo --result-check=./scratch/results/10-ls.output --project=1"
testdv10:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/10-dv.sce --inet-topo=./scratch/topologies/10.topo --result-check=./scratch/results/10-dv.output --project=1"

testls29:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/29-ls.sce --inet-topo=./scratch/topologies/29.topo --result-check=./scratch/results/29-ls.output --project=1"
testdv29:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/29-dv.sce --inet-topo=./scratch/topologies/29.topo --result-check=./scratch/results/29-dv.output --project=1"

testls40:
	./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/40-ls.sce --inet-topo=./scratch/topologies/40.topo --result-check=./scratch/results/40-ls.output --project=1"
testdv40:
	./waf --run "simulator-main --routing=DV --scenario=./scratch/scenarios/40-dv.sce --inet-topo=./scratch/topologies/40.topo --result-check=./scratch/results/40-dv.output --project=1"

project2a:
	./waf --run "simulator-main --routing=NS3 --scenario=./scratch/scenarios/m2.sce --inet-topo=./scratch/topologies/m2.topo --project=2"
project2b:
	./waf --run "simulator-main --routing=NS3 --scenario=./scratch/scenarios/m2i.sce --inet-topo=./scratch/topologies/m2.topo --project=2"