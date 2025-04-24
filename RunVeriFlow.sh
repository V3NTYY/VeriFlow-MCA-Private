#!/bin/bash

# read -p "Enter the port number to use (default is 6636): " PORT
PORT=$2

# read -p "Enter the location of the topology file (default is ../SingleTop.topo): " TOP
TOP=$1

address="127.0.0.1"

cd VeriFlow

sudo python3 Main.py $TOP $address $PORT