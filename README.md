# CIS553 Project Starter Code

This code base makes use of the [NS3](https://www.nsnam.org/) simulator.

## Repo structure

```bash
.
├── Makefile
├── README.md
├── VERSION
├── bindings
├── contrib
│   ├── upenn-cis553
│   │   ├── common
│   │   │   ├── penn-application.cc
│   │   │   ├── penn-application.h
│   │   │   ├── penn-log.cc
│   │   │   ├── penn-log.h
│   │   │   ├── penn-routing-protocol.cc
│   │   │   ├── penn-routing-protocol.h
│   │   │   ├── ping-request.cc
│   │   │   ├── ping-request.h
│   │   │   ├── test-result.cc
│   │   │   └── test-result.h
│   │   ├── dv-routing-protocol
│   │   │   ├── dv-message.cc
│   │   │   ├── dv-message.h
│   │   │   ├── dv-routing-helper.cc
│   │   │   ├── dv-routing-helper.h
│   │   │   ├── dv-routing-protocol.cc
│   │   │   └── dv-routing-protocol.h
│   │   ├── keys
│   │   │   ├── metadata0.keys
│   │   │   ├── metadata1.keys
│   │   │   ├── metadata2.keys
│   │   │   ├── metadata3.keys
│   │   │   ├── metadata4.keys
│   │   │   └── metadata5.keys
│   │   ├── ls-routing-protocol
│   │   │   ├── ls-message.cc
│   │   │   ├── ls-message.h
│   │   │   ├── ls-routing-helper.cc
│   │   │   ├── ls-routing-helper.h
│   │   │   ├── ls-routing-protocol.cc
│   │   │   └── ls-routing-protocol.h
│   │   ├── penn-search
│   │   │   ├── penn-chord-message.cc
│   │   │   ├── penn-chord-message.h
│   │   │   ├── penn-chord.cc
│   │   │   ├── penn-chord.h
|   |   |   ├── penn-key-helper.h
│   │   │   ├── penn-search-helper.cc
│   │   │   ├── penn-search-helper.h
│   │   │   ├── penn-search-message.cc
│   │   │   ├── penn-search-message.h
│   │   │   ├── penn-search.cc
│   │   │   └── penn-search.h
│   │   ├── test-app
│   │   │   ├── test-app-helper.cc
│   │   │   ├── test-app-helper.h
│   │   │   ├── test-app-message.cc
│   │   │   ├── test-app-message.h
│   │   │   ├── test-app.cc
│   │   │   └── test-app.h
│   │   └── wscript
│   └── wscript
├── doc
├── scratch
│   ├── results
│   │   ├── 10-dv.output
│   │   ├── 10-ls.output
│   │   ├── 29-dv.output
│   │   ├── 29-ls.output
│   │   ├── 40-dv.output
│   │   └── 40-ls.output
│   ├── scenarios
│   │   ├── 10-dv.sce
│   │   ├── 10-ls.sce
│   │   ├── 29-dv.sce
│   │   ├── 29-ls.sce
│   │   ├── 40-dv.sce
│   │   ├── 40-ls.sce
│   │   ├── m2.sce
│   │   ├── m2i.sce
│   │   ├── penn-chord.sce
│   │   ├── penn-search.sce
│   │   └── test.sce
│   ├── simulator-main.cc
│   └── topologies
│       ├── 10.topo
│       ├── 29.topo
│       ├── 40.topo
│       ├── m2.topo
│       └── small.topo
├── src
├── test.py
├── utils
├── utils.py
├── waf
├── waf-tools
├── waf.bat
├── wscript
└── wutils.py
```

## Code base explanation

This entire repo is a custom NS3 program, written to simulate the DV/LS routing protocols, as well as your Chord and PennSearch implementations. The source code for the simulator can be found in the `simulator-main.cc` file under the `scratch` folder. You will not need to make any changes to that file.

All student submitted code will go under the `contrib` folder, which includes the `common`, `dv-routing-procotol`, `ls-routing-protocol`, `keys`, `penn-search` and `test-app` directories. When working on Project 1, you will make changes to the files under `dv-routing-procotol` or `ls-routing-protocol`, depending on which one your sub-group is implementing. In Project 2, you will work in the `penn-search` directory.

The `scratch` folder includes the aforementioned `simulator-main.cc` file and, more importantly, the files used to specify network topologies and scenarios. The `results` folder is used for local autogrtader testing in Project 1 _only_. Project 2 is autograded, but that autograder cannot be run locally.

The `src` directory includes the NS3 source code, broken down into sub-modules. Students to not have to modify anything in that directory - or any other directories for that matter, other than the ones mentioned above.

## Compiling and running the simulator

We assume you are familiar with basic git operations and have this repo cloned and locally available on your machine.

### Compiling

1. You need to run `./waf configure` if you're running the simulator for the first time. It may be that the `waf` executable does not have execution permissions on your machine. You can change that by running `chmod u+x waf`.
2. Once the step above completes, you will need to compile the simulator. You can do that bu running `./waf`. This may take a few minutes to run, depending on your machine.

### Running

You have two ways of running the simulator. We will focus on the more common way, which is via the [WAF](https://waf.io/) build system. It is important to disambiguate WAF from NS3. They are two separate projects, but NS3 uses WAF as its build system, and we recommend using it, too.

The recommended way of running the simulator is as as follows:

`./waf --run "simulator-main --routing=<NS3/LS/DV> --scenario=./scratch/scenarios/<SCENARIO_FILE_NAME>.sce --inet-topo=./scratch/topologies/<TOPOLOGY_FILE_NAME>.topo --project=<1/2>"`

So if you wanted to run the LS-40 simulation with your LS routing implementation in Project 1, you would use the following command:

`./waf --run "simulator-main --routing=LS --scenario=./scratch/scenarios/40-ls.sce --inet-topo=./scratch/topologies/40.topo --project=1"`

And here is an example for running Project 2 with the "m2" scenario and topology files and built-in NS3 routing:

`./waf --run "simulator-main --routing=NS3 --scenario=./scratch/scenarios/m2.sce --inet-topo=./scratch/topologies/m2.topo --project=2"`

## Troubleshooting

If at any point you are getting compiling errors, try cleaning the build cache of the NS3 simulator by running `./waf distclean`. This will mean that `waf` will need to recompile all source code the next time it runs.

Tips on using the provided hashing functions in `penn-key-helper.h`:

- Hash a node using it's `ns3::Ipv4` address
- Hash a lookup term using a `std::string`
- Only use `PennKeyHelper::KeyToHexString()` for printing
