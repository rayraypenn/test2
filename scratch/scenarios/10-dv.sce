* LS VERBOSE ALL OFF
* DV VERBOSE ALL OFF
* APP VERBOSE ALL OFF
* LS VERBOSE STATUS OFF
* LS VERBOSE ERROR OFF
* DV VERBOSE STATUS ON
* DV VERBOSE ERROR OFF
* APP VERBOSE STATUS OFF
* APP VERBOSE ERROR OFF
* DV VERBOSE TRAFFIC OFF
* APP VERBOSE TRAFFIC OFF

# Advance Time pointer by 60 seconds. Allow the routing protocol to stabilize.
TIME 60000

# Bring down Link Number 6.
LINK DOWN 6
TIME 10000

# Bring up Link Number 6.
LINK UP 6
TIME 10000

# Bring down all links of node 1
NODELINKS DOWN 1
TIME 10000

# Bring up all links of node 1
NODELINKS UP 1
TIME 10000

# Bring down link(s) between nodes 1 and 8
LINK DOWN 1 8
TIME 10000

# Bring up link(s) between nodes 1 and 8
LINK UP 1 8
TIME 10000

#TEST1 Dump Distance Vector Neighbor Table.
1 DV DUMP NEIGHBORS

#TEST2 Dump Distance Vector Routing Table.
1 DV DUMP ROUTES

# Quit the simulator
QUIT
