#!/bin/bash
read -p "Enter the port number to use (default is 6634): " PORT
PORT=${PORT:-6653}

cp FlowInterface.py ~/pox/pox/
cd ~/pox
./pox.py log.level --DEBUG FlowInterface openflow.of_01 --address=127.0.0.1 --port=$PORT