#!/bin/bash
cp pox/sendflows.py ~/pox/pox/
cp pox/revent.py ~/pox/pox/lib/revent/
cd ~/pox
./pox.py log.level --DEBUG sendflows forwarding.l2_learning openflow.discovery openflow.of_01 --address=127.0.0.1 --port=6653
