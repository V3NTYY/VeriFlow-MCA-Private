#!/usr/bin/env python

import sys
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.node import CPULimitedHost, Host, Node
from mininet.node import OVSKernelSwitch, UserSwitch
from mininet.node import IVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink, Intf
from subprocess import call

def getPorts():
    print("Please enter the ports used for both controllers in the format: <port1> <port2>")
    ports = input("Ports: ")
    ports = ports.split()
    if len(ports) != 2:
        print("Invalid input. Please enter two ports.")
        return getPorts()
    try:
        port1 = int(ports[0])
        port2 = int(ports[1])
    except ValueError:
        print("Invalid input. Please enter two valid port numbers.")
        return getPorts()
    if port1 < 1024 or port2 < 1024:
        print("Invalid input. Ports must be greater than 1024.")
        return getPorts()
    if port1 == port2:
        print("Invalid input. Ports must be different.")
        return getPorts()
    print("Please enter an offset (any number from 1-25)")
    offset = input("Offset: ")
    try:
        offset = int(offset)
    except ValueError:
        print("Invalid input. Please enter a valid number.")
        return getPorts()
    if offset < 1 or offset > 25:
        print("Invalid input. Offset must be between 1 and 25.")
        return getPorts()

    return [port1, port2, offset]

def myNetwork(port1, port2, offset):

    port1 = int(port1)
    port2 = int(port2)
    offset = int(offset)

    net = Mininet( topo=None,
                   build=False,
                   ipBase='10.0.0.0/8')

    info( '*** Adding controller\n' )
    c0=net.addController(name='c0',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=port1)
    
    c1=net.addController(name='c1',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=port2)

    info( '*** Add switches\n')
    s1 = net.addSwitch('s1', cls=OVSKernelSwitch, dpid='{}'.format(1*offset))
    s2 = net.addSwitch('s2', cls=OVSKernelSwitch, dpid='{}'.format(3*offset))
    s3 = net.addSwitch('s3', cls=OVSKernelSwitch, dpid='{}'.format(5*offset))
    s4 = net.addSwitch('s4', cls=OVSKernelSwitch, dpid='{}'.format(7*offset))

    info( '*** Add hosts\n')

    # Dynamically calculate IPs based on offset
    h1_ip = "10.0.0.1"
    h2_ip = "10.0.0.2"
    h3_ip = "10.0.0.3"
    h4_ip = "10.0.0.4"

    h2 = net.addHost('h2', cls=Host, ip=h1_ip, defaultRoute=None)
    h1 = net.addHost('h1', cls=Host, ip=h2_ip, defaultRoute=None)
    h3 = net.addHost('h3', cls=Host, ip=h3_ip, defaultRoute=None)
    h4 = net.addHost('h4', cls=Host, ip=h4_ip, defaultRoute=None)

    info( '*** Add links\n')
    net.addLink(h1, s1)
    net.addLink(h2, s1)
    net.addLink(h3, s2)
    net.addLink(h4, s4)
    net.addLink(s1, s3)
    net.addLink(s2, s4)


    info( '*** Starting network\n')
    net.build()
    info( '*** Starting controllers\n')
    for controller in net.controllers:
        controller.start()

    info( '*** Starting switches\n')
    net.get('s1').start([c0])
    net.get('s2').start([c0])
    net.get('s3').start([c1])
    net.get('s4').start([c1])

    info( '*** Post configure switches and hosts\n')
    s1.cmd('ifconfig s1 10.0.0.5')
    s2.cmd('ifconfig s2 10.0.0.6')
    s3.cmd('ifconfig s1 10.0.0.7')
    s4.cmd('ifconfig s2 10.0.0.8')

    CLI(net)
    net.stop()

if __name__ == '__main__':
    # results = getPorts()
    results = [sys.argv[1], sys.argv[2], sys.argv[3]]
    setLogLevel( 'info' )
    myNetwork(results[0], results[1], results[2])

