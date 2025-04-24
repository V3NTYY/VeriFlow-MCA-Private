#!/bin/bash

# Prompt the user for the base port
read -p "Enter the base port: " base_port
base_port=${base_port:-6634}
read -p "Enter the topology file location: " TOP
TOP=${TOP:-"../SingleTop.topo"}
read -p "Enter the number of topologies: " TOPn
TOPn=${TOPn:-2}

address="127.0.0.1"

# Launch each instance
poxPort=$((base_port))
flowIntPort=$((base_port+1))
vfPort=$((base_port+2))
mnPort1=$((base_port+3))
mnPort2=$((base_port+4))
offset=$((base_port % 100))

echo "Using ports: Pox: $poxPort and $flowIntPort, VeriFlow: $vfPort, Mininet1: $mnPort1 and $mnPort2"
for ((i=0; i<TOPn; i++)); do
    # Calculate the port range each CCPDN instance will use
    topoPort=$((base_port + 5 + i))
    echo "CCPDN Instance $i : $topoPort"

    if ((i == TOPn - 1)); then
        # Print total port range
       echo "Total port range: $base_port-$topoPort"
    fi
done


xterm -e "./RunPox.sh $poxPort" &
xterm -e "./RunVeriFlow.sh $TOP $vfPort" &
xterm -e "sudo python3 SingleTop.py $mnPort1 $mnPort2 $offset" &
xterm -e "./RunMCA.sh" &