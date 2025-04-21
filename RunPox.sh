#!/bin/bash
cp FlowInterface.py ~/pox/pox/

cd ~/pox
./pox.py log.level --DEBUG FlowInterface openflow.of_01 --address=127.0.0.1 --port=6653
