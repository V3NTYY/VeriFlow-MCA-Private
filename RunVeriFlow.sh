#!/bin/bash

read -p "Enter the port number to use (default is 6636): " PORT
PORT=${PORT:-6636}

read -p "Enter the location of the topology file (default is ../SingleTop.topo): " TOP
TOP=${TOP:-"../SingleTop.topo"}

cd VeriFlow

expect <<EOF
spawn sudo python3 Main.py
expect "Enter network configuration file name (eg.: file.txt):"
send "$TOP\r"
expect "Enter IP address to host VeriFlow on (i.e. 127.0.0.1)"
send "127.0.0.1\r"
expect "Enter port to host VeriFlow on (i.e. 6657)"
send "$PORT\r"
interact
EOF