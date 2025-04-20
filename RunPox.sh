#!/bin/bash
cp FlowInteface.py ~/pox/pox/

cd ~/pox
./pox.py log.level --DEBUG FlowInterface forwarding.l2_learning openflow.discovery openflow.of_01 --address=127.0.0.1 --port=6653
