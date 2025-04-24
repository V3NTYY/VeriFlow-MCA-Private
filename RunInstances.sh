#!/bin/bash

# Prompt the user for the base port
read -p "Enter the base port: " base_port
base_port=${base_port:-6634}
read -p "Enter the topology file location: " TOP
TOP=${TOP:-"../SingleTop.topo"}
read -p "Enter the number of topologies: " TOPn
TOPn=${TOPn:-2}
read -p "Launch mininet? (y/n): " launch_mn
launch_mn=${launch_mn:-y}

address="127.0.0.1"

# Launch each instance
poxPort=$((base_port))
flowIntPort=$((base_port+1))
vfPort=$((base_port+2))
offset=$((base_port % 100))

echo "PORTS:"
echo "Pox: $poxPort"
echo "FlowInterface $flowIntPort"
echo "VeriFlow: $vfPort"
echo "Mininet: Connects to $poxPort, additional controller hosted on $mnPort1"

for ((i=0; i<TOPn; i++)); do
    # Calculate the port range each CCPDN instance will use
    topoPort=$((base_port + 3 + i))
    echo "CCPDN Instance [$i] is on $topoPort"

    if ((i == TOPn - 1)); then
        # Print total port range
       echo "[PORT RANGE]: $base_port-$topoPort"
    fi
done

mnPort1=$((topoPort+1))

# Free calculated ports by killing processes using them
ports=($poxPort $flowIntPort $vfPort $mnPort1)
for ((i=0; i<TOPn; i++)); do
    topoPort=$((base_port + 3 + i))
    ports+=($topoPort)
done

echo "Freeing ports: ${ports[@]}"
for port in "${ports[@]}"; do
    pid=$(lsof -t -i:$port 2>/dev/null)
    if [[ -n $pid ]]; then
        echo "Killing process $pid using port $port"
        kill -9 $pid
    fi
done

echo "Killing existing Main.py processes..."
ps aux | grep "Main.py" | grep -v "grep" | awk '{print $2}' | sudo xargs kill -9

xterm -e "./RunPox.sh $poxPort" &
xterm -e "./RunVeriFlow.sh $TOP $vfPort" &
xterm -e "./RunMCA.sh" &

if [[ $launch_mn == "y" ]]; then
    xterm -e "sudo python3 SingleTop.py $poxPort $mnPort1 $offset" &
fi