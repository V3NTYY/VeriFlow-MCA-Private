#!/bin/bash
cp PoxC.py ~/pox/pox/

cd ~/pox
./pox.py log.level --DEBUG PoxC openflow.of_01 --address=127.0.0.1 --port=6653
