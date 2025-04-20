#!/bin/bash
cp PoxC.py ~/pox/pox/
cp of_01.py ~/pox/pox/openflow/

cd ~/pox
./pox.py log.level --DEBUG PoxC openflow.of_01 --address=127.0.0.1 --port=6653
