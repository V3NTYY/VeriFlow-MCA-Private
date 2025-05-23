# VeriFlow

[VeriFlow: Verifying Network-Wide Invariants in Real Time](https://www.usenix.org/system/files/conference/nsdi13/nsdi13-final100.pdf)

## Requirements
  - Python 3
 
## Running


```bash
$ python3 Main.py
```

## Sample Run

```bash
$ python Main.py
  Enter network configuration file name (eg.: file.txt):
  > Topo/Topo1.txt
  
  Number of ECs: 9
  Number of affected ECs: 9
  Network is well-formed (No property violations)

  Add rule by entering A#switchIP-rulePrefix-nextHopIP (eg.A#127.0.0.1-128.0.0.0/2-127.0.0.2)
  Remove rule by entering R#switchIP-rulePrefix-nextHopIP (eg.R#127.0.0.1-128.0.0.0/2-127.0.0.2)
  To exit type exit
  > A#127.0.0.1-128.0.0.0/2-127.0.0.2
  
  Number of ECs: 9
  Number of affected ECs: 2
  Network is well-formed (No property violations)
  
  Add rule by entering A#switchIP-rulePrefix-nextHopIP (eg.A#127.0.0.1-128.0.0.0/2-127.0.0.2)
  Remove rule by entering R#switchIP-rulePrefix-nextHopIP (eg.R#127.0.0.1-128.0.0.0/2-127.0.0.2)
  To exit type exit
  > exit
 ```

  
  
  
  
  


